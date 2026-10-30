/// <summary>
/// FETimeLine
/// ImGui 版时间线编辑控件,参考 Qt 版 UiTickMgr(动画编辑器)的交互:
///   - 数据直接绑定选中节点的 FEAnimation / FEAnimClip / FEKeyFrameTrack;
///   - 标尺 + 播放头,点击/拖动标尺跳帧,Ctrl+滚轮以鼠标为锚点缩放,中键拖动平移;
///   - 动画行显示播放范围块(可拖动改 offset),轨道行显示关键帧圆点;
///   - 关键帧单击选中、Ctrl/Shift 追加或取消、拖动改时间、框选;
///   - 右键菜单:复制/粘贴/删除/插值(贝塞尔/线性/离散)/在此处添加关键帧;
///   - 播放控制:正/逆向播放、上一/下一关键帧、循环、播放速率、当前帧与总帧数;
///   - 工具栏分两组:动画树上方为搜索栏(回车执行,支持大小写/全匹配),
///     时间线上方为播放控制图标按钮;
///   - 所有数据修改经 FEUndoStack,支持撤销重做。
/// 窗口是屏幕底部固定面板,由 Ribbon「设置 → 动画 → 时间线」开关。
/// </summary>

#pragma     once

#include    "FEUiMetrics.hpp"

#include    "../../../inc/animation/FEAnimation.hpp"
#include    "../../../inc/animation/FEKeyFrameTrack.hpp"
#include    "../../../inc/FEUndoStack.hpp"
#include    "../../../inc/node/FENode.hpp"

