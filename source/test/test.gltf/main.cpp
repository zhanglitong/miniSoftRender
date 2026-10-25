#include    <cstdio>
#include    <cstring>
#include    <cmath>
#include    <filesystem>
#include    <string>

#include    "../../inc/FEContext.hpp"
#include    "../../inc/FEFileFormatHelper.hpp"
#include    "../../inc/FEAppHelper.hpp"
#include    "../../inc/graphic/FEScene.h"
#include    "../../inc/stream/FEWriterMemrory.hpp"
#include    "../../inc/mesh/FEMesh.hpp"
#include    "../../inc/mesh/FEDrawElementUint16.hpp"
#include    "../../inc/node/FENode.hpp"
#include    "../../inc/material/FEMaterialPBR.hpp"
/// tiny_gltf 的实现由 Reader 头文件开启,必须放在 Writer 之前
#include    "../../inc/fileFormat/gltf/FEFormatGLTFReader.hpp"
#include    "../../inc/fileFormat/gltf/FEFormatGLTFWriter.hpp"

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
    bool    almostEqual(double a,double b,double eps = 1e-5)
    {
        return  std::fabs(a - b) <= eps;
    }
};

/// ==================== 测试场景 ====================
static  Node    buildScene(FEContext& ctx)
{
    /// 单个三角形网格:位置/法线/UV0 + uint16 索引
    Mesh    mesh    =   new FEMesh(ctx);
    mesh->setAttribute<float>({IS_VERTEX_POS,FMT_R32G32B32_FLOAT},
        {0,0,0, 1,0,0, 0,1,0});
    mesh->setAttribute<float>({IS_VERTEX_NOR,FMT_R32G32B32_FLOAT},
        {0,0,1, 0,0,1, 0,0,1});
    mesh->setAttribute<float>({IS_VERTEX_TEXCOORD0,FMT_R32G32_FLOAT},
        {0,0, 1,0, 0,1});

    auto    pri =   new FEDrawElementUint16(ctx);
    pri->index()    =   uint16s{0,1,2};
    Primitives  primitives;
    primitives.emplace_back(pri);
    mesh->setPrimitives(primitives);
    mesh->updateAabb();

    Node    root    =   new FENode(ctx);
    root->setName("root");
    root->setLocalTranslation(real3(1,2,3));
    root->setLocalScaling(real3(2,2,2));
    /// 绕 Y 轴 90 度,四元数顺序为 (w,x,y,z)
    root->setLocalRotation(quatr(0.7071067811865476,0.0,0.7071067811865476,0.0));

    Node    child   =   new FENode(ctx);
    child->setName("triangle");
    child->setLocalTranslation(real3(4,5,6));
    child->setMesh(mesh.get());
    root->addChild(child);

    return  root;
}
static  FEFileFormat   makeFormat(size_t index)
{
    /// 使用 writer 注册的格式描述符,registry 以 desc[version](ext) 组装 key 查询
    const   auto    formats =   FEFormatGLTFWriter::formatList();
    return  formats[index];
}
/// ==================== 带 PBR 材质的场景(需要引擎) ====================
static  Node    buildMaterialScene(FEContext& ctx)
{
    Mesh    mesh    =   new FEMesh(ctx);
    mesh->setAttribute<float>({IS_VERTEX_POS,FMT_R32G32B32_FLOAT},
        {0,0,0, 1,0,0, 0,1,0});
    mesh->setAttribute<float>({IS_VERTEX_NOR,FMT_R32G32B32_FLOAT},
        {0,0,1, 0,0,1, 0,0,1});
    mesh->setAttribute<float>({IS_VERTEX_TEXCOORD0,FMT_R32G32_FLOAT},
        {0,0, 1,0, 0,1});

    auto    pri =   new FEDrawElementUint16(ctx);
    pri->index()    =   uint16s{0,1,2};
    Primitives  primitives;
    primitives.emplace_back(pri);
    mesh->setPrimitives(primitives);
    mesh->updateAabb();

    /// PBR 材质:基色/金属度/粗糙度 + 4 个 KHR 扩展
    Material    material    =   new FEMaterialPBR(ctx);
    auto&       pbr         =   material->as<FEMaterialPBR>()->data()._value;
    pbr._diffuse    =   float4(0.8f,0.25f,0.1f,1.0f);
    pbr._emissive   =   float4(0.1f,0.2f,0.3f,0.0f);
    pbr._metallic   =   0.4f;
    pbr._roughness  =   0.6f;
    material->as<FEMaterialPBR>()->data().update();

    auto    pbrMat  =   material->as<FEMaterialPBR>();
    pbrMat->setClearcoat(0.5f,0.2f);
    pbrMat->setTransmission(0.75f);
    pbrMat->setVolume(0.3f,float3(0.9f,0.5f,0.4f),2.0f);
    pbrMat->setIridescence(0.6f,1.4f,120.0f,450.0f);

    Node    root    =   new FENode(ctx);
    root->setName("scene_root");
    root->setLocalTranslation(real3(1,2,3));

    Node    child   =   new FENode(ctx);
    child->setName("triangle_mat");
    child->setLocalTranslation(real3(4,5,6));
    child->setLocalScaling(real3(2,2,2));
    child->setMesh(mesh.get());
    child->setMaterial(material.get());
    root->addChild(child);

    return  root;
}
/// ==================== accessor 数据定位 ====================
static  const   uint8*  accessorData(const tinygltf::Model& model,const tinygltf::Accessor& accessor)
{
    const   auto&   view    =   model.bufferViews[size_t(accessor.bufferView)];
    const   auto&   buffer  =   model.buffers[size_t(view.buffer)];
    return  buffer.data.data() + size_t(view.byteOffset) + size_t(accessor.byteOffset);
}
static  bool    loadASCII(tinygltf::Model& model,const String& json)
{
    tinygltf::TinyGLTF  loader;
    std::string         err;
    std::string         warn;
    bool    ok  =   loader.LoadASCIIFromString(&model,&err,&warn,json.c_str(),
                        unsigned(json.size()),"");
    if (!ok)
        printf("       tinygltf error: %s\n",err.c_str());
    return  ok;
}
/// ==================== 内存写出并校验结构 ====================
static  void    testMemory(FEContext& ctx)
{
    Node    root    =   buildScene(ctx);
    Buffer  buffer  =   new FEBuffer(ctx);
    buffer->cInfo()._buffer.resize(1024 * 1024);

    auto    writer  =   FEFileFormatHelper::queryWriter(ctx,makeFormat(0));
    check(writer != nullptr,"memory: query gltf writer");
    if (writer == nullptr)
        return;

    FEWriterMemory  stream(ctx,buffer);
    check(writer->writeBuffer({root.get()},stream),"memory: write buffer");
    const   uint64  written =   stream.tell();
    check(written > 0,"memory: bytes written");
    buffer->cInfo()._buffer.resize(size_t(written));

    const   String  json    =   String((const char*)buffer->data(),size_t(written));
    /// 文本 glTF 应包含节点名与二进制数据内嵌的 data URI
    check(json.find("\"root\"") != String::npos,"memory: node name in json");
    check(json.find("data:application/octet-stream;base64") != String::npos,"memory: embedded binary");

    tinygltf::Model model;
    check(loadASCII(model,json),"memory: parse ascii");
    check(model.nodes.size() == 2,"memory: node count");
    if (model.nodes.size() != 2)
        return;
    check(model.scenes.size() == 1 && model.scenes[0].nodes.size() == 1,"memory: scene roots");
    check(model.defaultScene == 0,"memory: default scene");
    if (model.scenes.empty() || model.scenes[0].nodes.empty())
        return;

    const   auto&   rootNode    =   model.nodes[size_t(model.scenes[0].nodes[0])];
    check(rootNode.name == "root","memory: root name");
    check(rootNode.translation.size() == 3
        && almostEqual(rootNode.translation[0],1) && almostEqual(rootNode.translation[1],2) && almostEqual(rootNode.translation[2],3),
        "memory: root translation");
    check(rootNode.rotation.size() == 4
        && almostEqual(rootNode.rotation[0],0) && almostEqual(rootNode.rotation[1],0.7071067811865476)
        && almostEqual(rootNode.rotation[2],0) && almostEqual(rootNode.rotation[3],0.7071067811865476),
        "memory: root rotation (x,y,z,w)");
    check(rootNode.scale.size() == 3
        && almostEqual(rootNode.scale[0],2) && almostEqual(rootNode.scale[1],2) && almostEqual(rootNode.scale[2],2),
        "memory: root scale");
    check(rootNode.children.size() == 1,"memory: root children");
    check(rootNode.mesh == -1,"memory: root has no mesh");
    if (rootNode.children.size() != 1)
        return;

    const   auto&   childNode   =   model.nodes[size_t(rootNode.children[0])];
    check(childNode.name == "triangle","memory: child name");
    check(childNode.translation.size() == 3
        && almostEqual(childNode.translation[0],4) && almostEqual(childNode.translation[1],5) && almostEqual(childNode.translation[2],6),
        "memory: child translation");
    check(childNode.rotation.empty() && childNode.scale.empty(),"memory: default trs omitted");
    check(childNode.mesh >= 0,"memory: child mesh index");
    if (childNode.mesh < 0)
        return;

    const   auto&   mesh    =   model.meshes[size_t(childNode.mesh)];
    check(mesh.primitives.size() == 1,"memory: primitive count");
    if (mesh.primitives.size() != 1)
        return;
    const   auto&   pri     =   mesh.primitives[0];
    check(pri.attributes.size() == 3,"memory: attribute count");
    check(pri.attributes.count("POSITION") == 1
        && pri.attributes.count("NORMAL") == 1
        && pri.attributes.count("TEXCOORD_0") == 1,"memory: attribute semantics");
    check(pri.mode == TINYGLTF_MODE_TRIANGLES,"memory: primitive mode");
    check(pri.indices >= 0,"memory: indices accessor");
    if (pri.attributes.count("POSITION") == 0 || pri.indices < 0)
        return;

    const   auto&   pos     =   model.accessors[size_t(pri.attributes.at("POSITION"))];
    check(pos.count == 3,"memory: position count");
    check(pos.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT
        && pos.type == TINYGLTF_TYPE_VEC3,"memory: position format");
    check(pos.minValues.size() == 3
        && almostEqual(pos.minValues[0],0) && almostEqual(pos.minValues[1],0) && almostEqual(pos.minValues[2],0),
        "memory: position min");
    check(pos.maxValues.size() == 3
        && almostEqual(pos.maxValues[0],1) && almostEqual(pos.maxValues[1],1) && almostEqual(pos.maxValues[2],0),
        "memory: position max");

    const   float*  posData =   (const float*)accessorData(model,pos);
    check(almostEqual(posData[0],0) && almostEqual(posData[1],0) && almostEqual(posData[2],0)
        && almostEqual(posData[3],1) && almostEqual(posData[4],0) && almostEqual(posData[5],0)
        && almostEqual(posData[6],0) && almostEqual(posData[7],1) && almostEqual(posData[8],0),
        "memory: position data");

    const   auto&   idx     =   model.accessors[size_t(pri.indices)];
    check(idx.count == 3 && idx.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT,
        "memory: index format");
    const   uint16* idxData =   (const uint16*)accessorData(model,idx);
    check(idxData[0] == 0 && idxData[1] == 1 && idxData[2] == 2,"memory: index data");
}
/// ==================== 文件往返 ====================
static  void    testFile(FEContext& ctx)
{
    namespace   fs  =   std::filesystem;
    const   fs::path    textPath    =   fs::temp_directory_path() / "fe_test_scene.gltf";
    const   fs::path    binPath     =   fs::temp_directory_path() / "fe_test_scene.glb";

    Node    root    =   buildScene(ctx);

    /// .gltf 文本
    auto    writer  =   FEFileFormatHelper::queryWriter(ctx,makeFormat(0));
    check(writer != nullptr,"file: query gltf writer");
    if (writer != nullptr)
    {
        check(writer->writeFile({root.get()},textPath.string()),"file: write gltf");
        check(fs::exists(textPath) && fs::file_size(textPath) > 0,"file: gltf exists");
        printf("       .gltf size = %llu bytes\n",(unsigned long long)fs::file_size(textPath));

        tinygltf::Model     model;
        tinygltf::TinyGLTF  loader;
        std::string         err;
        std::string         warn;
        bool    ok  =   loader.LoadASCIIFromFile(&model,&err,&warn,textPath.string());
        if (!ok)
            printf("       tinygltf error: %s\n",err.c_str());
        check(ok,"file: parse gltf");
        check(model.nodes.size() == 2,"file: gltf node count");
        if (model.nodes.size() == 2 && !model.meshes.empty() && !model.meshes[0].primitives.empty())
            check(model.meshes[0].primitives[0].attributes.size() == 3,"file: gltf attributes");
        fs::remove(textPath);
    }

    /// .glb 二进制
    writer  =   FEFileFormatHelper::queryWriter(ctx,makeFormat(1));
    check(writer != nullptr,"file: query glb writer");
    if (writer != nullptr)
    {
        check(writer->writeFile({root.get()},binPath.string()),"file: write glb");
        check(fs::exists(binPath) && fs::file_size(binPath) > 0,"file: glb exists");
        printf("       .glb size = %llu bytes\n",(unsigned long long)fs::file_size(binPath));

        tinygltf::Model     model;
        tinygltf::TinyGLTF  loader;
        std::string         err;
        std::string         warn;
        bool    ok  =   loader.LoadBinaryFromFile(&model,&err,&warn,binPath.string());
        if (!ok)
            printf("       tinygltf error: %s\n",err.c_str());
        check(ok,"file: parse glb");
        check(model.nodes.size() == 2,"file: glb node count");
        check(!model.buffers.empty() && !model.buffers[0].data.empty(),"file: glb binary data");
        fs::remove(binPath);
    }
}

