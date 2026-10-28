/// <summary>
/// FETimeLine
/// ImGui 版时间线编辑控件,参考 Qt 版 UiTickMgr(动画编辑器)的交互:
///   - 数据直接绑定选中节点的 FEAnimation / FEAnimClip / FEKeyFrameTrack;
///   - 标尺 + 播放头,点击/拖动标尺跳帧,Ctrl+滚轮以鼠标为锚点缩放,中键拖动平移;
///   - 动画行显示播放范围块(可拖动改 offset),轨道行显示关键帧圆点;
///   - 关键帧单击选中、Ctrl/Shift 追加或取消、拖动改时间、框选;
///   - 右键菜单:复制/粘贴/删除/插值(贝塞尔/线性/离散)/在此处添加关键帧;
///   - 播放控制:播放/暂停/首帧/上一帧/下一帧/末帧/循环;
///   - 所有数据修改经 FEUndoStack,支持撤销重做。
/// 窗口是屏幕底部固定面板,由 Ribbon「设置 → 动画 → 时间线」开关。
/// </summary>

#pragma     once

#include    "../../../inc/animation/FEAnimation.hpp"
#include    "../../../inc/animation/FEKeyFrameTrack.hpp"
#include    "../../../inc/FEUndoStack.hpp"
#include    "../../../inc/node/FENode.hpp"

#include    <imgui.h>

#include    <map>
#include    <set>
#include    <string>
#include    <vector>

namespace   FE
{
    class   FEScene;
    class   FEModelTree;

