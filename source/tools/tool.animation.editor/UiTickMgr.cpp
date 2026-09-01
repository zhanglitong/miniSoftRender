#include    "UiTickMgr.h"
#include    <QPainter>
#include    <QMouseEvent>
#include    "AnimItem.h"
#include    <cmath>

QShortcut* REGIST_SHORTCUT(const std::string& str,QObject* parent)
{
    return  new QShortcut(QKeySequence(str.c_str()), parent);
}

UiTickMgr::UiTickMgr(QWidget* parent)
    : _pTree(nullptr), _rootItem(nullptr), QWidget(parent)
{
    _bar    =   nullptr;
    _menu   =   new QMenu(this);
    _menu->setTitle("关键帧编辑");

    _shortcutCopy       =   REGIST_SHORTCUT("Ctrl+C"        ,parent);
    _shortcutPaste      =   REGIST_SHORTCUT("Ctrl+V"        ,parent);
    _shortcutCopyPaste  =   REGIST_SHORTCUT("Ctrl+Shift+C"  ,parent);
    _shortcutDelete     =   REGIST_SHORTCUT("Delete"        ,parent);

    connect(_shortcutCopy, &QShortcut::activated, [&]()
    {
        auto    globaPos    =   QCursor::pos();
        auto    localPos    =   mapFromGlobal(globaPos);

        if (rect().contains(localPos))
        {
            slotCopyKeyframes();
        }
    });
    connect(_shortcutPaste, &QShortcut::activated, [&]()
    {
        auto    globaPos    =   QCursor::pos();
        auto    localPos    =   mapFromGlobal(globaPos);

        if (rect().contains(localPos))
        {
            slotPasteKeyframes();
        }
    });

    connect(_shortcutCopyPaste, &QShortcut::activated, [&]()
        {
            auto    globaPos    =   QCursor::pos();
            auto    localPos    =   mapFromGlobal(globaPos);

            if (rect().contains(localPos))
            {
                slotCopyPasteKeyframes();
            }
        });

    connect(_shortcutDelete, &QShortcut::activated, [&]()
        {
            auto    globaPos    =   QCursor::pos();
            auto    localPos    =   mapFromGlobal(globaPos);

            if (rect().contains(localPos))
            {
                slotDeleteKeyframes();
            }
        });

    QAction*    deleteAction        =   new QAction("删除", _menu);
    QAction*    copyPasteAction     =   new QAction("复制粘贴", _menu);
    QAction*    copyAction          =   new QAction("复制", _menu);
    QAction*    pasteAction         =   new QAction("粘贴", _menu);

    connect(deleteAction,       SIGNAL(triggered()), this, SLOT(slotDeleteKeyframes()));
    connect(copyPasteAction,    SIGNAL(triggered()), this, SLOT(slotCopyPasteKeyframes()));
    connect(pasteAction,        SIGNAL(triggered()), this, SLOT(slotPasteKeyframes()));
    connect(copyAction,         SIGNAL(triggered()), this, SLOT(slotCopyKeyframes()));

    _menu->addAction(copyAction);
    _menu->addAction(pasteAction);
    _menu->addAction(copyPasteAction);
    _menu->addAction(deleteAction);

    auto subMenu = new QMenu("插值", _menu);
    _menu->addMenu(subMenu);

    QAction*    linerAction = new QAction("线性", subMenu);
    QAction*    cubicAction = new QAction("离散", subMenu);
    subMenu->addAction(linerAction);
    subMenu->addAction(cubicAction);

    connect(linerAction, SIGNAL(triggered()), this, SLOT(slotSetInterpolate()));
    connect(cubicAction, SIGNAL(triggered()), this, SLOT(slotSetInterpolate()));
}

UiTickMgr::~UiTickMgr()
{
}

void    UiTickMgr::linkScrollBar(QScrollBar* bar)
{
    _bar    =   bar;
    updateScrollLength();

    connect(_bar, SIGNAL(valueChanged(int)), this, SLOT(slotScrollValueChanged(int)));
}

void    UiTickMgr::setAnimItem(AnimItem* pItem)
{
    _rootItem = pItem;
}

void    UiTickMgr::setAniTree(AniTree* pTree)
{
    _pTree  =   pTree;
}

void    UiTickMgr::setKeyRowHeight(int rowHeight)
{
    _keyRowHeight      =   rowHeight;
}

void    UiTickMgr::setTimeRowHeight(int height)
{
    _timeRowHeight      =   height;
    _timeSlider.setSize(QSize(24, _timeRowHeight - 10));
    updateTimeLineRect();
}

void    UiTickMgr::setCurFrame(const int& frame, bool applyToAnim)
{
    _curFrame   =   frame;
    update();
    emit sigCurFrameChanged(frame);
}