/// ==================== 引擎回读(材质 + 场景) ====================
static  void    testEngineRoundTrip(FEContext& ctx)
{
    namespace   fs  =   std::filesystem;
    const   fs::path    path    =   fs::temp_directory_path() / "fe_test_engine.gltf";

    Node    root    =   buildMaterialScene(ctx);
    auto    writer  =   FEFileFormatHelper::queryWriter(ctx,makeFormat(0));
    check(writer != nullptr,"engine: query gltf writer");
    if (writer == nullptr)
        return;
    check(writer->writeFile({root.get()},path.string()),"engine: write gltf");

    auto    reader  =   FEFileFormatHelper::queryReader(ctx,makeFormat(0));
    check(reader != nullptr,"engine: query gltf reader");
    if (reader == nullptr)
        return;

    Objects objects =   reader->readFiles({path.string()});
    check(objects.size() == 1,"engine: object count");
    Node    loaded  =   objects.size() == 1 ? objects[0]->cast<FENode>() : nullptr;
    check(loaded != nullptr,"engine: root is node");
    if (loaded != nullptr)
    {
        check(strcmp(loaded->name(),"scene_root") == 0,"engine: root name");
        const   auto    pos =   loaded->localTranslation();
        check(almostEqual(double(pos.x),1) && almostEqual(double(pos.y),2) && almostEqual(double(pos.z),3),
            "engine: root translation");
        check(loaded->material() == nullptr,"engine: root has no material");
        check(loaded->children().size() == 1,"engine: child count");

        if (loaded->children().size() == 1)
        {
            const   Node&   child   =   loaded->children()[0];
            check(strcmp(child->name(),"triangle_mat") == 0,"engine: child name");
            const   auto    cpos    =   child->localTranslation();
            check(almostEqual(double(cpos.x),4) && almostEqual(double(cpos.y),5) && almostEqual(double(cpos.z),6),
                "engine: child translation");
            check(almostEqual(double(child->localScaling().x),2),"engine: child scale");
            check(child->mesh() != nullptr,"engine: child mesh");
            if (child->mesh() != nullptr)
            {
                check(child->mesh()->buffers().size() == 3,"engine: child attributes");
                check(child->mesh()->primitives().size() == 1,"engine: child primitives");
                if (child->mesh()->primitives().size() == 1)
                {
                    auto    pri =   child->mesh()->primitives()[0]->cast<FEDrawElementUint16>();
                    check(pri != nullptr,"engine: index primitive");
                    if (pri != nullptr)
                        check(pri->index().size() == 3,"engine: index count");
                }
            }
            check(child->material() != nullptr,"engine: child material");
            if (child->material() != nullptr)
            {
                auto    pbr =   child->material()->cast<FEMaterialPBR>();
                check(pbr != nullptr,"engine: pbr material");
                if (pbr != nullptr)
                {
                    const   auto&   value   =   pbr->data()._value;
                    const   auto&   ext     =   pbr->texParams()._value;
                    check(almostEqual(double(value._diffuse.x),0.8)
                        && almostEqual(double(value._diffuse.y),0.25)
                        && almostEqual(double(value._diffuse.z),0.1),"engine: base color");
                    check(almostEqual(double(value._metallic),0.4),"engine: metallic");
                    check(almostEqual(double(value._roughness),0.6),"engine: roughness");
                    check(almostEqual(double(value._emissive.x),0.1)
                        && almostEqual(double(value._emissive.z),0.3),"engine: emissive");
                    check(almostEqual(double(ext._clearcoatStrength),0.5)
                        && almostEqual(double(ext._clearcoatRoughness),0.2),"engine: clearcoat ext");
                    check(almostEqual(double(ext._transmissionFactor),0.75),"engine: transmission ext");
                    check(almostEqual(double(ext._volume[0]),0.3)
                        && almostEqual(double(ext._volume[1]),0.9)
                        && almostEqual(double(ext._volume[4]),2.0),"engine: volume ext");
                    check(almostEqual(double(ext._iriFactor),0.6)
                        && almostEqual(double(ext._iriIor),1.4),"engine: iridescence ext");
                }
            }
        }
    }
    fs::remove(path);
}