    class   FETimeLine
    {
    public:
        FETimeLine() = default;
        ~FETimeLine() = default;
        FETimeLine(const FETimeLine&)               =   delete;
        FETimeLine& operator=(const FETimeLine&)    =   delete;
    public:
        /// <summary>
        /// 绑定数据来源(场景动画系统 / 撤销栈 / 模型树选中项)
        /// </summary>
        void    bind(FEScene* scene,FEUndoStack* undoStack,FEModelTree* modelTree);
        /// <summary>
        /// 场景销毁前解绑,丢弃所有指向场景对象的引用
        /// </summary>
        void    unbind();
        /// <summary>
        /// 清空编辑状态(切换/重置/打开工程时调用)
        /// </summary>
        void    reset();
        /// <summary>
        /// 显示/隐藏时间线面板
        /// </summary>
        void    toggle();
        /// <summary>
        /// 直接设置面板的显示状态
        /// </summary>
        void    setOpen(bool open);
        bool    isOpen()  const    {   return  _open;  }
        /// <summary>
        /// 每帧绘制;未打开时直接返回
        /// </summary>
        /// <param name="iconFont">图标字体,可为 nullptr(暂未使用)</param>
        void    draw(ImFont* iconFont);
        /// <summary>
        /// 设置动画帧率(默认 30)
        /// </summary>
        void    setFps(float fps);
        float   fps() const         {   return  _fps;   }
        /// <summary>
        /// 跳到指定帧并重新采样动画
        /// </summary>
        /// <param name="frame">帧号</param>
        /// <param name="applyToAnim">是否立即把时间应用到动画系统</param>
        void    setCurFrame(int frame,bool applyToAnim = true);
        int     curFrame() const;
        real    curTime() const     {   return  _curTime;   }
        /// <summary>
        /// 在当前帧为选中动画添加(或更新)关键帧
        /// </summary>
        void    addKeyframe();
    protected:
        /// <summary>
        /// 时间线的一行:动画头行或轨道行
        /// </summary>
        struct  Row
        {
            Animation       _anim;
            KeyFrameTrack   _track;
            bool            _header =   false;
        };
        /// <summary>
        /// 拖拽关键帧时保存的轨道原始数据
        /// </summary>
        struct  DragTrack
        {
            KeyFrameTrack           _track;
            std::vector<real>       _times;
            std::vector<size_t>     _indices;
            real                    _offset  =   0;
        };
        /// <summary>
        /// 剪贴板:一条轨道上的若干个关键帧(时间线时间 + 值)
        /// </summary>
        struct  ClipKey
        {
            real        _time   =   0;
            KFValue     _value  =   {};
        };
        struct  ClipTrack
        {
            PropIndex               _prop   =   -1;
            std::vector<ClipKey>    _keys;
        };
        /// <summary>
        /// 鼠标交互状态
        /// </summary>
        enum    DragMode
        {
            DM_None,
            /// <summary>
            /// 拖动播放头
            /// </summary>
            DM_Playhead,
            /// <summary>
            /// 拖动关键帧
            /// </summary>
            DM_Keys,
            /// <summary>
            /// 拖动动画范围块(改 offset)
            /// </summary>
            DM_Offset,
            /// <summary>
            /// 空白处框选
            /// </summary>
            DM_Box,
        };
    protected:
        /// <summary>
        /// 选中节点变化时同步动画列表与编辑状态
        /// </summary>
        void    ensureNode();
        /// <summary>
        /// 从节点重新收集动画组件
        /// </summary>
        void    reloadAnimations();
        /// <summary>
        /// 重建行列表(动画头 + 轨道)
        /// </summary>
        void    rebuildRows();
        /// <summary>
        /// 所有动画的时间范围(帧)
        /// </summary>
        void    contentRange(double& firstFrame,double& lastFrame) const;
        /// <summary>
        /// 帧号 → 屏幕 x / 屏幕 x → 帧号
        /// </summary>
        float   xForFrame(double frame) const;
        double  frameForX(float x) const;
        /// <summary>
        /// 行号 → 屏幕 y(顶部)
        /// </summary>
        float   rowTop(int index) const;
        /// <summary>
        /// 鼠标 y 所在行,-1 表示不在行区域内
        /// </summary>
        int     rowAtY(float y) const;
        /// <summary>
        /// 测试某行在 x 处命中的关键帧索引,-1 表示未命中
        /// </summary>
        int     keyAt(int row,float x,float y) const;
        /// <summary>
        /// 查找轨道所属动画
        /// </summary>
        Animation   animationOf(KeyFrameTrack track) const;
        /// <summary>
        /// 关键帧选择
        /// </summary>
        bool    isKeySelected(KeyFrameTrack track,size_t index) const;
        void    addKeySelection(KeyFrameTrack track,size_t index);
        void    removeKeySelection(KeyFrameTrack track,size_t index);
        void    clearSelection();
        /// <summary>
        /// 某个动画的全部关键帧是否都被选中
        /// </summary>
        bool    allKeysSelected(Animation anim) const;
        /// <summary>
        /// 收集所有可见关键帧的帧号(去重排序后返回)
        /// </summary>
        void    collectKeyFrames(std::vector<double>& frames) const;
        /// <summary>
        /// 首帧/上一帧/下一帧/末帧
        /// </summary>
        /// <param name="mode">0 首帧 1 上一帧 2 下一帧 3 末帧</param>
        void    gotoKeyframe(int mode);
        /// <summary>
        /// 把时间应用到动画系统并刷新节点
        /// </summary>
        void    applyTime();
        /// <summary>
        /// 动画数据变更后:重算范围、重设时间、刷新节点姿态
        /// </summary>
        void    refreshScene();
        /// <summary>
        /// 工具栏
        /// </summary>
        void    drawToolbar();
        /// <summary>
        /// 标尺 + 行 + 关键帧 + 播放头
        /// </summary>
        void    drawCanvas(bool hovered);
        /// <summary>
        /// 鼠标交互
        /// </summary>
        void    handleInput(bool hovered);
        /// <summary>
        /// 右键菜单
        /// </summary>
        void    handleContextMenu();
        /// <summary>
        /// 关键帧拖动结束后应用时间偏移
        /// </summary>
        void    applyKeyDrag(float deltaPixel);
        /// <summary>
        /// 范围块拖动结束后应用 offset
        /// </summary>
        void    applyOffsetDrag(float deltaPixel);
        /// <summary>
        /// 矩形框选
        /// </summary>
        void    boxSelect(float x0,float y0,float x1,float y1);
        /// <summary>
        /// 复制/粘贴/删除/设置插值
        /// </summary>
        void    copySelection();
        void    pasteClipboard();
        void    deleteSelection();
        void    setInterpolation(int type);
        /// <summary>
        /// 为选中节点创建动画组件
        /// </summary>
        void    createAnimation();
    protected:
        FEScene*                    _scene      =   nullptr;
        FEUndoStack*                _undoStack  =   nullptr;
        FEModelTree*                _modelTree  =   nullptr;
        bool                        _open       =   false;
        /// <summary>
        /// 当前绑定的节点(模型树选中项)
        /// </summary>
        Node                        _node;
        /// <summary>
        /// 节点上的动画组件
        /// </summary>
        std::vector<Animation>      _anims;
        std::vector<Row>            _rows;
        /// <summary>
        /// 动画行折叠状态
        /// </summary>
        std::map<Animation,bool>    _collapsed;
        /// <summary>
        /// 当前活动动画(添加关键帧/创建动画的目标)
        /// </summary>
        Animation                   _activeAnim;
        /// <summary>
        /// 选中的关键帧:轨道 → 关键帧索引
        /// </summary>
        std::map<KeyFrameTrack,std::set<size_t>>    _selKeys;
        /// <summary>
        /// 剪贴板
        /// </summary>
        std::vector<ClipTrack>      _clipboard;
        real                        _clipBaseTime   =   0;
        /// <summary>
        /// 帧率
        /// </summary>
        float                       _fps            =   30.0f;
        /// <summary>
        /// 一帧多少像素
        /// </summary>
        float                       _framePixel     =   8.0f;
        /// <summary>
        /// 可视区域左边界帧号
        /// </summary>
        double                      _viewLeftFrame  =   0.0;
        /// <summary>
        /// 行垂直滚动偏移
        /// </summary>
        float                       _scrollY        =   0.0f;
        /// <summary>
        /// 当前时间(秒)
        /// </summary>
        real                        _curTime        =   0.0;
        bool                        _playing        =   false;
        bool                        _loop           =   true;
        /// <summary>
        /// 画布几何(每帧更新)
        /// </summary>
        float                       _canvasX        =   0.0f;
        float                       _canvasY        =   0.0f;
        float                       _canvasW        =   0.0f;
        float                       _canvasH        =   0.0f;
        float                       _nameW          =   160.0f;
        float                       _timelineX      =   0.0f;
        float                       _rulerH         =   24.0f;
        float                       _rowH           =   22.0f;
        /// <summary>
        /// 拖拽状态
        /// </summary>
        DragMode                    _dragMode       =   DM_None;
        float                       _pressX         =   0.0f;
        float                       _pressY         =   0.0f;
        std::vector<DragTrack>      _dragTracks;
        std::vector<Animation>      _dragAnims;
        std::vector<real>           _dragOffsets;
        bool                        _panning        =   false;
        float                       _panStartX      =   0.0f;
        double                      _panStartView   =   0.0;
        /// <summary>
        /// 右键菜单上下文
        /// </summary>
        double                      _ctxFrame       =   0.0;
        int                         _ctxRow         =   -1;
        size_t                      _ctxKey         =   0;
        bool                        _ctxOnKey       =   false;
        /// <summary>
        /// 工具栏提示
        /// </summary>
        std::string                 _status;
    };
}