void    UiTickMgr::toPreKeyframe()
{
    int curFrame = _curFrame;
    int preFrame = _curFrame;

    for (auto itr = _rootItem->_drawKeyDatas.rbegin(); itr != _rootItem->_drawKeyDatas.rend(); ++itr)
    {
        auto frame = frameFromPos(itr->first);
        if (frame < curFrame)
        {
            preFrame    =   frame;
            break;
        }
    }
    
    if (preFrame != _curFrame)
    {
        setCurFrame(preFrame, false);
    }
}

void    UiTickMgr::toFirstKeyframe()
{
    if (_curFrame != _firstFrame)
    {
        setCurFrame(_firstFrame, false);
    }
}

void    UiTickMgr::toNextKeyframe()
{
    int curFrame  = _curFrame;
    int nextFrame = _curFrame;
    int distance = INT_MAX;

    for (auto& keyData : _rootItem->_drawKeyDatas)
    {
        auto frame = frameFromPos(keyData.first);

        if (frame > curFrame)
        {
            nextFrame   =   frame;
            break;
        }
    }
    if (nextFrame != _curFrame)
    {
        setCurFrame(nextFrame, false);
    }
}

void    UiTickMgr::toLastKeyframe()
{
    if (_curFrame != _lastFrame)
    {
        setCurFrame(_lastFrame, false);
    }
}

void    UiTickMgr::reset()
{
    _rowItem.clear();
    _isPressKeyframe            =   false;
    _isPress                    =   false;
    _isCopyPasting              =   false;
    _isMiddlePress              =   false;
    _preDeltaFrame              =   0;
    _isNeedUpdateKeyframeDatas  =   true;
}

void    UiTickMgr::updateScrollLength()
{
    if (_bar == nullptr)
        return;
    int pageSize    =   this->size().width();
    int max         =   static_cast<int>(std::ceil((_lastFrame + _frameFrequency) * _framePixel)) - pageSize;
    _bar->blockSignals(true);
    if (max <= 0)
    {
        _bar->setMaximum(0);
        _bar->setValue(0);
        _bar->setVisible(false);
    }
    else
    {
        _bar->setVisible(true);
        _bar->setMaximum(max);
        _bar->setPageStep(pageSize);
    }
    _bar->blockSignals(false);
}

void    UiTickMgr::updateTimeLineRect()
{
    _timeLineRect   =   QRect(QPoint(0,0), QPoint(size().width(), _timeRowHeight));
}

int     UiTickMgr::frameFromPos(int x)
{
    if (x >= 0 && x <= size().width())
    {
        double frame = (static_cast<double>(x) - _beginPixel) / _framePixel + _beginFrame;
        return static_cast<int>(std::round(frame));
    }
    return -1;
}

int     UiTickMgr::posFromFrame(int frame)
{
    return static_cast<int>(std::round((frame - _beginFrame) * _framePixel + _beginPixel));
}

int     UiTickMgr::rowFromPos(const QPoint& p) const
{
    return (p.y() - _timeRowHeight) / _keyRowHeight;
}

void    UiTickMgr::updateItemKeyDatas(AnimItem* pItem, int& beginRow, int& curRow, const bool& isExpanded)
{
    if (isExpanded)
    {
        pItem->setRow(curRow - beginRow);
    }
    else
    {
        pItem->setRow(-1);
    }

    if (pItem->getRow() >= 0)
    {
        _rowItem[pItem->getRow()] = pItem;
    }

    pItem->_drawKeyDatas.clear();

    auto pTrack = pItem->animKeyframeTrack();
    
    if (isExpanded)
    {
        curRow++;
    }

    for (int i = 0; i < pItem->rowCount(); ++i)
    {
        AnimItem*   pChild = (AnimItem*)pItem->child(i);
        updateItemKeyDatas(pChild, beginRow, curRow, isExpanded && pItem->isExpanded());
        for (auto& var : pChild->_drawKeyDatas)
        {
            pItem->_drawKeyDatas[var.first] |= var.second;
        }
    }
}

void    UiTickMgr::drawItemKeyDatas(QPainter& painter)
{
    for (auto& var : _rowItem)
    {
        auto& row   = var.first;
        auto& pItem = var.second;

        auto&   drawRow =   pItem->getRow();

        if (pItem->isSelected())
        {
            int     beginY  =   _timeRowHeight + drawRow * _keyRowHeight;
            QRect   rect    =   QRect(QPoint(0, beginY), QPoint(size().width(), beginY + _keyRowHeight));
            painter.fillRect(rect, QColor(52, 135, 255, 100));
        }

        for (auto& dKey : pItem->_drawKeyDatas)
        {
            auto& drawX = dKey.first;
            QPoint  pos = QPoint(drawX, _keyRowHeight / 2 + drawRow * _keyRowHeight + _timeRowHeight);
            if (dKey.second)
            {
                painter.setBrush(Qt::yellow);
            }
            else
            {
                painter.setBrush(Qt::gray);
            }
            painter.setPen(Qt::black);
            painter.drawEllipse(pos, _keyWidth, _keyWidth);
        }
    }
}

