#include    <cmath>
#include    <cstdio>
#include    <cstring>
#include    <filesystem>
#include    <string>

#include    "../../inc/FEContext.hpp"
#include    "../../inc/FEFileFormatHelper.hpp"
#include    "../../inc/fileFormat/xml/FESkeletonXml.hpp"

using   namespace   FE;

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
    bool    realNear(real a,real b,real eps = 1e-4)
    {
        return  std::abs(a - b) <= eps;
    }
    bool    matNear(const mat4r& a,const mat4r& b,real eps = 1e-4)
    {
        for (int col = 0;col < 4;++col)
        {
            for (int row = 0;row < 4;++row)
            {
                if (!realNear(a[col][row],b[col][row],eps))
                    return  false;
            }
        }
        return  true;
    }
    FESkeletonXml::Skeleton    makeSkeleton()
    {
        FESkeletonXml::Skeleton skel;
        /// 非单位 worldRotation,验证骨骼数据的朝向校正能往返
        skel.worldRotation  =   mat4r(glm::rotate(mat4r(1.0),glm::radians(real(90)),real3(1,0,0)));

        FESkeletonXml::JointDesc    root;
        root.name           =   "Root";
        root.parent         =   -1;

        FESkeletonXml::JointDesc    hips;
        hips.name           =   "Hips";
        hips.parent         =   0;
        hips.translation    =   real3(0,0,real(0.95));

        FESkeletonXml::JointDesc    chest;
        chest.name          =   "Chest";
        chest.parent        =   1;
        chest.translation   =   real3(0,0,real(0.5));
        chest.scaling       =   real3(real(1),real(1),real(2));

        skel.joints =   {root,hips,chest};
        skel.bindings.push_back({"Soldier","data/model/Soldier.glb"});
        return  skel;
    }
    FENode* findNode(FENode* node,const char* name)
    {
        if (node == nullptr)
            return  nullptr;
        if (node->name() != nullptr && std::strcmp(node->name(),name) == 0)
            return  node;
        for (auto& child : node->children())
        {
            FENode* found   =   findNode(child.get(),name);
            if (found != nullptr)
                return  found;
        }
        return  nullptr;
    }
    FEFileFormat    makeFormat()
    {
        FEFileFormat    fmt(".fepj","1.0.0.0","FE Buildin Format!");
        fmt._type   =   FEFileFormat::DT_Model;
        fmt._mode   =   FEFileFormat::SM_FILE|FEFileFormat::SM_MEMORY;
        return  fmt;
    }
};

/// ==================== 含蒙皮场景的 .fepj 往返 ====================
/// 覆盖:FESkin / FESkinData 写入 fepj、共享骨骼数据去重、
/// 关节引用(节点 id)在节点树读完后由 resolvePending 回填。
static  void    testSkinFepjRoundTrip(FEContext& ctx)
{
    namespace   fs  =   std::filesystem;
    const   auto    skel    =   makeSkeleton();

    FESkeletonXml::Instance inst;
    String  err;
    if (!FESkeletonXml::build(ctx,skel,inst,&err))
    {
        check(false,"fepj: build skeleton");
        return;
    }
    if (!err.empty())
        printf("       err: %s\n",err.c_str());

    Node    sceneRoot   =   new FENode(ctx);
    sceneRoot->setName("scene_root");
    for (auto& root : inst.roots)
        sceneRoot->addChild(root);

    Node    meshNode    =   new FENode(ctx);
    meshNode->setName("Soldier");
    sceneRoot->addChild(meshNode);

    FESkin* skin    =   FESkeletonXml::bind(sceneRoot.get(),"Soldier",inst.skin);
    check(skin != nullptr,"fepj: bind skin to mesh node");
    if (skin == nullptr)
        return;
    skin->setCpuSkin(true);
    check(meshNode->objects<FESkin>().size() == 1,"fepj: component attached");

    const   fs::path    path    =   fs::temp_directory_path() / "fe_test_skin.fepj";
    FEFileFormat    fmt         =   makeFormat();
    auto            writerFmt   =   FEFileFormatHelper::queryWriter(ctx,fmt);
    check(writerFmt != nullptr,"fepj: query writer");
    if (writerFmt == nullptr)
        return;
    check(writerFmt->writeFile({sceneRoot.get()},path.string()),"fepj: write file");
    check(fs::exists(path),"fepj: file exists");
    if (!fs::exists(path))
        return;
    printf("       file size = %llu bytes\n",(unsigned long long)fs::file_size(path));

    auto    readerFmt   =   FEFileFormatHelper::queryReader(ctx,fmt);
    check(readerFmt != nullptr,"fepj: query reader");
    if (readerFmt == nullptr)
        return;
    Objects objects =   readerFmt->readFiles({path.string()});
    check(objects.size() == 1,"fepj: object count");
    if (objects.size() != 1)
        return;

    Node    loadedRoot  =   objects[0]->cast<FENode>();
    check(loadedRoot != nullptr,"fepj: root is node");
    if (loadedRoot == nullptr)
        return;

    Nodes   roots   =   {loadedRoot};
    FENode* loadedMesh  =   findNode(loadedRoot.get(),"Soldier");
    check(loadedMesh != nullptr,"fepj: mesh node loaded");
    if (loadedMesh == nullptr)
        return;

    auto    loadedSkins =   loadedMesh->objects<FESkin>();
    check(loadedSkins.size() == 1,"fepj: skin component loaded");
    if (loadedSkins.empty())
        return;
    FESkin* loadedSkin  =   loadedSkins.front();
    check(loadedSkin->cpuSkin(),"fepj: cpu skin flag");
    check(loadedSkin->data() != nullptr,"fepj: skin data loaded");
    if (loadedSkin->data() == nullptr)
        return;

    /// 关节引用在蒙皮数据之后才读到,必须由 resolvePending 回填
    check(loadedSkin->data()->pendingJointCount() == inst.joints.size(),"fepj: joints pending before resolve");
    const   size_t  resolved    =   FESkinData::resolvePending(roots);
    check(resolved == inst.joints.size(),"fepj: joints resolved");
    check(loadedSkin->data()->pendingJointCount() == 0,"fepj: no pending joints");

    auto&   bones   =   loadedSkin->data()->bones();
    check(bones.size() == 3,"fepj: bone count");
    if (bones.size() != 3)
        return;
    for (size_t i = 0;i < bones.size();++i)
    {
        check(bones[i].node != nullptr,"fepj: bone node bound");
        if (bones[i].node == nullptr)
            continue;
        check(std::strcmp(bones[i].node->name(),skel.joints[i].name.c_str()) == 0,"fepj: bone node name");
        check(matNear(bones[i].inverseBind,inst.skin->bones()[i].inverseBind),"fepj: inverse bind");
    }
    check(matNear(loadedSkin->data()->worldRotation(),skel.worldRotation),"fepj: world rotation");

    fs::remove(path);
}

int     main(int argc,char** argv)
{
    UNUSED(argc,argv);
    setvbuf(stdout,nullptr,_IONBF,0);
    FEContext&      ctx =   FEContext::instance();

    printf("=== test.skin.fepj: skin skeleton fepj round-trip ===\n");
    testSkinFepjRoundTrip(ctx);
    printf("=== %d passed, %d failed ===\n",g_pass,g_fail);

    return  g_fail == 0 ? 0 : 1;
}
