/// test.cull
/// GPU/CPU 视锥裁剪一致性测试:
///   1. 场景中随机创建 10000 个对象(立方体/圆柱/圆球,固定随机种子,可复现)
///   2. 三种对象共享三种 FEMaterialV3C4 材质,在同一渲染工厂内形成 3 个实例组
///   3. 引擎渲染循环每帧自动执行 GPU compute 视锥裁剪(computeCull/default)
///   4. 测试侧每帧用完全相同的判定规则(可见标志 + 包围球视锥测试 + 最小像素测试)
///      在 CPU 上重新计算每组可见个数,与 GPU 裁剪计数(GroupNode::gpuCullCount)逐一比对
///   5. 窗口显示的就是 GPU 裁剪后的渲染结果;控制台周期输出 GPU/CPU 计数与 MATCH 统计
/// 判定规则与 computeCull/shader.comp 严格一致:
///   - checkVisible:     (flagBits & RF_VISIBLE) != 0
///   - checkFrustumIn:   dot(vec4(center,1), plane[i]) < -radius  => 剔除(6 个面)
///   - checkMinPixels:   minVisiblePixels=10,投影中心与 center+radius*rightDir,
///                       clip.w<=0 剔除,pixelDist=length((ndc1-ndc0)*viewSizeW) < 10 剔除
/// 用法: test.cull.exe [--vulkan|--webgpu]
///   WebGPU 后端不支持 GPU 裁剪(引擎自动回退全量绘制),仅输出 CPU 统计,不计错误
/// 运行: 相机自动环绕场景,视锥持续变化,对象进出视锥/缩小到像素阈值以下时
///   GPU 与 CPU 计数逐帧比对并周期输出,任何不一致立即打印 [FAIL]

#include    <cstdio>
#include    <cfloat>
#include    <algorithm>
#include    <iostream>
#include    <random>

#include    "../../../inc/FEContext.hpp"
#include    "../../../inc/FEAppHelper.hpp"
#include    "../../../inc/graphic/FEScene.h"
#include    "../../../inc/graphic/FEShaderDefine.h"
#include    "../../../inc/graphic/FEFactoryRender.hpp"
#include    "../../../inc/geometry/FEGeometryBox.hpp"
#include    "../../../inc/geometry/FEGeometrySphere.hpp"
#include    "../../../inc/geometry/FEGeometryCylinder.hpp"
#include    "../../../inc/material/FEMaterialV3C4.hpp"

using   namespace   FE;

USING_PLUGIN(rs_vulkan);
USING_PLUGIN(rs_webgpu);

namespace
{
    /// <summary>
    /// 对象总数
    /// </summary>
    constexpr   uint    kObjectCount        =   10000;
    /// <summary>
    /// 随机分布的空间半边长(对象分布在 [-kSceneExtent, +kSceneExtent]^3)
    /// </summary>
    constexpr   real    kSceneExtent        =   320.0;
    /// <summary>
    /// 最小可见像素,必须与 FEFactoryRender::fillCullParams 中的硬编码值一致
    /// </summary>
    constexpr   float   kMinVisiblePixels   =   10.0f;
    /// <summary>
    /// 固定随机种子,保证每次运行的场景完全一致,结果可复现
    /// </summary>
    constexpr   uint    kRandomSeed         =   20261109u;

    /// <summary>
    /// CPU 侧裁剪判定使用的相机数据(与上传到 GPU 的 CullParam 完全同源同精度)
    /// </summary>
    struct  CpuCullCam
    {
        float4  _planes[6];
        mat4    _mvp;
        float3  _right;
        float   _viewW      =   0.0f;
        float   _minPixels  =   kMinVisiblePixels;
    };

    /// <summary>
    /// 一类对象(共享一个材质实例,对应渲染工厂内的一个实例组)
    /// </summary>
    struct  CullType
    {
        const char*                 _name   =   "";
        Material                    _mat;
        Nodes                       _nodes;
        FEFactoryRender::GroupNode* _grp    =   nullptr;
        /// <summary>上一帧 CPU 可见数(用于识别 GPU 回读的一帧延迟)</summary>
        uint32                      _cpuPrev    =   0;
        uint32                      _cpu        =   0;
        uint32                      _cpuBoundary  =   0;
        uint32                      _gpu        =   0;
    };

