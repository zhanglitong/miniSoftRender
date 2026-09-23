#pragma     once
#include    <QWidget>
#include    <QScrollBar>
#include    <QShortcut>
#include    <QMenu>
#include    <QTimer>
#include    <QUndoStack>
#include    <QColor>
#include    <map>
#include    "TimeSlider.h"


class   AnimationItem;
class   AnimationTree;
namespace   FE { class QtTree; }
namespace   FE { class FEKeyFrameTrack; }
using   QtTree  =   FE::QtTree;

/// <summary>
/// 鼠标命中测试结果
/// </summary>
enum    HitType { HT_None, HT_Keyframe, HT_Block, HT_Empty };
struct  HitResult
{
    HitType         type    =   HT_None;
    AnimationItem*  item    =   nullptr;
    size_t          keyIndex=   0;
};

class   UiTickMgr : public QWidget
{
    Q_OBJECT
public:
public:
    UiTickMgr(QWidget* parent);
    ~UiTickMgr();
public:
    void    linkScrollBar(QScrollBar* bar);
    void    setAniTree(AnimationTree* pTree);
    void    setModelTree(QtTree* pTree);
    void    setUndoStack(QUndoStack* stack) { _undoStack = stack; }
    void    setKeyRowHeight(int rowHeight);
    void    setTimeRowHeight(int height);
    void    setCurFrame(const int& frame, bool applyToAnim = true);
    /// 获取当前时间线时间(秒)
    double  curTime() const  { return _curFrame / double(_fps); }
   
    void    toPreKeyframe();
    void    toFirstKeyframe();
    void    toNextKeyframe();
    void    toLastKeyframe();
    void    reset();
    void    updateScrollLength();
private:
    void    updateTimeLineRect();

    int     frameFromPos(int x);
    /// <summary>
    /// 屏幕坐标转换成时间线时间
    /// </summary>
    /// <param name="x"></param>
    /// <returns></returns>
    double  timeFromPos(int x);
    int     posFromFrame(int frame);
    int     rowFromPos(const QPoint& p) const;
    /// <summary>
    /// 绘制所有关键帧，只是快捷展示
    /// </summary>
    /// <param name="painter"></param>
    /// <param name=""></param>
    void    drawKeyframeTimeline(QPainter& painter,AnimationItem*);
    void    drawItem(QPainter& painter,AnimationItem*);
    void    drawTrack(QPainter& painter,AnimationItem*);
    void    drawAnimation(QPainter& painter,AnimationItem*);
    /// <summary>
    /// 绘制 FENode 所有动画的总纲(矩形范围,全部画到该节点所在行)
    /// </summary>
    void    drawNodeAnimations(QPainter& painter,AnimationItem*);
    int     calcDeltaFrame(const int& p0, const int& p1) const;
    /// <summary>
    /// 鼠标命中测试: 检测点击的是关键帧、范围块还是空白区域
    /// </summary>
    HitResult   hitTest(const QPoint& pos);
    /// <summary>
    /// 遍历动画树中所有可见 Track 的关键帧,收集帧号集合
    /// </summary>
    void    collectAllKeyframeFrames(std::vector<int>& frames);
    /// <summary>
    /// 清除动画树中所有选择状态(FLAG_SELECTED + track user objects)
    /// </summary>
    void    clearSelection();
    /// <summary>
    /// 框选: 遍历动画树,选中范围内的关键帧
    /// </summary>
    void    boxSelect(const QRect& rect);
    /// <summary>
    /// 遍历动画树,对每个 item 执行回调
    /// </summary>
    void    traverseTree(std::function<void(AnimationItem*)> cb);
    /// <summary>
    /// 在 track 的 value object 上设置 UintsObject 选择标记
    /// </summary>
    static  void    setTrackKeySelection(FE::FEKeyFrameTrack* track, size_t keyIndex, bool selected);
    /// <summary>
    /// 创建/重置 track 的选择标记数组(全 0)
    /// </summary>
    static  void    initTrackKeySelection(FE::FEKeyFrameTrack* track);
    /// <summary>
    /// 查询关键帧是否被选中
    /// </summary>
    static  bool    isKeyframeSelected(FE::FEKeyFrameTrack* track, size_t keyIndex);
    /// <summary>
    /// 清除 track 的选择标记
    /// </summary>
    static  void    clearTrackKeySelection(FE::FEKeyFrameTrack* track);
public slots:
    void    slotScrollValueChanged(int value);
    void    slotDoPaint();
    void    slotDeleteKeyframes();
    void    slotCopyPasteKeyframes();
    void    slotPasteKeyframes();
    void    slotCopyKeyframes();
    void    slotSetInterpolate();
    void    slotPlayToNextFrame();
    /// <summary>
    /// 在当前帧添加关键帧,将选中节点的属性值写入到动画轨道
    /// 流程:
    ///     1. 从动画树(AnimationTree)获取当前选中项,校验是否为动画对象(FEAnimation)
    ///     2. 如果没有选中动画或选中的不是动画对象,提示用户选择动画对象
    ///     3. 如果动画 clip 没有 track,先创建默认轨道(Position/Scale/Rotation),再添加关键帧
    ///     4. 对当前动画的所有 track 增加/插入关键帧
    /// </summary>
    void    slotAddKeyframe();

signals:
    void    sigUpdatePropsUi();
    void    sigCurFrameChanged(double);
    void    sigKeyframesChanged();
public:
    void    paintEvent(QPaintEvent *event)              override ;
    void    mousePressEvent(QMouseEvent *event)         override ;
    void    mouseReleaseEvent(QMouseEvent *event)       override ;
    void    mouseDoubleClickEvent(QMouseEvent *event)   override ;
    void    mouseMoveEvent(QMouseEvent *event)          override ;
    void    wheelEvent(QWheelEvent *event)              override ;
    void    resizeEvent(QResizeEvent *event)            override ;
    void    contextMenuEvent(QContextMenuEvent* event)  override ;
    void    keyPressEvent(QKeyEvent* evt)               override ;
    void    keyReleaseEvent(QKeyEvent* evt)             override ;
public:
    QMenu*          _menu;
    QScrollBar*     _bar;
    QTimer*         _timer;
    /// <summary>
    /// 动画播放的帧率
    /// </summary>
    float           _fps                =   30;
    // 目前采用帧动画模式（其他软件优先的模式），1秒30帧
    int             _curFrame           =   0;
    double          _curTime            =   0;
    // 每行的高度，由动画树的itemHeight决定
    int             _keyRowHeight       =   24;
    int             _timeRowHeight      =   32;
    int             _keyWidth           =   5;
    // 一帧对应多少像素
    float           _framePixel         =   8;
    /// <summary>
    /// 关键点的大小
    /// </summary>
    int             _pointPixel         =   16;

