/// test.constraint.trans
/// 平移约束可视化(Trans: 仅位置,偏移在世界空间,不受目标旋转影响)
///   1. 橙色目标由动画驱动沿 X 轴往返,并固定绕 Y 旋转 45°
///   2. 紫色方块附加 Trans(targetNode=目标),偏移 (0,1.2,0),始终悬在目标正上方
///   3. 绿色方块附加 Trans(固定目标点 (2.4,0.6,0),maintainOffset=false),直接吸附到目标点
///   4. 断言: 目标旋转 90° 后紫色方块仍在正上方(世界空间偏移),与 Follow 的本地系偏移不同
/// 用法: test.constraint.trans.exe [--vulkan|--webgpu]

#include    <iostream>

#include    "ConstraintVisualCommon.hpp"
#include    "constraint/FEConstraintTrans.hpp"
#include    "animation/FEAnimationHelper.hpp"

using   namespace   FE;

/// 注册渲染后端插件
USING_PLUGIN(rs_vulkan);
USING_PLUGIN(rs_webgpu);

int     main(int argc, char** argv)
{
    fe_test::VisualCase tc;
    if  (!tc.begin(argc, argv, "test.constraint.trans"))
        return  1;

    /// ==================== 1. 节点 ====================
    /// 动画目标(橙色): 带 45° 旋转,用来证明平移约束不跟随旋转
    Node    leader  =   tc.makeNode(real3(-2.0, 1.4, 0.0), 0.7, float4(1.00f, 0.55f, 0.15f, 1.0f));
    leader->setLocalRotation(quatr(glm::radians(real3(0.0, 45.0, 0.0))));
    leader->makeDirty();
    leader->update();

    /// 跟随目标节点(紫色): leader + (0,1.2,0)
    Node    hover   =   tc.makeNode(real3(-2.0, 2.6, 0.0), 0.4, float4(0.75f, 0.35f, 0.95f, 1.0f));
    /// 吸附固定目标点(绿色): 初始在原点上方,附加后应跳到 (2.4,0.6,0)
    Node    fixed   =   tc.makeNode(real3(0.0, 0.6, 0.0), 0.4, float4(0.20f, 0.90f, 0.35f, 1.0f));

    /// ==================== 2. 平移约束 ====================
    ConstraintTrans transNode   =   new FEConstraintTrans(*tc._ctx);
    transNode->setTargetNode(leader);
    hover->addComponent(Component(transNode.get()));

    ConstraintTrans transPoint  =   new FEConstraintTrans(*tc._ctx);
    transPoint->setTarget(real3(2.4, 0.6, 0.0));
    transPoint->setMaintainOffset(false);
    fixed->addComponent(Component(transPoint.get()));

    /// ==================== 3. 动画驱动目标位置 ====================
    Animation   anim    =   FEAnimationHelper::createNodeAnimtion(*tc._ctx, NP_TRANSFORM_XYZ);
    anim->clip()->addKeyFrame(0.0, real3(-2.0, 1.4, 0.0));
    anim->clip()->addKeyFrame(3.0, real3( 2.0, 1.4, 0.0));
    leader->addComponent(Component(anim.get()));

    tc.dispatch({ leader, hover, fixed });
    tc.playLoop();
    tc.step(3);

    /// ==================== 4. 断言 ====================
    real3   leaderPos   =   leader->globalTranslation();
    tc.checkVec("目标移动时保持世界偏移 (0,+1.2,0)", hover->globalTranslation(), leaderPos + real3(0.0, 1.2, 0.0));
    tc.checkVec("固定目标点吸附到 (2.4,0.6,0)",      fixed->globalTranslation(), real3(2.4, 0.6, 0.0));

    /// 目标旋转 90°: 平移约束使用世界空间偏移,位置不应改变
    leader->setLocalRotation(quatr(glm::radians(real3(0.0, 90.0, 0.0))));
    leader->makeDirty();
    leader->update();
    tc.step(1);

    leaderPos   =   leader->globalTranslation();
    tc.checkVec("目标旋转 90° 后仍在正上方", hover->globalTranslation(), leaderPos + real3(0.0, 1.2, 0.0));
    tc.checkVec("固定目标点不受目标旋转影响", fixed->globalTranslation(), real3(2.4, 0.6, 0.0));

    /// ==================== 5. 渲染循环 ====================
    tc.loop();
    return  tc.finish();
}
