/// <summary>
/// FEViewPad
/// 视图操作盘(参考 3ds Max / Unity 的 ViewCube + 罗盘环):
///   - 停靠在视口右上角的伪 3D 立方体 + 地面罗盘环,随当前相机朝向实时转动;
///   - 立方体六面标注 前/后/左/右/上/下,单击面切换到对应轴视图,单击角切换到等轴视图;
///   - 拖动立方体为自由轨道旋转,拖动罗盘环为仅水平环绕;
///   - 滚轮缩放,双击盘面回到默认视角;切换视图做短促平滑过渡;
///   - 只操作相机,不写 FEUndoStack(与视口轨道旋转/相机复位一致)。
/// 几何用 ImDrawList 按相机 right/up/dir 正交投影绘制,无需额外 3D 通道。
/// </summary>

#pragma     once

#include    "FEUiStyle.hpp"

#include    "../../../inc/FECamera.hpp"

namespace   FE
{
    class   FEScene;
    class   FEContext;

    class   FEViewPad
    {
    public:
        /// <summary>
        /// 操作盘上的可命中区域
        /// </summary>
        enum    Part
        {
            /// <summary>
            /// 空白区
            /// </summary>
            kPartNone   =   0,
            /// <summary>
            /// 立方体的可见面:单击切换轴视图
            /// </summary>
            kPartFace   =   1,
            /// <summary>
            /// 立方体的可见角:单击切换等轴视图
            /// </summary>
            kPartCorner =   2,
            /// <summary>
            /// 罗盘环:拖动水平环绕
            /// </summary>
            kPartRing   =   3,
        };
    public:
        FEViewPad()     =   default;
        ~FEViewPad()    =   default;
        FEViewPad(const FEViewPad&)             =   delete;
        FEViewPad&  operator=(const FEViewPad&) =   delete;
    public:
        /// <summary>
        /// 绑定场景与上下文;由 FEEditorUi 挂接主界面时调用
        /// </summary>
        /// <param name="scene">被操作的场景</param>
        /// <param name="ctx">引擎上下文(取轨道旋转/缩放的锚点)</param>
        void    bind(FEScene* scene,FEContext* ctx);
        /// <summary>
        /// 断开场景/上下文并清空交互状态;由 FEEditorUi 释放主界面前调用
        /// </summary>
        void    unbind();
        /// <summary>
        /// 显示/隐藏(由 Ribbon「设置 → 选项 → 视图操作盘」切换)
        /// </summary>
        void    toggle();
        void    setOpen(bool open);
        bool    isOpen()    const   {   return  _open;  }
        /// <summary>
        /// 每帧绘制并处理鼠标;未显示或未绑定时直接返回
        /// </summary>
        void    draw();
    protected:
        /// <summary>
        /// 清除过渡动画与拖拽状态(重新绑定时使用)
        /// </summary>
        void    resetState();
        /// <summary>
        /// 推进视图切换的平滑过渡
        /// </summary>
        /// <param name="camera">被操作的相机</param>
        /// <param name="dt">帧间隔(秒)</param>
        void    stepAnimation(FECamera& camera,real dt);
        /// <summary>
        /// 从当前朝向过渡到指定方位
        /// </summary>
        /// <param name="camera">被操作的相机</param>
        /// <param name="target">目标点</param>
        /// <param name="eye">目标眼睛位置</param>
        /// <param name="up">目标 up</param>
        void    beginAnimation(FECamera& camera,const real3& target,const real3& eye,const real3& up);
        /// <summary>
        /// 过渡到某个面(前/后/左/右/上/下)对应的轴视图
        /// </summary>
        void    snapToFace(FECamera& camera,int face);
        /// <summary>
        /// 过渡到某个立方体角对应的等轴视图
        /// </summary>
        void    snapToCorner(FECamera& camera,int corner);
        /// <summary>
        /// 过渡回默认视角(单击双击盘面)
        /// </summary>
        void    snapToDefault(FECamera& camera);
        /// <summary>
        /// 按鼠标位移轨道旋转(与视口左键拖动同一方向约定)
        /// </summary>
        void    orbitByMouse(FECamera& camera,const ImVec2& offset,int part);
    protected:
        /// <summary>
        /// 被操作的场景
        /// </summary>
        FEScene*    _scene  =   nullptr;
        /// <summary>
        /// 引擎上下文(锚点用于轨道旋转/缩放)
        /// </summary>
        FEContext*  _ctx    =   nullptr;
        /// <summary>
        /// 操作盘开关
        /// </summary>
        bool        _open   =   true;
        /// <summary>
        /// 过渡动画是否进行中
        /// </summary>
        bool        _animating  =   false;
        /// <summary>
        /// 过渡进度 0..1
        /// </summary>
        real        _animT      =   0.0;
        /// <summary>
        /// 过渡起点(目标点/眼睛/up)
        /// </summary>
        real3       _animTarget0;
        real3       _animEye0;
        real3       _animUp0;
        /// <summary>
        /// 过渡终点(目标点/眼睛/up)
        /// </summary>
        real3       _animTarget1;
        real3       _animEye1;
        real3       _animUp1;
        /// <summary>
        /// 按下时命中的区域与面/角下标
        /// </summary>
        int         _pressPart  =   kPartNone;
        int         _pressFace  =   -1;
        int         _pressCorner=   -1;
        /// <summary>
        /// 按下是否为双击的第二下(双击回默认视角)
        /// </summary>
        bool        _pressDouble   =   false;
        /// <summary>
        /// 当前是否已进入拖拽(超过阈值后不再触发单击切换)
        /// </summary>
        bool        _dragging      =   false;
        /// <summary>
        /// 按下位置(屏幕坐标)
        /// </summary>
        ImVec2      _pressPos      =   ImVec2(0.0f,0.0f);
    };
}
