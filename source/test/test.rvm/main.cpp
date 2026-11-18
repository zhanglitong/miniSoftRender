/// test.rvm
/// AVEVA RVM 读取测试:
///   1. 创建引擎 + 场景(创建 PBR 材质需要显卡资源)
///   2. 按格式描述符查询 RVM 读取器,加载 SAMPLE.RVM
///   3. 校验节点/网格/材质数量与整体包围盒
/// 用法: test.rvm.exe [rvm文件路径]

#include    <cstdio>
#include    <cstring>
#include    <cmath>
#include    <algorithm>
#include    <filesystem>
#include    <set>
#include    <string>

#include    "../../inc/FEContext.hpp"
#include    "../../inc/FEFileFormatHelper.hpp"
#include    "../../inc/FEAppHelper.hpp"
#include    "../../inc/FEReaderHelper.hpp"
#include    "../../inc/graphic/FEScene.h"
#include    "../../inc/mesh/FEMesh.hpp"
#include    "../../inc/node/FENode.hpp"
#include    "../../inc/material/FEMaterialPBR.hpp"
#include    "../../inc/fileFormat/rvm/FEFormatRVMReader.hpp"
#include    "../../inc/fileFormat/att/FEFormatATTReader.hpp"
#include    "../../inc/FEBuffer.hpp"
#include    "../../inc/FEProperty.hpp"

using   namespace   FE;

/// 注册渲染后端插件,材质创建需要显卡资源
USING_PLUGIN(rs_vulkan);

namespace
{
    int     g_pass  =   0;
    int     g_fail  =   0;

    void    check(bool ok,const char* label)
    {
        if (ok)     {   ++  g_pass;     printf("[PASS] %s\n",label);    }
        else        {   ++  g_fail;     printf("[FAIL] %s\n",label);    }
        fflush(stdout);
    }

    struct  Stat
    {
        size_t              nodes       =   0;
        size_t              meshes      =   0;
        size_t              vertices    =   0;
        size_t              badMesh     =   0;
        std::set<FEMaterial*>   materials;
        real3               min         =   real3(1e30);
        real3               max         =   real3(-1e30);
    };

    void    collect(const Node& node,const mat4r& parent,Stat& stat)
    {
        if (node == nullptr)
            return;
        ++  stat.nodes;
        const   mat4r   world   =   parent * node->localTransform();
        Mesh    mesh            =   node->mesh();
        if (mesh != nullptr)
        {
            ++  stat.meshes;
            auto*   pos =   mesh->get(FEAttribute(IS_VERTEX_POS));
            auto*   nor =   mesh->get(FEAttribute(IS_VERTEX_NOR));
            auto*   uv  =   mesh->get(FEAttribute(IS_VERTEX_TEXCOORD0));
            if (pos == nullptr || nor == nullptr || uv == nullptr
                || pos->count() == 0 || nor->count() != pos->count() || uv->count() != pos->count()
                || mesh->primitives().empty())
            {
                ++  stat.badMesh;
            }
            else
            {
                stat.vertices   +=  pos->count();
            }
            const   auto    aabb    =   mesh->aabb();
            for (uint i = 0; i < 8; ++i)
            {
                const   real3   corner((i & 1) ? aabb._maximum.x : aabb._minimum.x,
                                       (i & 2) ? aabb._maximum.y : aabb._minimum.y,
                                       (i & 4) ? aabb._maximum.z : aabb._minimum.z);
                const   real4   p   =   world * real4(corner,1.0);
                for (int k = 0; k < 3; ++k)
                {
                    stat.min[k] =   (std::min)(stat.min[k],p[k]);
                    stat.max[k] =   (std::max)(stat.max[k],p[k]);
                }
            }
            if (node->material() != nullptr)
                stat.materials.insert(node->material().get());
        }
        for (const auto& child : node->children())
            collect(child,world,stat);
    }

    void    checkModel(const Node& root,Stat& stat,const char* tag)
    {
        printf("--- %s ---\n",tag);
        collect(root,mat4r(real(1.0)),stat);
        printf("       nodes = %zu, meshes = %zu, vertices = %zu, materials = %zu\n",
               stat.nodes,stat.meshes,stat.vertices,stat.materials.size());
        printf("       bbox = (%.3f,%.3f,%.3f) - (%.3f,%.3f,%.3f)\n",
               stat.min.x,stat.min.y,stat.min.z,stat.max.x,stat.max.y,stat.max.z);

        check(root != nullptr,"root exists");
        if (root == nullptr)
            return;
        check(std::strcmp(root->name(),"SAM") == 0,"root name = SAM");
        check(stat.nodes == 8127,"node count = 8127");
        check(stat.meshes == 4974,"mesh count = 4974");
        check(stat.materials.size() == 5,"material count = 5");
        check(stat.badMesh == 0,"all meshes have pos/nor/uv + primitive");
        check(stat.vertices > 100000,"total vertex count > 100000");

        const   real3   size    =   stat.max - stat.min;
        check(size.x > 5.0 && size.x < 200.0
           && size.y > 5.0 && size.y < 200.0
           && size.z > 5.0 && size.z < 200.0,"world bbox in metres");
    }
};

