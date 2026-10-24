/// test.constraint
/// 约束系统测试(不依赖渲染后端,纯 CPU 求解):
///   1. Follow: 附加时记录偏移,目标移动后按偏移跟随
///   2. Trans : maintainOffset=false 时吸附到目标点
///   3. LookAt: 本地 +Z 轴指向目标点
///   4. Path  : 按速度沿路径前进,非循环时终点钳制
///   5. 同节点多约束: 按优先级链式求解(Follow 先、Path 后,与添加顺序无关)
///   6. 组件系统更新列表: update 后由 updateObjectList 显式刷新并清空
/// 用法: test.constraint.exe

#include    <iostream>
#include    <cmath>

#include    "../../inc/FEContext.hpp"
#include    "../../inc/node/FENode.hpp"
#include    "../../inc/constraint/FEConstraintSys.hpp"
#include    "../../inc/constraint/FEConstraintFollow.hpp"
#include    "../../inc/constraint/FEConstraintLookAt.hpp"
#include    "../../inc/constraint/FEConstraintPath.hpp"
#include    "../../inc/constraint/FEConstraintTrans.hpp"

using   namespace   FE;

static  int     g_failed    =   0;

static  void    checkVec(const char* name, const real3& actual, const real3& expect, real eps = 1e-4f)
{
    real    dist    =   real(glm::length(actual - expect));
    if (dist <= eps)
    {
        std::cout << "[PASS] " << name << "  (" << actual.x << ", " << actual.y << ", " << actual.z << ")" << std::endl;
    }
    else
    {
        std::cout << "[FAIL] " << name
                  << "  actual(" << actual.x << ", " << actual.y << ", " << actual.z << ")"
                  << "  expect(" << expect.x << ", " << expect.y << ", " << expect.z << ")"
                  << "  err=" << dist << std::endl;
        ++g_failed;
    }
}

static  void    checkDir(const char* name, const real3& dir, const real3& expect, real eps = 1e-4f)
{
    real3   d   =   glm::normalize(dir);
    real3   e   =   glm::normalize(expect);
    real    dot =   real(glm::dot(d, e));
    if (dot >= real(1) - eps)
    {
        std::cout << "[PASS] " << name << "  dot=" << dot << std::endl;
    }
    else
    {
        std::cout << "[FAIL] " << name << "  dot=" << dot << "  expect direction("
                  << expect.x << ", " << expect.y << ", " << expect.z << ")" << std::endl;
        ++g_failed;
    }
}

static  void    checkBool(const char* name, bool value, bool expect)
{
    if (value == expect)
    {
        std::cout << "[PASS] " << name << std::endl;
    }
    else
    {
        std::cout << "[FAIL] " << name << "  actual=" << value << " expect=" << expect << std::endl;
        ++g_failed;
    }
}

/// 创建一个空节点并应用本地位置后立即刷新全局变换
static  Node    makeNode(FEContext& ctx, const real3& pos)
{
    Node    node    =   new FENode(ctx);
    node->setLocalTranslation(pos);
    node->makeDirty();
    node->update();
    return  node;
}