    /// 背景块颜色(灰色半透明填充 + 深灰边框)
    static inline const QColor _blockColor   = QColor(128, 128, 128, 100);
    static inline const QColor _blockBorder  = QColor(80,  80,  80,  200);
    /// 关键帧点颜色(深灰半透明填充 + 深灰边框)
    static inline const QColor _dotColor     = QColor(64,  64,  64,  180);
    static inline const QColor _dotBorder    = QColor(40,  40,  40,  220);
    /// 启用态关键帧点颜色(深绿色填充 + 绿色边框)
    static inline const QColor _dotColorEn   = QColor(55,  125, 34, 180);
    static inline const QColor _dotBorderEn  = QColor(100, 200, 100, 220);

    // 每隔几帧画一条时间线
    int             _frameFrequency     =   10;

    // 开始绘制的帧
    int             _beginFrame         =   0;
    double          _beginPixel         =   0.0;
    // 第一帧
    int             _firstFrame         =   0;
    // 最后帧
    int             _lastFrame          =   150;
    int             _pixelLenth         =   _lastFrame * _framePixel;

    TimeSlider      _timeSlider;
    QRect           _timeLineRect;
    AnimationTree*  _pTree;
    QtTree*         _pModelTree =   nullptr;
    bool            _isDragTimeSlider   =   false;
    bool            _isPressKeyframe    =   false; // 是否点击到关键帧
    bool            _isPressSelected    =   false; // 否是点击到已经选择的关键帧
    bool            _isPress            =   false; // 鼠标左键是否按下
    bool            _isCopyPasting      =   false;// 是否正在复制粘贴
    bool            _isMiddlePress      =   false;
    QPoint          _menuPos;
    QPoint          _pressPos;
    QPoint          _middlePos;
    int             _middleHScroolValue;
    QPoint          _curPos;
    int             _preDeltaFrame = 0;
    bool            _isNeedUpdateKeyframeDatas = false;

    int             _copyDeltaFrame = 0;


    QShortcut*  _shortcutCopy;
    QShortcut*  _shortcutPaste;
    QShortcut*  _shortcutCopyPaste;
    QShortcut*  _shortcutDelete;

    QUndoStack*     _undoStack  =   nullptr;

    /// 拖动关键帧时保存原始时间(按 track 分组, 含动画 offset)
    struct  DragOrigData
    {
        std::vector<double>  times;
        double               offset  =   0;
    };
    std::map<FE::FEKeyFrameTrack*, DragOrigData> _dragOrigTimes;
    /// 拖动范围块时的原始 offset
    double          _dragOrigOffset  =   0;
    /// 正在拖动的范围块所属 item(FENode 或 FEAnimation)
    AnimationItem*  _dragBlockItem   =   nullptr;
    /// 选中高亮颜色
    static inline const QColor _selColor   = QColor(0, 162, 232, 200);
    static inline const QColor _selBorder  = QColor(255, 255, 255, 220);
};
