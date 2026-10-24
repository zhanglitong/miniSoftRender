/// test.constraint.path
/// 路径约束可视化(Path: 仅位置,按速度沿路径点前进)
///   1. 灰色小方块标出矩形路径的四个角点
///   2. 绿色方块附加 Path(循环,速度 1.8),沿矩形绕行
///   3. 蓝色方块附加 Path(非循环,起点偏移 20),1 秒内走到终点后被钳制在终点
///   4. 断言: 节点始终落在路径线段上(离最近线段距离 ~ 0),高度恒定;两帧间位置在移动
/// 用法: test.constraint.path.exe [--vulkan|--webgpu]

#include    <iostream>
#include    <vector>
#include    <cmath>
#include    <algorithm>
#include    <limits>

#include    "ConstraintVisualCommon.hpp"
#include    "constraint/FEConstraintPath.hpp"

using   namespace   FE;

/// 注册渲染后端插件
USING_PLUGIN(rs_vulkan);
USING_PLUGIN(rs_webgpu);

/// 点到线段集合的最短距离
static  real    distToPath(const real3& p, const std::vector<real3>& points)
{
    real    best    =   std::numeric_limits<real>::max();
    for (size_t i = 1; i < points.size(); ++i)
    {
        real3   a   =   points[i - 1];
        real3   b   =   points[i];
        real3   ab  =   b - a;
        real    len2    =   glm::dot(ab, ab);
        real    t   =   (len2 > 0) ? std::clamp(glm::dot(p - a, ab) / len2, real(0), real(1)) : real(0);
        real3   proj    =   a + t * ab;
        best    =   std::min(best, real(glm::length(p - proj)));
    }
    return  best;
}

int     main(int argc, char** argv)
{
    fe_test::VisualCase tc;
    if  (!tc.begin(argc, argv, "test.constraint.path"))
        return  1;

    /// ==================== 1. 路径 ====================
    /// 闭合矩形路径(首尾点重合,循环时无跳变),高度 0.5
    const   real    kY  =   0.5f;
    std::vector<real3>  points  =
    {
        real3(-2.8, kY, -2.0), real3(2.8, kY, -2.0),
        real3( 2.8, kY,  2.0), real3(-2.8, kY, 2.0),
        real3(-2.8, kY, -2.0)
    };

    /// 路径角点标记(灰色)
    Nodes   nodes;
    for (size_t i = 0; i + 1 < points.size(); ++i)
        nodes.push_back(tc.makeNode(points[i], 0.14, float4(0.60f, 0.60f, 0.65f, 1.0f)));

    /// 循环绕行(绿色)
    Node    runner  =   tc.makeNode(points.front(), 0.4, float4(0.20f, 0.90f, 0.35f, 1.0f));
    /// 非循环走到终点(蓝色): 起点偏移 20,1 秒内即到达终点被钳制
    Node    stopper =   tc.makeNode(points.front(), 0.3, float4(0.35f, 0.55f, 1.00f, 1.0f));

    ConstraintPath  loopPath    =   new FEConstraintPath(*tc._ctx);
    loopPath->setPath(points);
    loopPath->setSpeed(1.8);
    loopPath->setLoop(true);
    runner->addComponent(Component(loopPath.get()));

    ConstraintPath  clampPath   =   new FEConstraintPath(*tc._ctx);
    clampPath->setPath(points);
    clampPath->setSpeed(2.0);
    clampPath->setLoop(false);
    clampPath->setStartOffset(20.0);
    stopper->addComponent(Component(clampPath.get()));

    nodes.push_back(runner);
    nodes.push_back(stopper);

    tc.dispatch(nodes);
    tc.step(2);

    /// ==================== 2. 断言 ====================
    real3   p0  =   runner->globalTranslation();
    tc.checkBool("循环节点位于路径线段上", distToPath(p0, points) <= 2e-2f, true);
    tc.checkBool("循环节点高度恒定",       std::abs(p0.y - kY) <= 2e-2f, true);
    tc.checkVec ("非循环节点被钳制在终点", stopper->globalTranslation(), points.back());

    /// 再推进两帧,确认循环节点确实在沿路径移动
    tc.step(2);
    real3   p1  =   runner->globalTranslation();
    tc.checkBool("循环节点随时间前进", real(glm::length(p1 - p0)) > 1e-2f, true);
    tc.checkBool("前进后仍在路径上",   distToPath(p1, points) <= 2e-2f, true);
    std::cout << "runner pos=[" << p1.x << ", " << p1.y << ", " << p1.z << "]" << std::endl;

    /// ==================== 3. 渲染循环 ====================
    /// 窗口关闭时抢在场景销毁 device 之前释放用例持有的节点引用
    tc.onClose([&]
    {
        nodes.clear();
        runner  =   nullptr;
        stopper =   nullptr;
    });
    tc.loop();
    return  tc.finish();
}
