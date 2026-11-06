
#include    <cstdio>
#include    "../../inc/FEContext.hpp"
#include    "../../inc/node/FENodeTree.hpp"
#include    "../../inc/animation/FEAnimationSys.hpp"
#include    "../../inc/mesh/FEMesh.hpp"
#include    "../../inc/math/FERay.hpp"

using   namespace   FE;

namespace
{
    int     gFailures   =   0;

    void    check(bool ok,const String& msg,int line)
    {
        if (!ok)
        {
            std::printf("[FAIL] %s (main.cpp:%d)\n",msg.c_str(),line);
            ++gFailures;
        }
    }

    bool    nearPoint(const real3& value,const real3& expect)
    {
        return  FE::distance2(value,expect) < 1e-12;
    }

    /// <summary>
    /// DrawArray(PRI_POINTS) 网格:顶点直接作为待拾取的点
    /// </summary>
    Mesh    makeArrayPointMesh(FEContext& ctx,const float3s& poss)
    {
        Mesh    mesh    =   new FEMesh(ctx);
        mesh->setAttribute(FEAttribute(IS_VERTEX_POS,FMT_R32G32B32_FLOAT),poss);

        auto*   pDraw   =   new FEDrawArray(ctx);
        pDraw->setPrimitive(PRI_POINTS);
        pDraw->setStart(0);
        pDraw->setCount(uint32(poss.size()));

        Primitive   pri =   pDraw;
        mesh->setPrimitives({pri});
        return  mesh;
    }

    /// <summary>
    /// 索引图元(PRI_POINTS) 网格:通过索引挑出部分顶点参与拾取
    /// </summary>
    template<class TDRAW,class TINDEX>
    Mesh    makeIndexedPointMesh(FEContext& ctx,const float3s& poss,const std::vector<TINDEX>& index)
    {
        Mesh    mesh    =   new FEMesh(ctx);
        mesh->setAttribute(FEAttribute(IS_VERTEX_POS,FMT_R32G32B32_FLOAT),poss);

        auto*   pDraw   =   new TDRAW(ctx);
        pDraw->setPrimitive(PRI_POINTS);
        pDraw->index()  =   index;

        Primitive   pri =   pDraw;
        mesh->setPrimitives({pri});
        return  mesh;
    }
}

#define CHECK(cond,msg) check((cond),(msg),__LINE__)

/// <summary>
/// 节点树:挂接、防环、提升为顶层、移除
/// </summary>
static void testNodeTree(FEContext& ctx)
{
    FENodeTree  tree(ctx);
    Node        a   =   new FENode(ctx);
    Node        b   =   new FENode(ctx);
    Node        c   =   new FENode(ctx);

    tree.addToplevelNode(a);
    tree.addToplevelNode(b);
    CHECK(tree.topLevelNodes().size() == 2,"顶层节点应为 2");

    /// 正常挂接:子节点从顶层集合移除,父节点记录子节点
    CHECK(tree.setParent(b,a.get()),"b 挂到 a 应成功");
    CHECK(b->parent().get() == a.get(),"b 的父节点应为 a");
    CHECK(a->children().size() == 1,"a 应有 1 个子节点");
    CHECK(tree.topLevelNodes().size() == 1,"b 应已从顶层集合移除");

    /// 防环:不能挂到自身,也不能挂到自己的子孙
    CHECK(!tree.setParent(a,a.get()),"挂到自身应失败");
    CHECK(!tree.setParent(a,b.get()),"挂到自己的子孙应失败");
    CHECK(b->parent().get() == a.get(),"失败的挂接不应改变父子关系");

    /// 提升为顶层:重新登记到顶层集合
    CHECK(tree.setParent(b,nullptr),"b 提升为顶层应成功");
    CHECK(b->parent() == nullptr,"b 的父节点应清空");
    CHECK(a->children().empty(),"a 的子节点应被摘除");
    CHECK(tree.topLevelNodes().size() == 2,"b 应回到顶层集合");

    /// 多层挂接与跨层级重挂
    CHECK(tree.setParent(c,b.get()),"c 挂到 b 应成功");
    CHECK(tree.setParent(b,a.get()),"b 挂到 a 应成功");
    CHECK(c->parent().get() == b.get(),"c 的父节点应为 b");
    CHECK(tree.setParent(c,a.get()),"c 重挂到 a 应成功");
    CHECK(c->parent().get() == a.get(),"c 的父节点应为 a");
    CHECK(b->children().empty(),"c 应从 b 的子列表中摘除");
    CHECK(a->children().size() == 2,"a 应有 2 个子节点");

    /// 清空:顶层节点与子节点全部释放
    tree.clear();
    CHECK(tree.topLevelNodes().empty(),"清空后顶层集合应为空");
    CHECK(a->children().empty(),"清空后 a 的子节点应为空");
}