int     main()
{
    FEContext&  ctx =   FEContext::instance();

    /// ==================== 1. 构造节点与约束 ====================
    /// 目标节点: 初始位于 (2,0,0)
    Node    leader  =   makeNode(ctx, real3(2.0, 0.0, 0.0));

    /// Follow: 节点初始在原点,附加时记录相对目标的偏移(-2,0,0)
    Node    follower    =   makeNode(ctx, real3(0.0, 0.0, 0.0));
    ConstraintFollow    follow  =   new FEConstraintFollow(ctx);
    follow->setTargetNode(leader);
    follower->addComponent(Component(follow.get()));

    /// Trans: 关闭偏移保持,应吸附到固定目标点 (1,0,0)
    Node    transNode   =   makeNode(ctx, real3(0.0, 1.0, 0.0));
    ConstraintTrans     trans   =   new FEConstraintTrans(ctx);
    trans->setMaintainOffset(false);
    trans->setTarget(real3(1.0, 0.0, 0.0));
    transNode->addComponent(Component(trans.get()));

    /// LookAt: 位于原点,目标 (1,0,0),本地 +Z 应转向 +X
    Node    lookNode    =   makeNode(ctx, real3(0.0, 0.0, 0.0));
    ConstraintLookAt    look    =   new FEConstraintLookAt(ctx);
    look->setTarget(real3(1.0, 0.0, 0.0));
    lookNode->addComponent(Component(look.get()));

    /// Path: 速度为 0,节点应被放到路径起点 (0,0,0),覆盖其本地位置 (9,9,9)
    Node    pathNode    =   makeNode(ctx, real3(9.0, 9.0, 9.0));
    ConstraintPath      path    =   new FEConstraintPath(ctx);
    path->setPath({ real3(0.0, 0.0, 0.0), real3(2.0, 0.0, 0.0) });
    path->setSpeed(0.0);
    path->setLoop(false);
    pathNode->addComponent(Component(path.get()));

    /// 同节点多约束: 先添加 Path(EP_Last),后添加 Follow(EP_Second);
    /// 求解应仍按优先级执行,最终位置由后执行的 Path 决定
    Node    multiNode   =   makeNode(ctx, real3(0.0, 0.0, 0.0));
    ConstraintPath      multiPath   =   new FEConstraintPath(ctx);
    multiPath->setPath({ real3(0.0, 4.0, 0.0), real3(2.0, 4.0, 0.0) });
    multiPath->setSpeed(0.0);
    multiPath->setLoop(false);
    ConstraintFollow    multiFollow =   new FEConstraintFollow(ctx);
    multiFollow->setTargetNode(leader);
    multiNode->addComponent(Component(multiPath.get()));
    multiNode->addComponent(Component(multiFollow.get()));

    /// ==================== 2. 分发到约束系统 ====================
    FEConstraintSys     sys(ctx);
    Components          coms;
    coms.push_back(Component(follow.get()));
    coms.push_back(Component(trans.get()));
    coms.push_back(Component(look.get()));
    coms.push_back(Component(path.get()));
    coms.push_back(Component(multiPath.get()));
    coms.push_back(Component(multiFollow.get()));
    checkBool("约束全部加入系统", sys.addObjects(coms) == coms.size(), true);

    /// ==================== 3. 第一帧: 附加时不跳变 ====================
    sys.update(real(0.1));
    sys.updateObjectList();

    checkVec("Follow 附加后不跳变(保持原点)", follower->globalTranslation(), real3(0.0, 0.0, 0.0));
    checkVec("Trans  吸附到固定目标点", transNode->globalTranslation(), real3(1.0, 0.0, 0.0));
    checkDir("LookAt 本地 +Z 指向目标",
        quatr(lookNode->globalFETransform().rotation()) * real3(0.0, 0.0, 1.0), real3(1.0, 0.0, 0.0));
    checkVec("Path  速度为 0 时位于路径起点", pathNode->globalTranslation(), real3(0.0, 0.0, 0.0));
    checkVec("多约束 优先级链式求解(Path 后执行)", multiNode->globalTranslation(), real3(0.0, 4.0, 0.0));
    checkBool("更新列表已被 updateObjectList 清空", sys.updateList().objects().empty(), true);

    /// ==================== 4. 目标移动后第二帧 ====================
    leader->setLocalTranslation(real3(5.0, 0.0, 0.0));
    leader->makeDirty();
    leader->update();

    sys.update(real(0.1));
    sys.updateObjectList();

    checkVec("Follow 目标移动后按偏移跟随", follower->globalTranslation(), real3(3.0, 0.0, 0.0));
    checkVec("多约束 位置仍由 Path 决定", multiNode->globalTranslation(), real3(0.0, 4.0, 0.0));
    checkVec("Path  速度 0 不随时间移动", pathNode->globalTranslation(), real3(0.0, 0.0, 0.0));
    checkBool("更新列表第二次刷新后仍为空", sys.updateList().objects().empty(), true);

    /// ==================== 5. Path 采样单元测试 ====================
    /// 路径 (0,0,0)->(2,0,0)->(2,2,0),总长 4;速度 1
    ConstraintPath  unitPath    =   new FEConstraintPath(ctx);
    unitPath->setPath({ real3(0.0, 0.0, 0.0), real3(2.0, 0.0, 0.0), real3(2.0, 2.0, 0.0) });
    unitPath->setSpeed(1.0);
    unitPath->setLoop(false);

    FETransform     base    =   pathNode->globalFETransform();
    checkVec("Path  t=0.5 第一段中点", unitPath->solve(base, real(0.5)).position(), real3(0.5, 0.0, 0.0));
    checkVec("Path  t=1.5 第一段末端起", unitPath->solve(base, real(1.5)).position(), real3(1.5, 0.0, 0.0));
    checkVec("Path  t=2.5 第二段中点", unitPath->solve(base, real(2.5)).position(), real3(2.0, 0.5, 0.0));
    checkVec("Path  非循环超出终点钳制", unitPath->solve(base, real(5.0)).position(), real3(2.0, 2.0, 0.0));

    unitPath->setLoop(true);
    checkVec("Path  循环模式按总长取模", unitPath->solve(base, real(5.0)).position(), real3(1.0, 0.0, 0.0));

    /// ==================== 6. 结果 ====================
    if (g_failed == 0)
    {
        std::cout << "test.constraint PASSED" << std::endl;
        return  0;
    }
    else
    {
        std::cout << "test.constraint FAILED, " << g_failed << " case(s)" << std::endl;
        return  1;
    }
}
