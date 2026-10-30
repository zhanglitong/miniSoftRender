
#include    <cstdio>
#include    "../../inc/FEContext.hpp"
#include    "../../inc/node/FENodeTree.hpp"
#include    "../../inc/animation/FEAnimationSys.hpp"

using   namespace   FE;

namespace
{
    int     gFailures   =   0;

    void    check(bool ok,const char* msg,int line)
    {
        if (!ok)
        {
            std::printf("[FAIL] %s (main.cpp:%d)\n",msg,line);
            ++gFailures;
        }
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

int     main(int,char**)
{
    FEContext&  ctx =   FEContext::instance();

    testNodeTree(ctx);
    testAnimationSys(ctx);

    if (gFailures == 0)
        std::printf("[PASS] test.core\n");
    else
        std::printf("[FAIL] test.core: %d check(s) failed\n",gFailures);
    return  gFailures == 0 ? 0 : 1;
}
