#include    "UiTickMgr.h"
#include    <QPainter>
#include    <QHeaderView>
#include    <QMouseEvent>
#include    <functional>
#include    "AnimationItem.h"
#include    "AnimationTree.h"
#include    "QtTree.h"
#include    "animation/FEAnimation.hpp"
#include    "animation/FEAnimationHelper.hpp"
#include    "animation/FEAnimationSys.hpp"
#include    "graphic/FEScene.h"
#include    "node/FENode.hpp"
#include    "FEMathUtil.hpp"
#include    "FEEulerObject.hpp"
#include    <cmath>

QShortcut* REGIST_SHORTCUT(const std::string& str,QObject* parent)
{
    return  new QShortcut(QKeySequence(str.c_str()), parent);
}


inline  FE::real    time2Pixel(FE::real tm,float pixel = 8,float fps = 30)
{
    return  tm * pixel * fps;
}

UiTickMgr::UiTickMgr(QWidget* parent)
    :  QWidget(parent)
    , _pTree(nullptr)
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

    /// 创建定时器,在 initEngine 完成后启动
    _timer = new QTimer(this);
    connect(_timer, &QTimer::timeout, this, [this]()
    {
        if (_curFrame >= _lastFrame)
        {
            _curFrame = _lastFrame;
            _timer->stop();
            return;
        }
        ++_curFrame;
        setCurFrame(_curFrame);
    });
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

void    UiTickMgr::setAniTree(AnimationTree* pTree)
{
    _pTree  =   pTree;
}

void    UiTickMgr::setModelTree(QtTree* pTree)
{
    _pModelTree =   pTree;
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
    _curFrame       =   frame;
    double  time    =   frame/_fps;
    update();
    emit sigCurFrameChanged(time);
}


