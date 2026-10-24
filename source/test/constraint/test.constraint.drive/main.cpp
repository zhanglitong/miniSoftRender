/// test.constraint.drive
/// 驱动约束可视化(Drive: 驱动节点等效父节点,可同时驱动多个被驱动节点)
///   1. 橙色驱动节点由动画驱动沿 X 轴往返(-2,1.4,0) -> (2,1.4,0),循环播放
///   2. 青色/蓝色方块通过 FEConstraintDrive 挂到驱动节点,添加时自动记录偏移
///      青色偏移 (1.6,0,0),蓝色偏移 (0,1.2,0),节点树层级保持不变
///   3. 断言: 驱动节点平移/旋转/缩放时,被驱动节点按偏移完整跟随(完整 TRS)
/// 用法: test.constraint.drive.exe [--vulkan|--webgpu]

#include    <iostream>

#include    "ConstraintVisualCommon.hpp"
#include    "constraint/FEConstraintDrive.hpp"
#include    "animation/FEAnimationHelper.hpp"

using   namespace   FE;

/// 注册渲染后端插件
USING_PLUGIN(rs_vulkan);
USING_PLUGIN(rs_webgpu);

int     main(int argc, char** argv)
{
    fe_test::VisualCase tc;
    if  (!tc.begin(argc, argv, "test.constraint.drive"))
        return  1;

    /// ==================== 1. 节点 ====================
    /// 驱动节点(橙色),单位缩放便于换算偏移
    Node    driver  =   tc.makeNode(real3(-2.0, 1.4, 0.0), 1.0, float4(1.00f, 0.55f, 0.15f, 1.0f));
    /// 水平偏移(青色): 初始位于驱动节点 (+1.6,0,0),自身缩放 0.45
    Node    side    =   tc.makeNode(real3(-0.4, 1.4, 0.0), 0.45, float4(0.10f, 0.80f, 0.95f, 1.0f));
    /// 垂直偏移(蓝色): 初始位于驱动节点 (0,+1.2,0),自身缩放 0.4
    Node    above   =   tc.makeNode(real3(-2.0, 2.6, 0.0), 0.4, float4(0.35f, 0.55f, 1.00f, 1.0f));

    /// ==================== 2. 驱动约束 ====================
    ConstraintDrive drive   =   new FEConstraintDrive(*tc._ctx);
    driver->addComponent(Component(drive.get()));
    /// 添加时按当前世界姿态记录相对偏移,添加瞬间不发生跳变
    tc.checkBool("添加水平偏移节点", drive->addDrivenNode(side), true);
    tc.checkBool("添加垂直偏移节点", drive->addDrivenNode(above), true);
    tc.checkBool("重复添加被拒绝",   drive->addDrivenNode(side), false);
    tc.checkBool("被驱动节点数量为 2", drive->drivenNodeCount() == 2, true);

    /// ==================== 3. 动画驱动节点位置 ====================
    Animation   anim    =   FEAnimationHelper::createNodeAnimtion(*tc._ctx, NP_TRANSFORM_XYZ);
    anim->clip()->addKeyFrame(0.0, real3(-2.0, 1.4, 0.0));
    anim->clip()->addKeyFrame(3.0, real3( 2.0, 1.4, 0.0));
    driver->addComponent(Component(anim.get()));

    tc.dispatch({ driver, side, above });
    tc.playLoop();
    tc.step(3);

    /// ==================== 4. 断言 ====================
    real3   driverPos   =   driver->globalTranslation();
    tc.checkVec("驱动节点平移时水平偏移保持 (+1.6,0,0)", side->globalTranslation(), driverPos + real3(1.6, 0.0, 0.0));
    tc.checkVec("驱动节点平移时垂直偏移保持 (0,+1.2,0)", above->globalTranslation(), driverPos + real3(0.0, 1.2, 0.0));

    /// 驱动节点绕 Y 旋转 90°: 偏移在驱动节点本地系,水平偏移应转到 -Z 方向
    driver->setLocalRotation(quatr(glm::radians(real3(0.0, 90.0, 0.0))));
    driver->makeDirty();
    driver->update();
    tc.step(1);

    driverPos   =   driver->globalTranslation();
    tc.checkVec("驱动节点旋转 90° 后水平偏移随旋转", side->globalTranslation(), driverPos + real3(0.0, 0.0, -1.6));
    tc.checkVec("驱动节点旋转 90° 后垂直偏移不变",   above->globalTranslation(), driverPos + real3(0.0, 1.2, 0.0));

    /// 驱动节点缩放 2 倍: 偏移距离与相对缩放同步放大(自身缩放 0.45/0.4 -> 0.9/0.8)
    driver->setLocalScaling(real3(2.0));
    driver->makeDirty();
    driver->update();
    tc.step(1);

    driverPos   =   driver->globalTranslation();
    tc.checkVec("驱动节点缩放 2 倍后水平偏移同步放大", side->globalTranslation(), driverPos + real3(0.0, 0.0, -3.2));
    tc.checkVec("驱动节点缩放 2 倍后垂直偏移同步放大", above->globalTranslation(), driverPos + real3(0.0, 2.4, 0.0));
    tc.checkVec("被驱动节点相对缩放保持恒定 0.9", side->globalScaling(), real3(0.9));

    /// ==================== 5. 渲染循环 ====================
    /// 窗口关闭时抢在场景销毁 device 之前释放用例持有的节点引用
    tc.onClose([&]
    {
        drive   =   nullptr;
        driver  =   nullptr;
        side    =   nullptr;
        above   =   nullptr;
    });
    tc.loop();
    return  tc.finish();
}