int     UiTickMgr::calcDeltaFrame(const int& p0, const int& p1) const
{
    return static_cast<int>(std::round(static_cast<double>(p1 - p0) / _framePixel));
}

void    UiTickMgr::slotScrollValueChanged(int value)
{
    if (value < 0)
    {
        return;
    }

    double frameOffset = static_cast<double>(value) / _framePixel;
    _beginFrame = static_cast<int>(std::ceil(frameOffset));
    _beginPixel = _beginFrame * _framePixel - value;

    _isNeedUpdateKeyframeDatas = true;
    update();
}

void    UiTickMgr::slotDoPaint()
{
    _isNeedUpdateKeyframeDatas = true;
    update();
}

void    UiTickMgr::slotDeleteKeyframes()
{
   
}

void    UiTickMgr::slotCopyPasteKeyframes()
{
   
}

void    UiTickMgr::slotPasteKeyframes()
{
}

void    UiTickMgr::slotCopyKeyframes()
{
   
}

void    UiTickMgr::slotSetInterpolate()
{
}

void    UiTickMgr::paintEvent(QPaintEvent* event)
{
    QSize size = this->size();
    QPainter painter(this);

    painter.setPen(QPen(QColor(0,0,0,255)));
    painter.drawRect(0, 0, size.width(), size.height());
    painter.drawLine(0, _timeRowHeight, size.width(), _timeRowHeight);
    painter.fillRect(0, 0, size.width(), _timeRowHeight, QColor(150, 150, 150, 255));
    painter.fillRect(0, _timeRowHeight, size.width(), size.height(), QColor(255, 255, 255, 255));
    // 画时间线
    double beginX  =   _beginPixel;
    int beginFrame =   _beginFrame;
    int yushu      =   _beginFrame % _frameFrequency;
    if (yushu)
    {
        beginX      +=   (_frameFrequency - yushu) * _framePixel;
        beginFrame  =   _beginFrame - yushu  + _frameFrequency;
    }

    int lastX = static_cast<int>(std::round((_lastFrame - _beginFrame) * _framePixel + _beginPixel));
    if (lastX < size.width())
    {
        painter.fillRect(lastX, _timeRowHeight, size.width(), size.height(), QColor(200, 200, 200, 255));
    }
    int firstX = static_cast<int>(std::round((_firstFrame - _beginFrame) * _framePixel + _beginPixel));
    if (firstX > 0)
    {
        painter.fillRect(0, _timeRowHeight, firstX, size.height(), QColor(200, 200, 200, 255));
    }

    for (;beginX <= size.width() + _frameFrequency * _framePixel; beginFrame += _frameFrequency)
    {
        // 画时间
        painter.setPen(QPen(QColor(255,255,255,255)));
        int drawX = static_cast<int>(std::round(beginX));
        QRect   numRect = {QPoint(drawX - 20, 0), QPoint(drawX + 20, _timeRowHeight)};
        painter.drawText(numRect, Qt::AlignCenter, std::to_string(beginFrame).c_str());
        // 画线
        QPoint  pointA = QPoint(drawX, _timeRowHeight);
        QPoint  pointB = QPoint(drawX, size.height());
        if ((beginFrame / _frameFrequency) % 2)
        {
            painter.setPen(QPen(QColor(128,128,128,128)));
        }
        else
        {
            painter.setPen(QPen(QColor(0,0,0,128)));
        }
        
        painter.drawLine(pointA, pointB);

        beginX += _frameFrequency * _framePixel;
    }

    if (_curFrame >= _beginFrame)
    {
        beginX = (_curFrame - _beginFrame) * _framePixel + _beginPixel;
        if (beginX <= size.width())
        {
            _timeSlider.setPos(QPoint(static_cast<int>(std::round(beginX)), _timeRowHeight / 2));
            _timeSlider.paint(painter, size.width() - _timeRowHeight, std::to_string(_curFrame).c_str());
        }
    }

    if (_isNeedUpdateKeyframeDatas)
    {
        _isNeedUpdateKeyframeDatas = false;
    }

    // 绘制关键帧
    drawItemKeyDatas(painter);
    // 绘制框选框
    if (_isPress && !_isPressKeyframe)
    {
        QPen    pen(QColor(0, 162, 232, 255));
        pen.setStyle(Qt::DashLine);
        painter.setPen(pen);
        painter.setBrush(QColor(200,200,200,60));
        painter.drawRect(QRect(_pressPos, _curPos));
    }
}