void    UiTickMgr::toPreKeyframe()
{
    int     curFrame    =   _curFrame;
    int     preFrame    =   _curFrame;

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
    int curFrame    =   _curFrame;
    int nextFrame   =   _curFrame;
    int distance    =   INT_MAX;

    /// for (auto& keyData : _rootItem->_drawKeyDatas)
    /// {
    ///     auto frame = frameFromPos(keyData.first);
    /// 
    ///     if (frame > curFrame)
    ///     {
    ///         nextFrame   =   frame;
    ///         break;
    ///     }
    /// }
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
    _isPressKeyframe            =   false;
    _isPress                    =   false;
    _isCopyPasting              =   false;
    _isMiddlePress              =   false;
    _preDeltaFrame              =   0;
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
double  UiTickMgr::timeFromPos(int x)
{
    if (x >= 0 && x <= size().width())
    {
        double  frame   =   (static_cast<double>(x) - _beginPixel) / _framePixel + _beginFrame;
        return  frame / _fps;
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

void    UiTickMgr::drawItem(QPainter& painter,AnimationItem* item)
{
    if (item == nullptr)
        return;
    if (item->hasChildren() && !_pTree->isExpand(item))
        return;
    auto    anim    =   item->object()->cast<FEAnimation>();
    if (anim != nullptr)
    {
        drawAnimation(painter,item);
    }
    auto    track   =   item->object()->cast<FEKeyFrameTrack>();
    if (track)
    {
        drawTrack(painter,item);
    }
    int     cnt     =   item->rowCount();
    for (int r = 0 ; r < cnt ; ++ r)
    {
        auto cItem  =   (AnimationItem*)item->child(r,0);
        drawItem(painter,cItem);
    }
}

void    UiTickMgr::drawTrack(QPainter& painter,AnimationItem* item)
{
    auto    track   =   item->object()->cast<FEKeyFrameTrack>();
    if (track == nullptr)
        return;

    int     xOffset =   _bar ? _bar->value() : 0;
    QRect   rect    =   _pTree->visualRect(item->index());
    int     cnt     =   item->rowCount();
    int     rowH    =   rect.height();
    auto    rng     =   track->range();
    /// 计算从哪开始，到哪里结束
    real    xStart  =   time2Pixel(rng.x,_framePixel ,_fps) - xOffset;
    real    width   =   time2Pixel((rng.y - rng.x),_framePixel,_fps);
    int     yStart  =   rect.top()+ _timeRowHeight + 1 ;
    
    auto&    key =   track->times()->values();
    for (size_t i = 0; i <key.size(); i++)
    {
        auto    time    =   time2Pixel(key[i], _framePixel , _fps) - xOffset;
        painter.setBrush(Qt::yellow);
        painter.setPen(Qt::black);
        int     centerX =   int(time);
        int     centerY =   yStart + rowH/2;
        int     radius  =   _pointPixel/2;
        painter.drawEllipse(centerX - radius,centerY - radius,_pointPixel, _pointPixel);
    }
}
void    UiTickMgr::drawAnimation(QPainter& painter,AnimationItem* item)
{
    auto    anim    =   item->object()->cast<FE::FEAnimation>();
    if (anim == nullptr)
        return;
    int     xOffset =   _bar ? _bar->value() : 0;
    QRect   rect    =   _pTree->visualRect(item->index());
    int     cnt     =   item->rowCount();
    int     rowH    =   rect.height();
    auto    rng     =   anim->range();
    /// 计算从哪开始，到哪里结束
    real    xStart  =   time2Pixel(rng.x,_framePixel ,_fps) - xOffset;
    real    width   =   time2Pixel((rng.y - rng.x),_framePixel,_fps);
    int     yStart  =   rect.top()+ _timeRowHeight + 1 ;
    QColor  color(0, 0, 255, 100); 
    QColor  border(0, 0, 255, 200);
    QRect   temp(xStart,yStart,width,rowH);
    QRect   tmp     =   temp.marginsRemoved(QMargins(0, 1, 0, 1));
    painter.setBrush(color);
    painter.setPen(QPen(border, 2));
    painter.drawRect(tmp);
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

    update();
}

void    UiTickMgr::slotDoPaint()
{
    update();
}

void    UiTickMgr::slotDeleteKeyframes()
{}

void    UiTickMgr::slotCopyPasteKeyframes()
{}

void    UiTickMgr::slotPasteKeyframes()
{
}

void    UiTickMgr::slotCopyKeyframes()
{
   
}

void    UiTickMgr::slotSetInterpolate()
{
}

void    UiTickMgr::slotPlayToNextFrame()
{
    if (_timer && _timer->isActive())
        _timer->stop();
    else if(_timer)
        _timer->start((1000.0f/_fps));
}

void    UiTickMgr::slotAddKeyframe()
{
    /// 当前时间(秒)
    FE::real    curTime     =   FE::real(_curFrame) / FE::real(_fps);
    bool        bCreatedNew =   false;

    /// 1. 优先从动画树获取选中的动画(动画项或轨道项选中均可)
    FE::Animation   selectedAnim    =   nullptr;
    if (_pTree != nullptr && _pTree->curItem() != nullptr)
        selectedAnim    =   _pTree->curItem()->animation();

    if (selectedAnim != nullptr)
    {
        /// 动画树上有选中的动画,直接更新关键帧
        /// 从动画的 owner 获取所属节点(用于读取当前属性值)
        auto    ownerObj    =   selectedAnim->owner();
        auto    node        =   ownerObj ? ownerObj->cast<FE::FENode>() : nullptr;
        if (node != nullptr)
        {
            if (!FE::FEAnimationHelper::addNodeKeyFrame(selectedAnim, curTime, node))
                FE::FEAnimationHelper::updateNodeKeyFrame(selectedAnim, curTime, node);
        }
    }
    else
    {
        /// 2. 动画树没有选中动画,从模型树获取选中节点
        if (_pModelTree == nullptr)
            return;
        auto    selected    =   _pModelTree->selected();
        if (selected.empty())
            return;

        /// 3. 循环所有选中对象
        for (auto& obj : selected)
        {
            auto    node    =   obj ? obj->cast<FE::FENode>() : nullptr;
            if (node == nullptr)
                continue;

            /// 4. 获取节点的所有动画
            auto    anims   =   node->objects<FE::FEAnimation>();
            if (anims.empty())
            {
                /// 节点没有动画,通过 FEAnimationHelper 创建动画(含 clip + 默认轨道)
                FE::Animation   anim    =   FE::FEAnimationHelper::createNodeAnimtion(node->ctx());
                node->addComponent(anim.get());

                /// 注册到动画系统
                auto    scene   =   node->ctx().scene();
                if (scene)
                {
                    auto    sys =   scene->animationSystem();
                    if (sys)
                        sys->addObject(anim.get());
                }
                anims.push_back(anim.get());
                bCreatedNew =   true;
            }

            /// 5. 对每个动画添加或更新关键帧
            ///    时间点已存在 -> updateNodeKeyFrame;否则 -> addNodeKeyFrame
            for (auto* animPtr : anims)
            {
                FE::Animation   anim(animPtr);
                if (!FE::FEAnimationHelper::addNodeKeyFrame(anim, curTime, node))
                    FE::FEAnimationHelper::updateNodeKeyFrame(anim, curTime, node);
            }
        }

        /// 6. 如果创建了新动画,刷新动画树
        if (bCreatedNew && _pTree)
            _pTree->selectObject(selected.front(), false);
    }

    /// 7. 通知动画系统刷新范围(_range重算+清空cache),再重新应用当前帧
    emit sigKeyframesChanged();
    setCurFrame(_curFrame);
}

void    UiTickMgr::paintEvent(QPaintEvent* event)
{
    QSize       size = this->size();
    QPainter    painter(this);

    painter.setPen(QPen(QColor(0,0,0,255)));
    painter.drawRect(0, 0, size.width(), size.height());
    painter.drawLine(0, _timeRowHeight, size.width(), _timeRowHeight);
    painter.fillRect(0, 0, size.width(), _timeRowHeight, QColor(150, 150, 150, 255));
    painter.fillRect(0, _timeRowHeight, size.width(), size.height(), QColor(255, 255, 255, 255));
    // 画时间线
    double  beginX      =   _beginPixel;
    int     beginFrame  =   _beginFrame;
    int     yushu       =   _beginFrame % _frameFrequency;
    if (yushu)
    {
        beginX      +=   (_frameFrequency - yushu) * _framePixel;
        beginFrame  =   _beginFrame - yushu  + _frameFrequency;
    }

    int     lastX   =   static_cast<int>(std::round((_lastFrame - _beginFrame) * _framePixel + _beginPixel));
    if (lastX < size.width())
    {
        painter.fillRect(lastX, _timeRowHeight, size.width(), size.height(), QColor(200, 200, 200, 255));
    }
    int     firstX  =   static_cast<int>(std::round((_firstFrame - _beginFrame) * _framePixel + _beginPixel));
    if (firstX > 0)
    {
        painter.fillRect(0, _timeRowHeight, firstX, size.height(), QColor(200, 200, 200, 255));
    }

    for (;beginX <= size.width() + _frameFrequency * _framePixel; beginFrame += _frameFrequency)
    {
        // 画时间
        painter.setPen(QPen(QColor(255,255,255,255)));
        int     drawX   =   static_cast<int>(std::round(beginX));
        QRect   numRect =   {QPoint(drawX - 20, 0), QPoint(drawX + 20, _timeRowHeight)};
        painter.drawText(numRect, Qt::AlignCenter, std::to_string(beginFrame).c_str());
        // 画线
        QPoint  pointA  =   QPoint(drawX, _timeRowHeight);
        QPoint  pointB  =   QPoint(drawX, size.height());
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
    if (_pTree != nullptr && _pTree->rootItem() != nullptr)
    {
        drawItem(painter,_pTree->rootItem());
    }
    
    /// 绘制框选框
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
                auto frame  =   frameFromPos(event->pos().x());
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
            _middleHScroolValue =   _bar->value();
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
        }
        break;
    case Qt::RightButton:
        {}
        break;
    case Qt::MiddleButton:
        {
            _isMiddlePress  =   false;
        }
        break;
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
           
            update();
            _preDeltaFrame  =   deltaFrame;
        }
        else
        {
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
    if (event->modifiers() & Qt::ControlModifier) 
    {
        int     delta       =   event->angleDelta().y();
        float   newFP       =   _framePixel;
        if(delta > 0)
            newFP   *= 1.2f;
        else
            newFP   *= 0.8f;
        newFP               =   std::clamp(newFP,1.0f,100.0f); 

        /// 以鼠标位置为锚点缩放: 保持鼠标下的帧在缩放后仍在鼠标处
        /// 推导: screenX = frame * framePixel - scrollValue
        ///   旧: mouseX  = anchorFrame * oldFP - oldValue
        ///   新: mouseX  = anchorFrame * newFP - newValue   (要求锚点位置不变)
        ///   => newValue = anchorFrame * newFP - mouseX
        QPoint  mousePos    =   event->position().toPoint();
        int     mouseX      =   mousePos.x();
        double  anchorFrame =   (static_cast<double>(mouseX) - _beginPixel) / _framePixel + _beginFrame;

        _framePixel         =   newFP;
        updateScrollLength();

        if (_bar)
        {
            int newValue    =   static_cast<int>(std::round(anchorFrame * newFP - mouseX));
                newValue    =   std::clamp(newValue, 0, _bar->maximum());
            _bar->blockSignals(true);
            _bar->setValue(newValue);
            _bar->blockSignals(false);
            slotScrollValueChanged(newValue);
        }
        update();
    }
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

void    UiTickMgr::keyPressEvent(QKeyEvent* evt)
{}
void    UiTickMgr::keyReleaseEvent(QKeyEvent* evt) 
{}