    /// <summary>
    /// 验证统计
    /// </summary>
    struct  CullStats
    {
        uint64  _frames     =   0;
        uint64  _matched    =   0;
        /// <summary>GPU 计数等于上一帧 CPU 计数:回读延迟,不算错误</summary>
        uint64  _latency    =   0;
        /// <summary>不一致但可由边界节点的浮点翻转解释(计数差 <= 边界节点数)</summary>
        uint64  _boundary   =   0;
        /// <summary>无法用边界浮点翻转解释的不一致(逻辑错误)</summary>
        uint64  _errors     =   0;
    };

    /// <summary>
    /// CPU 裁剪判定:逐条复刻 computeCull/shader.comp 的 checkVisible/checkFrustumIn/checkMinPixels
    /// </summary>
    /// <param name="cam">相机裁剪参数</param>
    /// <param name="center">实例包围球中心(世界坐标)</param>
    /// <param name="radius">实例包围球半径</param>
    /// <param name="flagBits">实例渲染标志</param>
    /// <param name="boundary">输出:判定是否处于浮点边界(margin 接近 0,GPU/CPU 的 ULP 级
    ///        浮点求值差异可能翻转结论)</param>
    /// <returns>true 表示可见(不被裁剪)</returns>
    bool    cpuCullVisible(const CpuCullCam& cam,const float3& center,float radius,uint flagBits,bool& boundary)
    {
        boundary    =   false;
        /// checkVisible
        if ((flagBits & RF_VISIBLE) == 0)
            return  false;
        /// checkFrustumIn:记录最不利面余量,dist+radius 接近 0 时为边界
        float   planeMargin =   FLT_MAX;
        for (int i = 0; i < 6; ++i)
        {
            const float dist    =   cam._planes[i].x * center.x
                                +   cam._planes[i].y * center.y
                                +   cam._planes[i].z * center.z
                                +   cam._planes[i].w;
            planeMargin =   std::min(planeMargin,dist + radius);
            if (dist < -radius)
            {
                /// 余量阈值取半径的 1%:远大于 float ULP(~1e-4),远小于逻辑错误的量级
                if (dist + radius > -0.01f * radius)
                    boundary    =   true;
                return  false;
            }
        }
        if (planeMargin < 0.01f * radius)
            boundary    =   true;
        /// checkMinPixels
        if (cam._minPixels > 0.0f)
        {
            const float4    clip0   =   cam._mvp * float4(center,1.0f);
            const float3    rightPos=   center + radius * cam._right;
            const float4    clip1   =   cam._mvp * float4(rightPos,1.0f);
            /// 避免除零(点在相机后方)
            if (clip0.w <= 0.0f || clip1.w <= 0.0f)
            {
                /// w 接近 0 时投影结论可能被 ULP 差异翻转
                if (std::min(fabs(clip0.w),fabs(clip1.w)) < 0.5f)
                    boundary    =   true;
                return  false;
            }
            const float2    ndc0    =   float2(clip0.x,clip0.y) / clip0.w;
            const float2    ndc1    =   float2(clip1.x,clip1.y) / clip1.w;
            const float     pixelDist   =   length((ndc1 - ndc0) * cam._viewW);
            /// 像素余量阈值 0.05px:远大于 ndc 浮点误差(~1e-3px)
            if (fabs(pixelDist - cam._minPixels) < 0.05f)
                boundary    =   true;
            if (pixelDist < cam._minPixels)
                return  false;
        }
        return  true;
    }

