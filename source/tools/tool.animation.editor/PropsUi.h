#pragma     once
#include    <QDoubleSpinBox>
#include    <QSpinBox>
#include    <QCheckBox>
#include    <QColor>
#include    <QTableWidget>
#include    <QHeaderView>

using   PropIndex   =   int;

class   QPainter;

/// <summary>
/// 编辑型 SpinBox 公共行为辅助:sub/mid/add 三区域绘制、hover 高亮、左键拖拽改值
/// 供 DoubleEditingSpinBox/IntEditingSpinBox 复用,消除重复实现
/// </summary>
class   SpinBoxEditHelper
{
public:
    /// <summary>
    /// 计算三区域矩形(sub=左方块, add=右方块, mid=中间文本区)
    /// </summary>
    void    getRects(int w, int h, QRect& subRect, QRect& midRect, QRect& addRect) const;
    /// <summary>
    /// 非焦点态绘制:isCursorIn 时绘制 sub/add 按钮及 hover 高亮,否则仅背景+文本
    /// </summary>
    void    paint(QPainter& painter, const QWidget* w, const QString& text) const;
    /// <summary>
    /// 按光标位置更新 hover 状态,返回状态是否变化(变化时需 update())
    /// </summary>
    bool    updateHover(const QPoint& pos, int w, int h);
    /// <summary>
    /// 清空 hover/光标在内状态(leaveEvent 用)
    /// </summary>
    void    resetHover();
    /// <summary>
    /// 拖拽时钳制光标位置于控件内,并处理左右出界回绕,返回修正后的位置
    /// </summary>
    QPoint  clampDragPos(const QPoint& curPos, const QWidget* w);
public:
    bool    _subHover       =   false;
    bool    _midHover       =   false;
    bool    _addHover       =   false;
    bool    _isCursorIn     =   false;
    QPoint  _leftDownPos;
    bool    _leftDown       =   false;
    bool    _dragging       =   false;
    QColor  _bgColor        =   QColor(200, 200, 200);
    QColor  _hoverColor     =   QColor(180, 180, 180);
};

class   DoubleEditingSpinBox : public QDoubleSpinBox
{
    Q_OBJECT
public:
    DoubleEditingSpinBox(QWidget* parent = nullptr);
public:
    inline void    setPropIndex(PropIndex propIndex)
    {
        _propIndex  =   propIndex;
    }

    inline const PropIndex& propIndex() const
    {
        return  _propIndex;
    }
    inline void     setBgColor(QColor color)
    {
        _helper._bgColor    =   color;
    }
    inline void     setHoverColor(QColor color)
    {
        _helper._hoverColor =   color;
    }
public:
    virtual void    focusInEvent(QFocusEvent* evt)         override;
    virtual void    focusOutEvent(QFocusEvent* evt)        override;
    virtual void    paintEvent(QPaintEvent* evt)           override;
    virtual void    leaveEvent(QEvent* event)              override;
    virtual void    enterEvent(QEnterEvent* event)          override;
    virtual void    mouseMoveEvent(QMouseEvent* event)     override;
    virtual void    mousePressEvent(QMouseEvent *event)    override;
    virtual void    mouseReleaseEvent(QMouseEvent *event)  override;
public slots:
    void    slotEditFinished();
signals:
    void    sigEditFinished(double beginValue, double endValue);
private:
    double              _beginValue =   0;
    PropIndex           _propIndex  =   -1;
    SpinBoxEditHelper   _helper;
};

class   IntEditingSpinBox : public QSpinBox
{
    Q_OBJECT
public:
    IntEditingSpinBox(QWidget* parent = nullptr);
public:
    inline void    setPropIndex(PropIndex propIndex)
    {
        _propIndex  =   propIndex;
    }

    inline const PropIndex& propIndex() const
    {
        return  _propIndex;
    }
    inline void     setBgColor(QColor color)
    {
        _helper._bgColor    =   color;
    }
    inline void     setHoverColor(QColor color)
    {
        _helper._hoverColor =   color;
    }
public:
    virtual void    focusInEvent(QFocusEvent* evt)         override;
    virtual void    focusOutEvent(QFocusEvent* evt)        override;
    virtual void    paintEvent(QPaintEvent* evt)           override;
    virtual void    leaveEvent(QEvent* event)              override;
    virtual void    enterEvent(QEnterEvent* event)          override;
    virtual void    mouseMoveEvent(QMouseEvent* event)     override;
    virtual void    mousePressEvent(QMouseEvent *event)    override;
    virtual void    mouseReleaseEvent(QMouseEvent *event)  override;
public slots:
    void    slotEditFinished();
signals:
    void    sigEditFinished(int beginValue, int endValue);
private:
    int                 _beginValue =   0;
    PropIndex           _propIndex  =   -1;
    SpinBoxEditHelper   _helper;
};

class   TrackCheckBox : public QCheckBox
{
    Q_OBJECT
public:
    TrackCheckBox(QWidget* parent = nullptr);
public:
    inline void    setPropIndex(PropIndex propIndex)
    {
        _propIndex  =   propIndex;
    }

    inline const PropIndex& propIndex() const
    {
        return  _propIndex;
    }
private:
    PropIndex       _propIndex  =   -1;
};

class   TableWidget : public QTableWidget
{
public:
    TableWidget(QWidget* parent)
        : QTableWidget(parent)
    { }

public:
    virtual QSize   sizeHint() const override
    {
        QSize   hint    =   QTableWidget::sizeHint();
        int     h       =   this->horizontalHeader()->height() * 2;

        for (int row = 0; row < qMin<int>(rowCount(), 5); row++)
        {
            h += rowHeight(row);
        }

        hint.setHeight(h);
        return  hint;
    }
};
