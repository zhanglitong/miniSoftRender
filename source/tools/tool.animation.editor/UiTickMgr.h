#pragma     once
#include    <QWidget>
#include    <QScrollBar>
#include    <QShortcut>
#include    <QMenu>
#include    "TimeSlider.h"

class   AnimationItem;
class   AnimationTree;
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
    void    setKeyRowHeight(int rowHeight);
    void    setTimeRowHeight(int height);
    void    setCurFrame(const int& frame, bool applyToAnim = true);
    void    toPreKeyframe();
    void    toFirstKeyframe();
    void    toNextKeyframe();
    void    toLastKeyframe();
    void    reset();
    void    updateScrollLength();
private:
    void    updateTimeLineRect();

    int     frameFromPos(int x);
    int     posFromFrame(int frame);
    int     rowFromPos(const QPoint& p) const;
    void    drawItem(QPainter& painter,AnimationItem*);
    void    drawTrack(QPainter& painter,AnimationItem*);
    void    drawAnimation(QPainter& painter,AnimationItem*);

    int     calcDeltaFrame(const int& p0, const int& p1) const;
public slots:
    void    slotScrollValueChanged(int value);
    void    slotDoPaint();
    void    slotDeleteKeyframes();
    void    slotCopyPasteKeyframes();
    void    slotPasteKeyframes();
    void    slotCopyKeyframes();
    void    slotSetInterpolate();
signals:
    void    sigUpdatePropsUi();
    void    sigCurFrameChanged(int);
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
    // 目前采用帧动画模式（其他软件优先的模式），1秒30帧
    int             _curFrame           =   0;
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

    int     _copyDeltaFrame = 0;


    QShortcut*  _shortcutCopy;
    QShortcut*  _shortcutPaste;
    QShortcut*  _shortcutCopyPaste;
    QShortcut*  _shortcutDelete;
};