/// <summary>
/// 动画系统:播放器创建/命名去重、动画归属与移动、移除
/// </summary>
static void testAnimationSys(FEContext& ctx)
{
    FEAnimationSys  sys(ctx);
    Node            a   =   new FENode(ctx);

    /// getOrCreate 同名返回同一个播放器
    Playback        def     =   sys.getOrCreate();
    CHECK(def != nullptr,"默认播放器应创建成功");
    CHECK(def->name() == "default","默认播放器名应为 default");
    CHECK(sys.getOrCreate("default") == def,"同名播放器应复用");
    CHECK(sys.playbacks().size() == 1,"应只有 1 个播放器");

    /// addPlayback 同名自动加后缀,重复登记返回 0
    Playback        extra   =   new FEPlayback(ctx);
    extra->setName("default");
    CHECK(sys.addPlayback(extra) == 1,"同名播放器应登记成功");
    CHECK(extra->name() == "default1","同名播放器应自动加后缀");
    CHECK(sys.addPlayback(extra) == 0,"重复登记应失败");
    CHECK(sys.playbacks().size() == 2,"应有 2 个播放器");

    /// 动画登记到指定播放器
    Animation       anim    =   new FEAnimation(ctx);
    anim->attach(Object(a.get()));
    CHECK(sys.addObject(Component(anim.get()),"clipA") == 1,"动画应登记到 clipA");
    CHECK(sys.animationOf(Object(a.get())).get() == anim.get(),"应能按 owner 找到动画");
    CHECK(sys.addObject(Component(anim.get()),"clipA") == 0,"重复登记应失败");
    CHECK(sys.playbacks().find("clipA") != sys.playbacks().end(),"clipA 播放器应存在");
    CHECK(sys.playbacks().find("clipA")->second->objects().size() == 1,"clipA 应持有 1 个动画");

    /// 移动:动画只归属一个播放器,登记到新播放器时从原播放器摘除
    CHECK(sys.addObject(Component(anim.get()),"clipB") == 1,"动画应移动到 clipB");
    CHECK(sys.playbacks().find("clipA")->second->objects().empty(),"clipA 应已释放动画");
    CHECK(sys.playbacks().find("clipB")->second->objects().size() == 1,"clipB 应持有 1 个动画");
    CHECK(sys.animationOf(Object(a.get())).get() == anim.get(),"移动后仍能按 owner 找到动画");

    /// 移除
    CHECK(sys.removeObject(Component(anim.get())) == 1,"移除动画应成功");
    CHECK(sys.animationOf(Object(a.get())) == nullptr,"移除后不应再找到动画");
}

/// <summary>
/// 索引图元点拾取:顶点集中索引 3 最靠近射线,索引 2 超出阈值,索引 4 位于射线后方
/// </summary>
template<class TDRAW,class TINDEX>
void    testIndexedPointPickup(FEContext& ctx,const String& name)
{
    const   mat4r               mat     =   mat4r(real(1));
    const   Ray                 ray     =   Ray(real3(0,0,0),real3(0,0,1));
    const   float3s             poss    =   { float3(0,0,9),float3(0.5f,0,9),float3(3,0,9),float3(0,0,3),float3(0,0,-3) };
    const   std::vector<TINDEX> all     =   { 0,1,2,3,4 };
    const   std::vector<TINDEX> subset  =   { 0,1,2 };

    /// 全部索引:最近的 (0,0,3) 命中,超阈值与射线后方的点被过滤
    {
        Mesh        mesh    =   makeIndexedPointMesh<TDRAW>(ctx,poss,all);
        FEPickup    result;
        CHECK(mesh->intersect(ray,mat,result),name + " 点拾取应命中");
        CHECK(result.object.get() == mesh.get(),name + " 拾取对象应为当前 mesh");
        CHECK(result.pickup == FEPickup::PR_Point,name + " 拾取类型应为 PR_Point");
        CHECK(nearPoint(result.point,real3(0,0,3)),name + " 应拾取垂距最小的点 (0,0,3)");
        CHECK(std::abs(result.time - 3.0) < 1e-9,name + " 射线参数 t 应为 3");
        CHECK(std::abs(result.distance - 3.0) < 1e-9,name + " 到拾取点距离应为 3");
    }

    /// 子集索引:未被索引的最近点 (0,0,3) 不参与拾取,命中 (0,0,9)
    {
        Mesh        mesh    =   makeIndexedPointMesh<TDRAW>(ctx,poss,subset);
        FEPickup    result;
        CHECK(mesh->intersect(ray,mat,result),name + " 子集索引应命中");
        CHECK(nearPoint(result.point,real3(0,0,9)),name + " 未索引的顶点不应参与拾取");
        CHECK(std::abs(result.time - 9.0) < 1e-9,name + " 子集索引射线参数 t 应为 9");
    }
}