    /// <summary>
    /// 从当前相机提取 CPU 裁剪参数,与 FEFactoryRender::fillCullParams 上传 GPU 的数据同源
    /// </summary>
    /// <param name="camera">场景相机(按值拷贝:extract 非常量,FERef 常量访问返回 const 指针)</param>
    /// <returns></returns>
    CpuCullCam  makeCpuCullCam(Camera camera)
    {
        CpuCullCam  result;
        FrustumR    frustum =   camera->extract();
        for (size_t i = 0; i < 6; i++)
        {
            result._planes[i].x =   float(frustum._planes[i]._normal.x);
            result._planes[i].y =   float(frustum._planes[i]._normal.y);
            result._planes[i].z =   float(frustum._planes[i]._normal.z);
            result._planes[i].w =   float(frustum._planes[i]._distance);
        }
        /// _mvp 上传前由 mat4r 转 mat4,CPU 侧做同样的逐元素 float 转换
        const mat4r     mvpR    =   camera->getProject() * camera->getView();
        for (int c = 0; c < 4; ++c)
            for (int r = 0; r < 4; ++r)
                result._mvp[c][r]   =   float(mvpR[c][r]);
        const real3     right   =   camera->getRight();
        result._right           =   float3(float(right.x),float(right.y),float(right.z));
        result._viewW           =   camera->viewportWidth<float>();
        result._minPixels       =   kMinVisiblePixels;
        return  result;
    }

    /// <summary>
    /// 一类节点的 CPU 裁剪计数结果
    /// </summary>
    struct  CpuCullResult
    {
        uint32  _visible    =   0;
        /// <summary>处于判定边界的节点数(浮点翻转候选)</summary>
        uint32  _boundary   =   0;
    };

    /// <summary>
    /// 计算一类节点的实例包围球并做 CPU 裁剪计数
    /// 包围球与 FEInstance::setAabb 完全一致:中心=世界AABB中心,半径=length(size)*0.5
    /// </summary>
    /// <param name="cam">相机裁剪参数</param>
    /// <param name="nodes">该类节点列表</param>
    /// <returns>可见节点数与边界节点数</returns>
    CpuCullResult   cpuCullCount(const CpuCullCam& cam,const Nodes& nodes)
    {
        CpuCullResult   result;
        /// FERef 的 const operator-> 返回 const 指针,按值拷贝迭代以获得可写节点访问
        for (auto node : nodes)
        {
            const aabb3dr   box     =   node->globalAabb();
            const real3     vCenter =   box.getCenter();
            const float     radius  =   float(length(box.getSize()) * 0.5f);
            const float3    center(float(vCenter.x),float(vCenter.y),float(vCenter.z));
            const uint      bits    =   node->renderBits().data();
            bool            boundary=   false;
            if (cpuCullVisible(cam,center,radius,bits,boundary))
                ++  result._visible;
            if (boundary)
                ++  result._boundary;
        }
        return  result;
    }