#include    "FEAnimationTree.hpp"

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
        /// <param name="iconFont">图标字体,可为 nullptr(为空时按钮回退为文字)</param>
        void    draw(ImFont* iconFont);
        /// <summary>
        /// 设置动画帧率(默认 30)
        /// </summary>
        void    setFps(float fps);
        float   fps() const         {   return  _fps;   }
        /// <summary>
        /// 是否正在播放(时间线自身推进时间);播放中禁止编辑节点姿态
        /// </summary>
        bool    isPlaying() const   {   return  _playDir != 0;  }
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
        /// <summary>
        /// 指定属性在当前时间是否有关键帧(供属性面板显示勾选状态)。
        /// 整组轨道(TRANSFORM_XYZ 等)视为覆盖其各轴。
        /// </summary>
        /// <param name="prop">属性索引(如 PROP_TRANSFORM_X)</param>
        /// <returns>true 当前时间有关键帧</returns>
        bool    hasKeyframe(PropIndex prop) const;
        /// <summary>
        /// 切换指定属性在当前时间的关键帧:没有则添加,已有则移除。
        /// </summary>
        /// <param name="prop">属性索引</param>
        /// <returns>切换后该属性是否有关键帧</returns>
        bool    toggleKeyframe(PropIndex prop);
    protected:
        /// <summary>
        /// 拖拽关键帧时保存的轨道原始数据
        /// </summary>
        struct  DragTrack
        {
            KeyFrameTrack           _track;
            reals                   _times;
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
            /// <summary>
            /// 拖动动画树与时间线之间的分割条
            /// </summary>
            DM_Splitter,
            /// <summary>
            /// 拖动垂直滚动条(动画树与时间线共用)
            /// </summary>
            DM_VScroll,
            /// <summary>
            /// 拖动水平滚动条
            /// </summary>
            DM_HScroll,
        };
    protected:
        /// <summary>
        /// 选中节点变化时同步动画列表与编辑状态
        /// </summary>
        void    ensureNode();
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
        /// 工具栏(两组:动画树上方搜索栏 / 时间线上方播放控制)
        /// </summary>
        void    drawToolbar();
        /// <summary>
        /// 图标按钮;未加载图标字体时回退为文字按钮
        /// </summary>
        /// <param name="glyph">字体图标码位</param>
        /// <param name="text">无图标字体时的替代文字</param>
        /// <param name="tip">悬停提示</param>
        /// <param name="active">是否处于按下态(高亮)</param>
        bool    iconButton(const char* glyph,const char* text,const char* tip,bool active = false);
        /// <summary>
        /// 旋转 180° 的图标按钮(逆向播放用:即正向播放图标旋转半圈)
        /// </summary>
        bool    iconButtonRotated(const char* glyph,const char* text,const char* tip,bool active = false);
        /// <summary>
        /// 图标按钮实现:自绘图标,尺寸与标准控件一致
        /// </summary>
        /// <param name="rotate">是否把图标旋转 180°</param>
        bool    iconButtonImpl(const char* glyph,const char* text,const char* tip,bool active,bool rotate);
        /// <summary>
        /// 开始正向(1)/逆向(-1)播放;同一方向再次调用即暂停
        /// </summary>
        void    startPlay(int dir);
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
        /// 每帧更新滚动范围与滚动条几何(内容超出可视区时才显示)
        /// </summary>
        void    updateScrollMetrics();
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
        /// <summary>
        /// 删除单个动画(动画树名称列右键菜单):从动画系统与所属节点上摘除,走撤销栈
        /// </summary>
        /// <param name="anim">要删除的动画</param>
        void    removeAnimation(Animation anim);
        /// <summary>
        /// 创建播放器(动画树空白处右键菜单):名字自动取不重复的序号
        /// </summary>
        void    createPlayback();
        /// <summary>
        /// 删除播放器(动画树播放器行右键菜单):只从动画系统摘除,节点上的动画组件保留
        /// </summary>
        /// <param name="playback">要删除的播放器</param>
        void    removePlayback(Playback playback);
        /// <summary>
        /// 重命名播放器(动画树播放器行右键菜单)
        /// </summary>
        /// <param name="playback">目标播放器</param>
        /// <param name="newName">新名字</param>
        void    renamePlayback(Playback playback,const String& newName);
    protected:
        FEScene*                    _scene      =   nullptr;
        FEUndoStack*                _undoStack  =   nullptr;
        FEModelTree*                _modelTree  =   nullptr;
        /// <summary>
        /// 工具栏图标字体(来自 FEEditorUi 的 Font Awesome)
        /// </summary>
        ImFont*                     _iconFont   =   nullptr;
        bool                        _open       =   false;
        /// <summary>
        /// 当前绑定的节点(模型树选中项)
        /// </summary>
        Node                        _node;
        /// <summary>
        /// 动画树(动画列表/行/折叠/搜索)
        /// </summary>
        FEAnimationTree             _tree;
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
        /// 面板高度(<=0 表示使用默认高度);上边缘可拖动调整
        /// </summary>
        float                       _panelH         =   -1.0f;
        /// <summary>
        /// 上边缘拖动调整高度时的起始高度与鼠标 y
        /// </summary>
        float                       _resizeStartH   =   0.0f;
        float                       _resizeStartY   =   0.0f;
        /// <summary>
        /// 当前时间(秒)
        /// </summary>
        real                        _curTime        =   0.0;
        /// <summary>
        /// 播放方向:0 停止,1 正向,-1 逆向
        /// </summary>
        int                         _playDir        =   0;
        bool                        _loop           =   true;
        /// <summary>
        /// 播放速率倍率(0.1 ~ 60)
        /// </summary>
        float                       _speed          =   1.0f;
        /// <summary>
        /// 时间线总帧数(播放/循环的结束帧)
        /// </summary>
        int                         _totalFrames    =   150;
        /// <summary>
        /// 总帧数是否已被用户手动指定;未指定时自动覆盖动画内容范围
        /// </summary>
        bool                        _totalManual    =   false;
        /// <summary>
        /// 画布几何(每帧更新)
        /// </summary>
        float                       _canvasX        =   0.0f;
        float                       _canvasY        =   0.0f;
        float                       _canvasW        =   0.0f;
        float                       _canvasH        =   0.0f;
        /// <summary>
        /// 动画树(名称列)宽度,可由分割条拖动调整
        /// </summary>
        float                       _nameW          =   ui::kTimelineNameWidth;
        /// <summary>
        /// 动画树与时间线之间的间隙,分割条绘制在该间隙内
        /// </summary>
        float                       _splitterW      =   ui::kTimelineSplitterWidth;
        float                       _timelineX      =   0.0f;
        float                       _rulerH         =   ui::kTimelineRulerHeight;
        /// <summary>
        /// 行高(不含行间距,与模型树一致)
        /// </summary>
        float                       _rowH           =   ui::kTimelineRowHeight;
        /// <summary>
        /// 行步进 = 行高 + 行间距,所有行位置/滚动/命中计算都用它
        /// </summary>
        float                       _rowStep        =   ui::kTimelineRowStep;
        /// <summary>
        /// 滚动条几何(每帧由 updateScrollMetrics 更新;仅内容超出可视区时显示)
        /// </summary>
        bool                        _vScroll        =   false;
        bool                        _hScroll        =   false;
        float                       _vTrackTop      =   0.0f;
        float                       _vTrackH        =   0.0f;
        float                       _vThumbTop      =   0.0f;
        float                       _vThumbH        =   0.0f;
        float                       _hTrackLeft     =   0.0f;
        float                       _hTrackW        =   0.0f;
        float                       _hThumbLeft     =   0.0f;
        float                       _hThumbW        =   0.0f;
        /// <summary>
        /// 垂直最大滚动偏移 / 水平最大左边界帧
        /// </summary>
        float                       _scrollMax      =   0.0f;
        double                      _viewMax        =   0.0;
        /// <summary>
        /// 滚动条拖动时鼠标在滑块内的偏移
        /// </summary>
        float                       _sbGrab         =   0.0f;
        /// <summary>
        /// 拖拽状态
        /// </summary>
        DragMode                    _dragMode       =   DM_None;
        float                       _pressX         =   0.0f;
        float                       _pressY         =   0.0f;
        std::vector<DragTrack>      _dragTracks;
        Animations                  _dragAnims;
        reals                       _dragOffsets;
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
        /// 名称列右键选中的动画(为空表示右键的是其它行)
        /// </summary>
        Animation                   _ctxAnim;
        /// <summary>
        /// 动画行右键菜单待打开(下一帧在 handleContextMenu 中打开,避免当帧打开即取不到菜单项)
        /// </summary>
        bool                        _ctxAnimOpen   =   false;
        /// <summary>
        /// 名称列右键选中的播放器(为空表示右键的不是播放器行)
        /// </summary>
        Playback                    _ctxPlayback;
        /// <summary>
        /// 播放器行右键菜单待打开
        /// </summary>
        bool                        _ctxPlaybackOpen    =   false;
        /// <summary>
        /// 动画树空白处右键:显示"创建播放器"菜单
        /// </summary>
        bool                        _ctxCreatePlaybackOpen  =   false;
        /// <summary>
        /// 重命名播放器弹窗
        /// </summary>
        Playback                    _renamePlayback;
        bool                        _renamePlaybackOpen     =   false;
        bool                        _renamePlaybackFocus    =   false;
        char                        _renamePlaybackBuf[ui::kRenameBufSize]  =   {   0   };
        /// <summary>
        /// 工具栏提示
        /// </summary>
        std::string                 _status;
    };
}
