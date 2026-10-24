/// test.constraint.follow
/// 跟随约束可视化(Follow: 位置 + 旋转,偏移记录在目标本地坐标系)
///   1. 橙色目标由动画驱动沿 X 轴往返(-2,1.4,0) -> (2,1.4,0),循环播放
///   2. 青色方块附加 Follow,附加时偏移 (1.6,0,0)(目标本地系)
///   3. 蓝色方块附加 Follow,附加时偏移 (0,1.2,0)
///   4. 断言: 目标绕 Y 旋转 90° 后,水平偏移随之旋转到 -Z,垂直偏移不变
/// 用法: test.constraint.follow.exe [--vulkan|--webgpu]

#include    <iostream>

#include    "ConstraintVisualCommon.hpp"
#include    "constraint/FEConstraintFollow.hpp"
#include    "animation/FEAnimationHelper.hpp"

using   namespace   FE;

/// 注册渲染后端插件
USING_PLUGIN(rs_vulkan);
USING_PLUGIN(rs_webgpu);

int     main(int argc, char** argv)
{
    fe_test::VisualCase tc;
    if  (!tc.begin(argc, argv, "test.constraint.follow"))
        return  1;

    /// ==================== 1. 节点 ====================
    /// 动画目标(橙色)
    Node    leader  =   tc.makeNode(real3(-2.0, 1.4, 0.0), 0.7, float4(1.00f, 0.55f, 0.15f, 1.0f));
    /// 水平偏移(青色): 初始位于目标 (+1.6,0,0)
    Node    side    =   tc.makeNode(real3(-0.4, 1.4, 0.0), 0.45, float4(0.10f, 0.80f, 0.95f, 1.0f));
    /// 垂直偏移(蓝色): 初始位于目标 (+0,1.2,0)
    Node    above   =   tc.makeNode(real3(-2.0, 2.6, 0.0), 0.4, float4(0.35f, 0.55f, 1.00f, 1.0f));

    /// ==================== 2. 跟随约束 ====================
    ConstraintFollow    followSide  =   new FEConstraintFollow(*tc._ctx);
    followSide->setTargetNode(leader);
    side->addComponent(Component(followSide.get()));

    ConstraintFollow    followAbove =   new FEConstraintFollow(*tc._ctx);
    followAbove->setTargetNode(leader);
    above->addComponent(Component(followAbove.get()));

    /// ==================== 3. 动画驱动目标位置 ====================
    Animation   anim    =   FEAnimationHelper::createNodeAnimtion(*tc._ctx, NP_TRANSFORM_XYZ);
    anim->clip()->addKeyFrame(0.0, real3(-2.0, 1.4, 0.0));
    anim->clip()->addKeyFrame(3.0, real3( 2.0, 1.4, 0.0));
    leader->addComponent(Component(anim.get()));

    tc.dispatch({ leader, side, above });
    tc.playLoop();
    tc.step(3);

    /// ==================== 4. 断言 ====================
    real3   leaderPos   =   leader->globalTranslation();
    tc.checkVec("目标移动时水平偏移保持 (+1.6,0,0)", side->globalTranslation(), leaderPos + real3(1.6, 0.0, 0.0));
    tc.checkVec("目标移动时垂直偏移保持 (0,+1.2,0)", above->globalTranslation(), leaderPos + real3(0.0, 1.2, 0.0));

    /// 目标绕 Y 旋转 90°: 偏移在目标本地系,水平偏移应转到 -Z 方向
    leader->setLocalRotation(quatr(glm::radians(real3(0.0, 90.0, 0.0))));
    leader->makeDirty();
    leader->update();
    tc.step(1);

    leaderPos   =   leader->globalTranslation();
    tc.checkVec("目标旋转 90° 后水平偏移随旋转", side->globalTranslation(), leaderPos + real3(0.0, 0.0, -1.6));
    tc.checkVec("目标旋转 90° 后垂直偏移不变",   above->globalTranslation(), leaderPos + real3(0.0, 1.2, 0.0));

    /// ==================== 5. 渲染循环 ====================
    /// 窗口关闭时抢在场景销毁 device 之前释放用例持有的节点引用
    tc.onClose([&]
    {
        leader  =   nullptr;
        side    =   nullptr;
        above   =   nullptr;
    });
    tc.loop();
    return  tc.finish();
}
