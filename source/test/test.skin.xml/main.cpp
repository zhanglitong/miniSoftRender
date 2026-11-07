#include    <cmath>
#include    <cstdio>
#include    <cstring>
#include    <filesystem>
#include    <string>

#include    "../../inc/FEContext.hpp"
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
};

/// ==================== 骨骼 XML 读写往返 ====================
static  void    testSkeletonXmlRoundTrip()
{
    namespace   fs  =   std::filesystem;
    const   fs::path    path    =   fs::temp_directory_path() / "fe_test_skeleton.xml";
    const   auto        skel    =   makeSkeleton();

    String  err;
    check(FESkeletonXml::save(skel,path.string(),&err),"xml: save");
    if (!err.empty())
        printf("       err: %s\n",err.c_str());
    check(fs::exists(path),"xml: file exists");

    FESkeletonXml::Skeleton loaded;
    check(FESkeletonXml::load(path.string(),loaded,&err),"xml: load");
    if (!err.empty())
        printf("       err: %s\n",err.c_str());
    check(loaded.joints.size() == 3,"xml: joint count");
    check(loaded.bindings.size() == 1,"xml: binding count");
    if (loaded.joints.size() == 3)
    {
        check(loaded.joints[0].name == "Root" && loaded.joints[0].parent == -1,"xml: joint0");
        check(loaded.joints[1].name == "Hips" && loaded.joints[1].parent == 0,"xml: joint1");
        check(loaded.joints[2].name == "Chest" && loaded.joints[2].parent == 1,"xml: joint2");
        check(realNear(loaded.joints[1].translation.z,real(0.95)),"xml: joint1 translation");
        check(realNear(loaded.joints[2].scaling.z,real(2.0)),"xml: joint2 scaling");
        check(std::abs((real)glm::dot(loaded.joints[0].rotation,quatr(1.0,0.0,0.0,0.0))) >= 1.0 - 1e-5,
            "xml: joint0 rotation");
    }
    check(matNear(loaded.worldRotation,skel.worldRotation),"xml: world rotation");
    if (loaded.bindings.size() == 1)
    {
        check(loaded.bindings[0].node == "Soldier","xml: binding node");
        check(loaded.bindings[0].mesh == "data/model/Soldier.glb","xml: binding mesh");
    }

    /// 不存在的文件 / 非法内容都要报错,而不是静默返回空骨骼
    FESkeletonXml::Skeleton bad;
    err.clear();
    check(!FESkeletonXml::load((fs::temp_directory_path() / "fe_test_skeleton_missing.xml").string(),bad,&err),
        "xml: missing file fails");
    check(!err.empty(),"xml: missing file reports error");

    const   fs::path    badPath =   fs::temp_directory_path() / "fe_test_skeleton_bad.xml";
    {
        std::FILE*  file    =   std::fopen(badPath.string().c_str(),"wb");
        const   char*   content =   "<skeleton><joints></joints></skeleton>";
        std::fwrite(content,1,std::strlen(content),file);
        std::fclose(file);
    }
    err.clear();
    check(!FESkeletonXml::load(badPath.string(),bad,&err),"xml: empty joints fails");
    check(!err.empty(),"xml: empty joints reports error");

    fs::remove(path);
    fs::remove(badPath);
}

/// ==================== 由骨骼 XML 构建节点与蒙皮数据 ====================
static  void    testSkeletonBuild(FEContext& ctx)
{
    const   auto    skel    =   makeSkeleton();
    FESkeletonXml::Instance inst;
    String  err;
    check(FESkeletonXml::build(ctx,skel,inst,&err),"build: success");
    if (!err.empty())
        printf("       err: %s\n",err.c_str());
    check(inst.joints.size() == 3,"build: joint nodes");
    check(inst.roots.size() == 1,"build: single root");
    check(inst.skin != nullptr,"build: skin data");
    if (inst.joints.size() != 3 || inst.skin == nullptr || inst.roots.size() != 1)
        return;

    check(std::strcmp(inst.joints[0]->name(),"Root") == 0,"build: joint0 name");
    check(std::strcmp(inst.joints[2]->name(),"Chest") == 0,"build: joint2 name");
    check(inst.joints[1]->parent() == inst.joints[0],"build: joint1 parent");
    check(inst.joints[2]->parent() == inst.joints[1],"build: joint2 parent");
    check(inst.roots[0] == inst.joints[0],"build: root node");

    check(inst.skin->jointCount() == 3,"build: joint count");
    check(matNear(inst.skin->worldRotation(),skel.worldRotation),"build: world rotation");
    /// 绑定姿态:全局平移应为局部平移累计
    check(realNear(inst.joints[2]->globalTransform()[3][2],real(1.45)),"build: bind global z");
    /// inverseBind 必须是绑定姿态全局变换的逆
    auto&   bones   =   inst.skin->bones();
    for (size_t i = 0;i < bones.size();++i)
    {
        char    label[64]   =   {0};
        std::snprintf(label,sizeof(label),"build: inverse bind %zu",i);
        const   mat4r   identity    =   bones[i].inverseBind * inst.joints[i]->globalTransform();
        check(matNear(identity,mat4r(1.0)),label);
    }

    /// 导出:从运行中的骨骼数据写回 XML,内容应与源描述一致
    namespace   fs  =   std::filesystem;
    const   fs::path    path    =   fs::temp_directory_path() / "fe_test_skeleton_export.xml";
    check(FESkeletonXml::save(*inst.skin,skel.bindings,path.string(),&err),"export: save");
    FESkeletonXml::Skeleton exported;
    check(FESkeletonXml::load(path.string(),exported,&err),"export: load");
    check(exported.joints.size() == 3,"export: joint count");
    if (exported.joints.size() == 3)
    {
        check(exported.joints[0].name == "Root" && exported.joints[0].parent == -1,"export: joint0");
        check(exported.joints[1].name == "Hips" && exported.joints[1].parent == 0,"export: joint1");
        check(exported.joints[2].name == "Chest" && exported.joints[2].parent == 1,"export: joint2");
        check(realNear(exported.joints[2].scaling.z,real(2.0)),"export: joint2 scaling");
    }
    check(exported.bindings.size() == 1 && exported.bindings[0].node == "Soldier","export: binding");
    check(matNear(exported.worldRotation,skel.worldRotation),"export: world rotation");
    fs::remove(path);
}

int     main(int argc,char** argv)
{
    UNUSED(argc,argv);
    setvbuf(stdout,nullptr,_IONBF,0);
    FEContext&      ctx =   FEContext::instance();

    printf("=== test.skin.xml: skeleton xml load/save + build ===\n");
    testSkeletonXmlRoundTrip();
    testSkeletonBuild(ctx);
    printf("=== %d passed, %d failed ===\n",g_pass,g_fail);

    return  g_fail == 0 ? 0 : 1;
}
