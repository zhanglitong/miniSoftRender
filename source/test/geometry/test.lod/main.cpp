/// test.lod
/// LOD(mesh 级)测试:
///   1. 生成 4 个不同细分等级的球体,把顶点拼进同一个 mesh,
///      索引按 L0+L1+L2+L3 连续存放(每级索引加上该级顶点基址)
///   2. 给图元挂 FELod:4 级{start,count,dist},start 相对该图元自身索引数组
///   3. 校验 LOD 数据结构、索引范围、primitive 拷贝深拷贝 LOD
///   4. 沿相机视线方向摆一排实例,GPU 裁剪按世界表面距离选级
///      近距离显示高模,远距离显示低模(需要 Vulkan 路径;WebGPU 不做选级)
/// 用法: test.lod.exe [--vulkan|--webgpu]

#include    <cstdio>
#include    <iostream>

#include    "../../../inc/FEContext.hpp"
#include    "../../../inc/FEAppHelper.hpp"
#include    "../../../inc/graphic/FEScene.h"
#include    "../../../inc/geometry/FEGeometrySphere.hpp"
#include    "../../../inc/material/FEMaterialPBR.hpp"

using   namespace   FE;

USING_PLUGIN(rs_vulkan);
USING_PLUGIN(rs_webgpu);

namespace
{
    int     gFailures   =   0;

    void    check(bool ok,const char* msg)
    {
        std::printf("%s %s\n",ok ? "[ OK ]" : "[FAIL]",msg);
        if (!ok)
            ++ gFailures;
    }

    /// <summary>
    /// 一级 LOD 的生成参数与距离阈值
    /// </summary>
    struct  LodLevel
    {
        uint    _segW;
        uint    _segH;
        float   _dist;
    };

    constexpr   LodLevel    g_levels[4]  =
    {
        { 32, 16,   0.0f },
        { 16,  8,  15.0f },
        {  8,  4,  40.0f },
        {  4,  2, 100.0f },
    };

    /// <summary>
    /// 收集一级球体:顶点数据追加到合并缓冲,索引加上该级顶点基址后追加
    /// </summary>
    bool    appendLevel(FEContext& ctx,const LodLevel& level,uint& baseVertex,
                        uint8s& poss,uint8s& nors,uint8s& uvs,
                        uint16s& indices,uint& start,uint& count)
    {
        FEGeometrySphere    geo(ctx);
        geo.param().radius          =   1.0f;
        geo.param().widthSegments   =   static_cast<int>(level._segW);
        geo.param().heightSegments  =   static_cast<int>(level._segH);

        Mesh    sub =   geo.triangular({
            {IS_VERTEX_POS,       FMT_R32G32B32_FLOAT},
            {IS_VERTEX_NOR,       FMT_R32G32B32_FLOAT},
            {IS_VERTEX_TEXCOORD0, FMT_R32G32_FLOAT} });
        if (sub == nullptr)
            return  false;
        auto*   pPos    =   sub->get(FEAttribute(IS_VERTEX_POS));
        auto*   pNor    =   sub->get(FEAttribute(IS_VERTEX_NOR));
        auto*   pUv     =   sub->get(FEAttribute(IS_VERTEX_TEXCOORD0));
        if (pPos == nullptr || pNor == nullptr || pUv == nullptr)
            return  false;
        /// 合并后各属性顶点数必须一致
        if (pNor->count() != pPos->count() || pUv->count() != pPos->count())
            return  false;
        if (sub->primitives().size() != 1)
            return  false;
        auto*   pDraw   =   sub->primitives().front()->as<FEDrawElementUint16>();
        if (pDraw == nullptr)
            return  false;

        uint32  vCount  =   pPos->count();
        poss.insert(poss.end(),pPos->data(),pPos->data() + pPos->length());
        nors.insert(nors.end(),pNor->data(),pNor->data() + pNor->length());
        uvs.insert(uvs.end(), pUv->data(), pUv->data()  + pUv->length());

        start   =   uint(indices.size());
        count   =   pDraw->count();
        for (auto idx : pDraw->index())
            indices.push_back(uint16(idx + baseVertex));

        baseVertex  +=  vCount;
        return  true;
    }
}

