/// test.constraint.lookat
/// 朝向约束可视化(LookAt: 仅旋转,本地 +Z 轴指向目标)
///   1. 橙色目标由动画驱动沿矩形巡游(-2.4,-2.4) -> (2.4,-2.4) -> (2.4,2.4) -> (-2.4,2.4),循环
///   2. 黄色细长方块(LookAt 绑定目标节点)始终把 +Z 长边对准目标,绕自身转动
///   3. 蓝色细长方块(LookAt 绑定固定目标点 (2.0,1.4,-3.0))静止指向 -Z
///   4. 断言: 本地 +Z 旋转后的方向与 (目标 - 自身位置) 一致
/// 用法: test.constraint.lookat.exe [--vulkan|--webgpu]

#include    <iostream>

#include    "ConstraintVisualCommon.hpp"
#include    "constraint/FEConstraintLookAt.hpp"
#include    "animation/FEAnimationHelper.hpp"

using   namespace   FE;

/// 注册渲染后端插件
USING_PLUGIN(rs_vulkan);
USING_PLUGIN(rs_webgpu);

/// 节点本地 +Z 轴在世界空间中的方向
static  real3   forwardOf(Node node)
{
    return  quatr(node->globalFETransform().rotation()) * real3(0.0, 0.0, 1.0);
}

int     main(int argc, char** argv)
{
    fe_test::VisualCase tc;
    if  (!tc.begin(argc, argv, "test.constraint.lookat"))
        return  1;

    /// ==================== 1. 节点 ====================
    /// 巡游目标(橙色)
    Node    target  =   tc.makeNode(real3(-2.4, 1.4, -2.4), 0.5, float4(1.00f, 0.55f, 0.15f, 1.0f));
    /// 观察者(黄色): 细长方块,长边为本地 +Z
    Node    watcher =   tc.makeNode(real3(0.0, 1.4, 0.0), real3(0.12, 0.12, 0.9), float4(0.95f, 0.85f, 0.15f, 1.0f));
    /// 固定点观察者(蓝色)
    Node    fixed   =   tc.makeNode(real3(2.0, 1.4, 0.0), real3(0.12, 0.12, 0.9), float4(0.35f, 0.55f, 1.00f, 1.0f));

    /// ==================== 2. 朝向约束 ====================
    ConstraintLookAt    lookNode    =   new FEConstraintLookAt(*tc._ctx);
    lookNode->setTargetNode(target);
    watcher->addComponent(Component(lookNode.get()));

    ConstraintLookAt    lookPoint   =   new FEConstraintLookAt(*tc._ctx);
    lookPoint->setTarget(real3(2.0, 1.4, -3.0));
    fixed->addComponent(Component(lookPoint.get()));

    /// ==================== 3. 动画驱动目标巡游 ====================
    Animation   anim    =   FEAnimationHelper::createNodeAnimtion(*tc._ctx, NP_TRANSFORM_XYZ);
    AnimClip    clip    =   anim->clip();
    clip->addKeyFrame(0.0, real3(-2.4, 1.4, -2.4));
    clip->addKeyFrame(1.5, real3( 2.4, 1.4, -2.4));
    clip->addKeyFrame(3.0, real3( 2.4, 1.4,  2.4));
    clip->addKeyFrame(4.5, real3(-2.4, 1.4,  2.4));
    target->addComponent(Component(anim.get()));

    tc.dispatch({ target, watcher, fixed });
    tc.playLoop();
    tc.step(3);

    /// ==================== 4. 断言 ====================
    real3   targetPos   =   target->globalTranslation();
    real3   watchPos    =   watcher->globalTranslation();
    tc.checkDir("观察者 +Z 对准移动目标", forwardOf(watcher), targetPos - watchPos);
    tc.checkDir("固定观察者 +Z 对准固定点", forwardOf(fixed), real3(2.0, 1.4, -3.0) - fixed->globalTranslation());

    /// 目标移到另一个方向后再验证一次(巡游到下一段)
    tc.step(3);
    targetPos   =   target->globalTranslation();
    watchPos    =   watcher->globalTranslation();
    tc.checkDir("目标移动后仍保持对准", forwardOf(watcher), targetPos - watchPos);

    /// ==================== 5. 渲染循环 ====================
    tc.loop();
    return  tc.finish();
}