/// <summary>
/// 网格点拾取:FEMesh::intersect 的 PRI_POINTS 分支(DrawArray 与三种索引图元)
/// </summary>
static void testMeshPointPickup(FEContext& ctx)
{
    const   mat4r   mat     =   mat4r(real(1));
    const   Ray     ray     =   Ray(real3(0,0,0),real3(0,0,1));

    /// 用例 1:DrawArray — 命中垂距最小且在射线前方的点
    {
        float3s     poss    =   { float3(0,0,5),float3(1.5f,0,5),float3(2.5f,0,5),float3(0,0,-5) };
        Mesh        mesh    =   makeArrayPointMesh(ctx,poss);
        FEPickup    result;

        CHECK(mesh->intersect(ray,mat,result),"DrawArray 点拾取应命中");
        CHECK(result.object.get() == mesh.get(),"拾取对象应为当前 mesh");
        CHECK(result.pickup == FEPickup::PR_Point,"拾取类型应为 PR_Point");
        CHECK(nearPoint(result.point,real3(0,0,5)),"应拾取垂距最小的点 (0,0,5)");
        CHECK(std::abs(result.time - 5.0) < 1e-9,"射线参数 t 应为 5");
        CHECK(std::abs(result.distance - 5.0) < 1e-9,"到拾取点距离应为 5");

        /// 非单位方向:射线参数 t 改变,拾取点不变
        Ray         ray2    =   Ray(real3(0,0,0),real3(0,0,2));
        FEPickup    result2;
        CHECK(mesh->intersect(ray2,mat,result2),"非单位方向射线也应命中");
        CHECK(nearPoint(result2.point,real3(0,0,5)),"非单位方向拾取点应为 (0,0,5)");
        CHECK(std::abs(result2.time - 2.5) < 1e-9,"非单位方向射线参数 t 应为 2.5");
    }

    /// 用例 2:候选点全部超出拾取半径或位于射线后方,不应命中
    {
        float3s     far     =   { float3(3,0,5),float3(0,3,5) };
        Mesh        mesh    =   makeArrayPointMesh(ctx,far);
        FEPickup    result;
        CHECK(!mesh->intersect(ray,mat,result),"垂距超出阈值的点不应命中");

        float3s     behind  =   { float3(0,0,-5) };
        Mesh        mesh2   =   makeArrayPointMesh(ctx,behind);
        FEPickup    result2;
        CHECK(!mesh2->intersect(ray,mat,result2),"射线后方的点不应命中");
    }

    /// 用例 3:三种索引格式
    testIndexedPointPickup<FEDrawElementUint8, uint8 >(ctx,"uint8 索引");
    testIndexedPointPickup<FEDrawElementUint16,uint16>(ctx,"uint16 索引");
    testIndexedPointPickup<FEDrawElementUint32,uint32>(ctx,"uint32 索引");
}

int     main(int,char**)
{
    FEContext&  ctx =   FEContext::instance();

    testNodeTree(ctx);
    testAnimationSys(ctx);
    testMeshPointPickup(ctx);

    if (gFailures == 0)
        std::printf("[PASS] test.core\n");
    else
        std::printf("[FAIL] test.core: %d check(s) failed\n",gFailures);
    return  gFailures == 0 ? 0 : 1;
}