int     main(int argc,char** argv)
{
    setvbuf(stdout,nullptr,_IONBF,0);
    const   String  file    =   argc > 1 ? argv[1] : "E:/model/RVM/SAMPLE.RVM";
    if (!std::filesystem::exists(file))
    {
        printf("[FAIL] sample not found: %s\n",file.c_str());
        return  1;
    }

    FEContext&  ctx =   FEContext::instance();
    FEScene*    pScene  =   nullptr;

    FEApp::CreateInfo   info    =   {};
    info._width     =   640;
    info._height    =   480;
    info._appInst   =   GetModuleHandle(nullptr);
    info._notify    =   [&](const FEMessage& msg)
    {
        if (pScene)
            pScene->onMessage(msg);
    };
    App app =   FEAppHelper::create(ctx,info);
    check(app != nullptr,"engine: create app");
    if (app == nullptr)
        return  1;

    ctx.setWindow(app.get());
    ctx.setWorkPath(app->path());
    ctx.setResourcePath(app->path() + "/../");

    Scene   scene   =   new FEScene(ctx);
    pScene          =   scene.get();
    check(scene->setup(app,RS_VULKAN),"engine: scene setup");

    /// 1. 按格式描述符查询读取器并加载文件
    FEFileFormat    fmt(".rvm","1.0.0.0","AVEVA RVM Model!");
    auto    reader  =   FEFileFormatHelper::queryReader(ctx,fmt);
    check(reader != nullptr,"query .rvm reader");
    if (reader != nullptr)
    {
        Objects objects =   reader->readFiles({file});
        check(objects.size() == 1,"readFiles: object count = 1");
        check(reader->progress() == real(1),"readFiles: progress reaches 1");
        Node    root    =   objects.size() == 1 ? objects[0]->cast<FENode>() : nullptr;
        check(root != nullptr,"readFiles: root is node");
        Stat    stat;
        checkModel(root,stat,"readFiles");
    }

    /// 2. 使用大写扩展名格式,内存路径读取同一文件
    FEFileFormat    fmtUpper(".RVM","1.0.0.0","AVEVA RVM Model!");
    auto    reader2 =   FEFileFormatHelper::queryReader(ctx,fmtUpper);
    check(reader2 != nullptr,"query .RVM reader (uppercase)");
    if (reader2 != nullptr)
    {
        FEReaderFile    fileReader(ctx,file.c_str());
        FEReaderHelper  helper(fileReader);
        Buffer          buffer  =   helper.read(ctx);
        fileReader.close();
        check(buffer != nullptr && buffer->length() > 0,"read buffer");

        Objects objects =   reader2->readBuffers({buffer});
        check(objects.size() == 1,"readBuffers: object count = 1");
        check(reader2->progress() == real(1),"readBuffers: progress reaches 1");
        Node    root    =   objects.size() == 1 ? objects[0]->cast<FENode>() : nullptr;
        check(root != nullptr,"readBuffers: root is node");
        Stat    stat;
        checkModel(root,stat,"readBuffers");
    }

    /// 3. .ATT 属性文件:解析记录并按节点名挂接 FEProperty 组件
    {
        const   String  att =
            "CADC_Attributes_File v1.0 , start: NEW , end: END , name_end: := , sep: &end&\n"
            "NEW Header Information\n"
            "  Source:= PDMS Data &end& Date:= 30 Nov 2021 &end& Time:= 09:34\n"
            "END\n"
            "  NEW /ROOT\n"
            "         TYPE:=  SITE\n"
            "         \xCE\xBB\xBA\xC5:=  /ROOT\n"
            "END\n"
            "  NEW /ROOT/CHILD\n"
            "         TYPE:=  ZONE\n"
            "         \xD7\xF8\xB1\xEA:=  E 1mm N 2mm U 3mm\n"
            "END\n";
        Buffer  buffer  =   new FEBuffer(ctx);
        buffer->cInfo()._buffer.assign(att.begin(),att.end());

        auto    attReader   =   new FEFormatATTReader(ctx);
        Objects properties  =   attReader->readBuffers({buffer});
        check(properties.size() == 2,"ATT: record count = 2 (header skipped)");
        check(attReader->progress() == real(1),"ATT: progress reaches 1");

        FEProperty* rootProp    =   properties.empty() ? nullptr : properties[0]->cast<FEProperty>();
        check(rootProp != nullptr && rootProp->code() == "/ROOT","ATT: code = NEW name");
        const   auto*   typeVal =   rootProp != nullptr ? rootProp->values().isExist("TYPE") : nullptr;
        check(typeVal != nullptr && std::get<String>(typeVal->value()) == "SITE","ATT: key/value kept");
        /// 键为 GBK 的"位号",读取后应转为 UTF-8
        const   auto*   nameKey =   rootProp != nullptr ? rootProp->values().isExist("\xE4\xBD\x8D\xE5\x8F\xB7") : nullptr;
        check(nameKey != nullptr && std::get<String>(nameKey->value()) == "/ROOT","ATT: GBK -> UTF-8");
        check(properties.size() > 1 && properties[1]->cast<FEProperty>() != nullptr
           && properties[1]->cast<FEProperty>()->code() == "/ROOT/CHILD","ATT: nested record name");

        Node    root    =   new FENode(ctx);
        root->setName("/ROOT");
        Node    child   =   new FENode(ctx);
        child->setName("/ROOT/CHILD");
        root->addChild(child);
        check(FEFormatATTReader::attach(root,properties) == 2,"ATT: attach by node name");
        check(root->objects<FEProperty>().size() == 1
           && child->objects<FEProperty>().size() == 1,"ATT: FEProperty as component");
    }

    scene->destroy();
    scene   =   nullptr;
    pScene  =   nullptr;
    app->destroy();

    printf("=== %d passed, %d failed ===\n",g_pass,g_fail);
    return  g_fail == 0 ? 0 : 1;
}