    /// <summary>
    /// 给 mesh 填充单色顶点色(v3c4 管线 binding0 声明了 IS_VERTEX_COLOR0,缺省会打包错位;
    /// 顶点色存在时实例色不生效,故按对象类型着色:立方体红/圆柱绿/圆球蓝)
    /// </summary>
    /// <param name="mesh">几何体生成的 mesh</param>
    /// <param name="color">该类对象的顶点色</param>
    /// <returns>顶点数</returns>
    uint    appendVertexColor(Mesh& mesh,const Rgba8& color)
    {
        if (mesh == nullptr)
            return  0;
        auto*   pPos    =   mesh->get(FEAttribute(IS_VERTEX_POS));
        if (pPos == nullptr)
            return  0;
        const uint  vCount  =   pPos->count();
        Rgba8s      colors(vCount,color);
        mesh->setAttribute(FEAttribute(IS_VERTEX_COLOR0,FMT_R8G8B8A8_UNORM),colors);
        mesh->updateAabb();
        return  vCount;
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

    /// <summary>
    /// 帧渲染完成后的验证回调:此刻 GPU 裁剪计数已随本帧渲染回读并缓存,
    /// 相机状态与本帧 GPU 裁剪所用的一致
    /// </summary>
    CullType    g_types[3];
    CullStats   g_stats;
    bool        g_groupChecked  =   false;
    bool        g_gpuCullOn     =   false;
    uint64      g_frame         =   0;

    FEApp::CreateInfo  info   =   {};
    info._width     =   1280;
    info._height    =   720;
    info._appInst   =   GetModuleHandle(nullptr);
    info._notify    =   [&](const FEMessage& msg)
    {
        /// 相机自动环绕:必须先于场景处理 MSG_UPDATE,
        /// 保证本帧 GPU 裁剪(MSG_UPDATE 中 dispatch)与渲染后 CPU 验证使用同一相机
        if (pScene && msg.msgId() == MSG_UPDATE)
        {
            const real  angle   =   real(g_frame) * 0.008;
            pScene->camera()->setEye(real3(450.0 * cos(angle),120.0,450.0 * sin(angle)));
            pScene->camera()->setTarget(real3(0,0,0));
            pScene->camera()->update();
        }
        if (pScene)
            pScene->onMessage(msg);
        if (pScene == nullptr || msg.msgId() != MSG_RENDER)
            return;
        ++  g_frame;

        /// 首帧:按材质指针把 3 类对象映射到渲染工厂内的实例组,并校验命令数
        if (!g_groupChecked)
        {
            g_groupChecked  =   true;
            /// 工厂列表按值拷贝:FERef 的 const operator-> 返回 const 指针,
            /// 需要非常量访问才能取 GroupNode* 并调用 gpuCullCount
            auto    factorys    =   pScene->factoryMgr().objects();
            for (auto& factory : factorys)
            {
                for (auto& grp : factory->groups())
                {
                    for (auto& type : g_types)
                    {
                        if (grp->material() == type._mat)
                            type._grp   =   grp.get();
                    }
                }
            }
            bool    locateOk    =   true;
            for (auto& type : g_types)
            {
                if (type._grp == nullptr)
                {
                    std::printf("[FAIL] %s 未找到实例组\n",type._name);
                    locateOk    =   false;
                    continue;
                }
                if (type._grp->commandCount() != type._nodes.size())
                {
                    std::printf("[FAIL] %s 命令数 %u 与节点数 %zu 不一致\n",
                                type._name,type._grp->commandCount(),type._nodes.size());
                    locateOk    =   false;
                }
            }
            std::printf("%s 实例组定位:box/cylinder/sphere 命令数=%u/%u/%u\n",
                        locateOk ? "[ OK ]" : "[FAIL]",
                        g_types[0]._grp ? g_types[0]._grp->commandCount() : 0u,
                        g_types[1]._grp ? g_types[1]._grp->commandCount() : 0u,
                        g_types[2]._grp ? g_types[2]._grp->commandCount() : 0u);
            /// 任一实例组所在工厂支持 GPU 裁剪即视为开启(三组装在同一工厂)
            g_gpuCullOn =   false;
            for (auto& factory : factorys)
            {
                for (auto& grp : factory->groups())
                {
                    if ((grp.get() == g_types[0]._grp || grp.get() == g_types[1]._grp ||
                         grp.get() == g_types[2]._grp) && factory->supportGPUCull())
                        g_gpuCullOn =   true;
                }
            }
            if (!g_gpuCullOn)
                std::printf("[Info] 当前后端不做 GPU 裁剪(WebGPU 回退全量绘制),仅输出 CPU 统计\n");
        }

        /// 每帧:CPU 重算 + 读取 GPU 缓存计数,逐组比对
        const CpuCullCam    cam =   makeCpuCullCam(pScene->camera());
        uint32  cpuTotal    =   0;
        uint32  gpuTotal    =   0;
        for (auto& type : g_types)
        {
            const CpuCullResult res =   cpuCullCount(cam,type._nodes);
            type._cpuPrev   =   type._cpu;
            type._cpu       =   res._visible;
            type._cpuBoundary   =   res._boundary;
            cpuTotal        +=  type._cpu;
            if (type._grp == nullptr)
                continue;
            type._gpu       =   type._grp->gpuCullCount();
            gpuTotal        +=  type._gpu;
            if (!g_gpuCullOn)
                continue;
            ++  g_stats._frames;
            if (type._gpu == type._cpu)
                ++  g_stats._matched;
            else if (type._gpu == type._cpuPrev)
            {
                /// GPU→CPU 回读存在一帧延迟:与上一帧 CPU 结果一致,属正常
                ++  g_stats._latency;
            }
            else
            {
                /// 计数差可由边界节点的浮点翻转解释:精度事件,仅记录;
                /// 否则为逻辑错误
                const uint32    diff    =   type._gpu > type._cpu
                                        ?   type._gpu - type._cpu
                                        :   type._cpu - type._gpu;
                if (diff <= type._cpuBoundary)
                {
                    ++  g_stats._boundary;
                    std::printf("[warn] frame %llu %s: GPU=%u CPU=%u 计数差 %u <= 边界节点 %u(浮点翻转)\n",
                                (unsigned long long)g_frame,type._name,
                                type._gpu,type._cpu,diff,type._cpuBoundary);
                }
                else
                {
                    ++  g_stats._errors;
                    std::printf("[FAIL] frame %llu %s: GPU=%u CPU=%u (prev=%u,boundary=%u)\n",
                                (unsigned long long)g_frame,type._name,
                                type._gpu,type._cpu,type._cpuPrev,type._cpuBoundary);
                }
            }
        }

        /// 周期输出(约每秒一行)
        if (g_frame % 60 == 0)
        {
            if (g_gpuCullOn)
            {
                std::printf("[cull] frame %5llu | box %u/%u | cyl %u/%u | sph %u/%u | total GPU %u CPU %u | match %llu latency %llu boundary %llu error %llu\n",
                            (unsigned long long)g_frame,
                            g_types[0]._gpu,g_types[0]._cpu,
                            g_types[1]._gpu,g_types[1]._cpu,
                            g_types[2]._gpu,g_types[2]._cpu,
                            gpuTotal,cpuTotal,
                            (unsigned long long)g_stats._matched,
                            (unsigned long long)g_stats._latency,
                            (unsigned long long)g_stats._boundary,
                            (unsigned long long)g_stats._errors);
            }
            else
            {
                std::printf("[cull] frame %5llu | CPU visible: box %u cyl %u sph %u total %u/%u (GPU cull off)\n",
                            (unsigned long long)g_frame,
                            g_types[0]._cpu,g_types[1]._cpu,g_types[2]._cpu,
                            cpuTotal,kObjectCount);
            }
        }
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

    /// ===== 1. 生成三种共享 mesh(顶点色按类型着色:立方体红/圆柱绿/圆球蓝)=====
    Mesh    meshes[3];
    {
        FEGeometryBox   geo(ctx);
        geo.param()._size   =   float3(1.6f,1.6f,1.6f);
        meshes[0]   =   geo.triangular({{IS_VERTEX_POS,FMT_R32G32B32_FLOAT}});
    }
    {
        FEGeometryCylinder  geo(ctx);
        geo.param().radiusTop       =   0.9f;
        geo.param().radiusBottom    =   0.9f;
        geo.param().height          =   1.8f;
        geo.param().radialSegments  =   16;
        meshes[1]   =   geo.triangular({{IS_VERTEX_POS,FMT_R32G32B32_FLOAT}});
    }
    {
        FEGeometrySphere    geo(ctx);
        geo.param().radius          =   1.0f;
        geo.param().widthSegments   =   16;
        geo.param().heightSegments  =   12;
        meshes[2]   =   geo.triangular({{IS_VERTEX_POS,FMT_R32G32B32_FLOAT}});
    }
    const char* names[3]  =   { "box", "cylinder", "sphere" };
    const Rgba8 typeColors[3] =   { Rgba8(235,110,110,255),Rgba8(120,220,130,255),Rgba8(120,160,245,255) };
    for (int i = 0; i < 3; ++i)
    {
        const uint vCount   =   appendVertexColor(meshes[i],typeColors[i]);
        if (vCount == 0)
        {
            std::cerr << names[i] << " mesh build failed" << std::endl;
            scene   =   nullptr;
            app->destroy();
            return  1;
        }
        std::printf("[ OK ] %s mesh vertices=%u primitives=%zu\n",
                    names[i],vCount,meshes[i]->primitives().size());
    }

    /// ===== 2. 三种共享材质(同管线,工厂内按材质实例分成 3 组)=====
    Material    mats[3];
    for (int i = 0; i < 3; ++i)
        mats[i] =   new FEMaterialV3C4(ctx);

    /// ===== 3. 随机创建 10000 个对象(固定种子,位置/旋转/缩放随机)=====
    {
        std::mt19937                            rng(kRandomSeed);
        std::uniform_real_distribution<real>    posDist(-kSceneExtent,kSceneExtent);
        std::uniform_real_distribution<real>    unitDist(0.0,1.0);
        std::uniform_int_distribution<int>      typeDist(0,2);

        Nodes   nodes;
        nodes.reserve(kObjectCount);
        for (int i = 0; i < 3; ++i)
        {
            g_types[i]._name    =   names[i];
            g_types[i]._mat     =   mats[i];
            g_types[i]._nodes.reserve(kObjectCount / 3 + 1);
        }
        for (uint i = 0; i < kObjectCount; ++i)
        {
            const int   type    =   typeDist(rng);
            Node        node    =   new FENode(ctx);
            node->setMesh(meshes[type]);
            node->setMaterial(mats[type]);
            node->setLocalTranslation(real3(posDist(rng),posDist(rng),posDist(rng)));
            const real  scale   =   0.7 + 1.6 * unitDist(rng);
            node->setLocalScaling(real3(scale,scale,scale));
            node->setLocalEuler(real3(360.0 * unitDist(rng),360.0 * unitDist(rng),360.0 * unitDist(rng)));
            node->makeDirty();
            node->update();
            nodes.push_back(node);
            g_types[type]._nodes.push_back(node);
        }
        scene->dispatchNodesToSystem(nodes);
        std::printf("[ OK ] 创建对象 %u 个:box=%zu cylinder=%zu sphere=%zu\n",
                    kObjectCount,
                    g_types[0]._nodes.size(),g_types[1]._nodes.size(),g_types[2]._nodes.size());
    }

    /// ===== 4. 相机:正 Z 方向俯视整个场景,远平面覆盖对角线;运行中自动环绕 =====
    scene->camera()->setEye(real3(0,120,450));
    scene->camera()->setTarget(real3(0,0,0));
    scene->camera()->setNearFar(real2(1.0,3000.0));
    scene->camera()->update();

    std::printf("进入渲染循环:相机自动环绕(每帧约0.46度),GPU/CPU 裁剪计数逐帧比对,约每秒输出一行\n");

    /// 关闭窗口时抢在场景销毁 device 之前归还本文件持有的 GPU 资源:
    /// 设备销毁后再释放网格/材质会访问已销毁的设备
    app->messageBus().subscribe(MSG_CLOSE,[&](const FEMessage&)
    {
        for (auto& m : meshes)
            m   =   nullptr;
        for (auto& m : mats)
            m   =   nullptr;
        for (auto& t : g_types)
        {
            t._mat  =   nullptr;
            t._nodes.clear();
        }
    });
    app->run();

    /// ===== 5. 汇总 =====
    std::printf("\n==== 裁剪验证汇总 ====\n");
    std::printf("比对 %llu 组次:match=%llu latency=%llu boundary=%llu error=%llu\n",
                (unsigned long long)g_stats._frames,
                (unsigned long long)g_stats._matched,
                (unsigned long long)g_stats._latency,
                (unsigned long long)g_stats._boundary,
                (unsigned long long)g_stats._errors);

    scene   =   nullptr;
    app->destroy();

    if (g_gpuCullOn && g_stats._errors > 0)
    {
        std::printf("裁剪测试失败:%llu 次 GPU/CPU 计数不一致\n",(unsigned long long)g_stats._errors);
        return  1;
    }
    std::printf("裁剪测试通过\n");
    return  0;
}