int     main(int argc,char** argv)
{
    UNUSED(argc,argv);
    setvbuf(stdout,nullptr,_IONBF,0);
    FEContext&  ctx =   FEContext::instance();

    printf("=== test.gltf: gltf writer round-trip ===\n");
    testMemory(ctx);
    testFile(ctx);

    /// 材质创建需要显卡资源,因此先创建窗口与场景(不进入渲染循环)
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
    if (app != nullptr)
    {
        ctx.setWindow(app.get());
        ctx.setWorkPath(app->path());
        ctx.setResourcePath(app->path() + "/../");

        Scene   scene   =   new FEScene(ctx);
        pScene          =   scene.get();
        const   bool    setupOk =   scene->setup(app,RS_VULKAN);
        check(setupOk,"engine: scene setup");
        if (setupOk)
        {
            printf("--- engine assets initialized ---\n");
            testEngineRoundTrip(ctx);
        }
        /// 退出前显式卸载场景:静态析构阶段再销毁场景会访问已释放的全局对象
        scene->destroy();
        scene   =   nullptr;
        pScene  =   nullptr;
        app->destroy();
    }
    pScene  =   nullptr;

    printf("=== %d passed, %d failed ===\n",g_pass,g_fail);

    return  g_fail == 0 ? 0 : 1;
}