void    UiTickMgr::mousePressEvent(QMouseEvent* event)
{
    switch (event->button())
    {
    case Qt::LeftButton:
        {
            if (_timeLineRect.contains(event->pos()))
            {
                auto frame = frameFromPos(event->pos().x());
           
                setCurFrame(frame);
                _isDragTimeSlider = true;
            }
        }
        break;
    case Qt::RightButton:
        {}
        break;
    case Qt::MiddleButton:
        {
            _isMiddlePress      =   true;
            _preDeltaFrame      =   0;
            _middlePos          =   event->pos();
            _middleHScroolValue = _bar->value();
        }
        break;
    default:
        break;
    }
    QWidget::mousePressEvent(event);
}

void    UiTickMgr::mouseReleaseEvent(QMouseEvent* event)
{
    switch (event->button())
    {
    case Qt::LeftButton:
    {
        if (_isDragTimeSlider)
        {
            _isDragTimeSlider   =   false;
        }

        if (_isPress)
        {
            // 如果点到了关键帧，拖动移动关键帧
            if (_isPressKeyframe)
            {
                if (_pressPos == event->pos())
                {
                    // 例子：已有多个关键帧被选中，press到已选中的，但是没有鼠标唯一，这时点选到当前，取消其他选中
                    if (_isPressSelected)
                    {
                        _isPressSelected = false;
                    }
                }
                else
                {
                    // 如果有帧变化，则添加移动命令
                    int deltaFrame = calcDeltaFrame(_pressPos.x(), event->pos().x());
                    
                }
            }
            _isPress            =   false;
        }
    }break;
    case Qt::RightButton:
    {

    }break;
    case Qt::MiddleButton:
    {
        _isMiddlePress  =   false;
    }break;
    default:
        break;
    }

    QWidget::mouseReleaseEvent(event);
}

void    UiTickMgr::mouseDoubleClickEvent(QMouseEvent* event)
{
    QWidget::mouseDoubleClickEvent(event);
}

void    UiTickMgr::mouseMoveEvent(QMouseEvent* event)
{
    _curPos = event->pos();
    if (_isCopyPasting)
    {
        auto deltaX = event->pos().x() - _menuPos.x();
        _copyDeltaFrame = static_cast<int>(std::round(static_cast<double>(deltaX) / _framePixel));
        _isNeedUpdateKeyframeDatas = true;
        update();
    }
    else if (_isDragTimeSlider)
    {
        QPoint  mousePos = event->pos();
        if (mousePos.x() >= 0 && mousePos.x() <= size().width())
        {
            //int    remainder = mousePos.x() % _framePixel;

            //int    intergFramePixel =  mousePos.x() - remainder;
            //if (remainder >= _framePixel / 2)
            //{
            //    intergFramePixel += _framePixel;
            //    if (intergFramePixel > size().width())
            //    {
            //        goto ret;
            //    }
            //}

            //int    frame = intergFramePixel / _framePixel + _beginFrame;
            //setCurFrame(frame);

            int frame = frameFromPos(mousePos.x());
            if (frame == -1)
            {
                goto ret;
            }
            else
            {
                setCurFrame(frame);
            }
        }
    }
    else if (_isPress)
    {
        if (_isPressKeyframe)
        {
            int deltaPos   = event->pos().x() - _pressPos.x();
            int deltaFrame = static_cast<int>(std::round(static_cast<double>(deltaPos) / _framePixel));

            int curDeltaFrme = deltaFrame - _preDeltaFrame;
           
            _isNeedUpdateKeyframeDatas = true;
            update();
            _preDeltaFrame  =   deltaFrame;
        }
        else
        {
            _isNeedUpdateKeyframeDatas = true;
            // 框选
            update();
        }
    }
    else if (_isMiddlePress)
    {
        int deltaPixel  = event->pos().x() - _middlePos.x();
        int curValue    = _middleHScroolValue - deltaPixel;
        curValue = std::clamp(curValue, 0, _bar->maximum());
        _bar->setValue(curValue);
    }

    ret:
    QWidget::mouseMoveEvent(event);
}

void    UiTickMgr::wheelEvent(QWheelEvent* event)
{
 
    QWidget::wheelEvent(event);
}

void    UiTickMgr::resizeEvent(QResizeEvent* event)
{
    updateScrollLength();

    updateTimeLineRect();
    QWidget::resizeEvent(event);
}

void    UiTickMgr::contextMenuEvent(QContextMenuEvent* event)
{
    _menuPos    =   event->pos();
    _menu->exec(event->globalPos());
    QWidget::contextMenuEvent(event);
}