int     main(int argc, char** argv)
{
    FEUuid  rendererId  =   RS_VULKAN;
    for (int i = 1; i < argc; ++i)
    {
        const char* arg = argv[i];
        if (_stricmp(arg, "--webgpu") == 0 || _stricmp(arg, "-webgpu") == 0 ||
            _stricmp(arg, "/webgpu") == 0)
            rendererId = RS_WEBGPU;
        else if (_stricmp(arg, "--vulkan") == 0 || _stricmp(arg, "-vulkan") == 0 ||
                 _stricmp(arg, "/vulkan") == 0)
            rendererId = RS_VULKAN;
    }

    FEContext&  ctx =   FEContext::instance();
    FEScene*    pScene  =   nullptr;

    FEApp::CreateInfo  info   =   {};
    info._width     =   1280;
    info._height    =   720;
    info._appInst   =   GetModuleHandle(nullptr);
    info._notify    =   [&](const FEMessage& msg)
    {
        if (pScene)
            pScene->onMessage(msg);
    };
    App app =   FEAppHelper::create(ctx, info);
    if (app == nullptr)
    {
        std::cerr << "create app failed" << std::endl;
        return  1;
    }
    ctx.setWindow(app.get());
    ctx.setWorkPath(app->path());
    ctx.setResourcePath(app->path() + "/../");

    Scene   scene   =   new FEScene(ctx);
    pScene          =   scene.get();
    if (!scene->setup(app, rendererId))
    {
        std::cerr << "scene setup failed" << std::endl;
        scene   =   nullptr;
        app->destroy();
        return  1;
    }

    /// ===== 1. 构建 4 级 LOD 的合并 mesh =====
    uint8s      poss;
    uint8s      nors;
    uint8s      uvs;
    uint16s     indices;
    uint        starts[4]   =   {};
    uint        counts[4]   =   {};
    uint        baseVertex  =   0;

    bool        built       =   true;
    for (uint i = 0; i < 4; ++i)
    {
        if (!appendLevel(ctx,g_levels[i],baseVertex,poss,nors,uvs,indices,starts[i],counts[i]))
        {
            std::printf("[FAIL] 生成第 %u 级球体失败\n",i);
            built   =   false;
            break;
        }
    }
    check(built,"4 级球体索引合并");
    if (!built)
    {
        scene   =   nullptr;
        app->destroy();
        return  1;
    }

    /// start 必须是相对该图元索引数组的连续偏移,count 为该级索引数
    uint    total   =   0;
    for (uint i = 0; i < 4; ++i)
    {
        if (starts[i] != total)
        {
            std::printf("[FAIL] 第 %u 级 start=%u,期望 %u\n",i,starts[i],total);
            ++ gFailures;
        }
        total   +=  counts[i];
    }
    check(total == indices.size(),"索引总长度 == 各级 count 之和");
    check(counts[0] > counts[1] && counts[1] > counts[2] && counts[2] > counts[3],
          "索引数随 LOD 级递减");

    /// 每一级索引必须落在该级自己的顶点区间内(合并顶点缓冲)
    {
        bool    inRange =   true;
        uint    vBase   =   0;
        for (uint i = 0; i < 4; ++i)
        {
            uint    vCount  =   (g_levels[i]._segW + 1) * (g_levels[i]._segH + 1);
            for (uint k = 0; k < counts[i]; ++k)
            {
                uint    idx =   indices[starts[i] + k];
                if (idx < vBase || idx >= vBase + vCount)
                {
                    inRange =   false;
                    break;
                }
            }
            if (!inRange)
            {
                std::printf("[FAIL] 第 %u 级索引越出本级的顶点区间\n",i);
                ++ gFailures;
                break;
            }
            vBase   +=  vCount;
        }
        check(inRange,"各级索引落在本级顶点区间内");
    }

    Mesh    mesh    =   new FEMesh(ctx);
    mesh->setAttribute(FEAttribute(IS_VERTEX_POS,       FMT_R32G32B32_FLOAT),poss);
    mesh->setAttribute(FEAttribute(IS_VERTEX_NOR,       FMT_R32G32B32_FLOAT),nors);
    mesh->setAttribute(FEAttribute(IS_VERTEX_TEXCOORD0, FMT_R32G32_FLOAT),   uvs);

    auto*   pDraw   =   new FEDrawElementUint16(ctx);
    pDraw->setPrimitive(PRI_TRIANGLES);
    pDraw->index()  =   indices;
    Primitive   pri =   pDraw;
    mesh->setPrimitives({pri});
    mesh->updateAabb();
    check(!mesh->primitives().empty(),"mesh 图元非空");

    /// ===== 2. 挂 LOD =====
    Lod     lod =   new FELod(ctx);
    for (uint i = 0; i < 4; ++i)
    {
        lod->_lod[i].start  =   starts[i];
        lod->_lod[i].count  =   counts[i];
        lod->_lod[i].dist   =   g_levels[i]._dist;
        /// 第 i 项的 start 必须等于此前所有 count 之和
        check(lod->_lod[i].start == (i == 0 ? 0u : lod->_lod[i-1].start + lod->_lod[i-1].count),
              "LOD 各级 start 连续");
        check(lod->_lod[i].count > 0,"LOD 各级 count 有效");
    }
    check(lod->_lod[0].dist == 0.0f,"L0 距离阈值为 0(始终可见)");
    check(lod->_lod[3].dist > lod->_lod[2].dist,"距离阈值随级递增");
    pDraw->setLod(lod);
    check(pDraw->lod() == lod,"图元保存 LOD 指针");

    /// 拷贝构造必须深拷贝 LOD,避免多个图元共享同一份可变数据
    {
        FEDrawElementUint16 copy(*pDraw);
        check(copy.lod() != nullptr && copy.lod().get() != lod.get(),"图元拷贝得到独立 LOD");
        check(copy.lod() != nullptr && copy.lod()->_lod[3].count == counts[3],"图元拷贝保留 4 级数据");
        check(copy.count() == indices.size(),"图元拷贝保留完整索引");
    }

    /// ===== 3. 沿相机视线方向摆实例,观察 GPU 选级 =====
    /// 相机在 (0,-6,3) 看向 +Y,实例沿 +Y 排列,表面距离逐级跨过阈值
    scene->camera()->setEye(real3(0,-6,3));
    scene->camera()->setTarget(real3(0,30,0));
    scene->camera()->update();

    const   real  ys[]  =   { 0.0f, 12.0f, 25.0f, 45.0f, 80.0f, 140.0f, 220.0f };
    Nodes       nodes;
    for (real y : ys)
    {
        Node    node    =   new FENode(ctx);
        node->setMesh(mesh);
        FEMaterialPBR* pMat  =   new FEMaterialPBR(ctx);
        pMat->data()._value._diffuse  =   float4(0.15f, 0.55f, 0.95f, 1.0f);
        pMat->data()._value._emissive =   float4(0.10f, 0.35f, 0.65f, 1.0f);
        pMat->data().update();
        node->setMaterial(pMat);
        node->setLocalTranslation(real3(0,y,0));
        node->makeDirty();
        node->update();
        nodes.push_back(node);
    }
    scene->dispatchNodesToSystem(nodes);

    /// 打印每个实例按当前相机/阈值应选中的级别(与画面比对)
    std::printf("\n实例期望选级(相机 %s):\n",rendererId == RS_WEBGPU ? "WebGPU 不做选级" : "Vulkan GPU 裁剪");
    for (real y : ys)
    {
        float   dist    =   float(FE::length(real3(0,y,0) - real3(0,-6,3))) - 1.0f;
        uint    sel     =   0;
        for (uint i = 0; i < 4; ++i)
        {
            if (lod->_lod[i].count > 0 && dist >= lod->_lod[i].dist)
                sel =   i;
        }
        std::printf("  y=%6.1f  表面距离=%7.2f  ->  L%u (count=%u)\n",
                    float(y),dist,sel,counts[sel]);
    }
    std::printf("\n");

    if (gFailures > 0)
    {
        std::printf("LOD 测试失败:%d 项\n",gFailures);
        scene   =   nullptr;
        app->destroy();
        return  1;
    }
    std::printf("LOD 数据校验通过,进入渲染循环(近处高模,远处低模)...\n");

    app->run();

    scene   =   nullptr;
    app->destroy();
    return  0;
}
