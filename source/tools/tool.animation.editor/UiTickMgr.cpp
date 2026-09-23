#include    "UiTickMgr.h"
#include    <QPainter>
#include    <QHeaderView>
#include    <QMouseEvent>
#include    <QMessageBox>
#include    <functional>
#include    "AnimationItem.h"
#include    "AnimationTree.h"
#include    "QtTree.h"
#include    "animation/FEAnimation.hpp"
#include    "animation/FEAnimationHelper.hpp"
#include    "animation/FEAnimationSys.hpp"
#include    "animation/FEUintsObject.hpp"
#include    "graphic/FEScene.h"
#include    "node/FENode.hpp"
#include    "FEMathUtil.hpp"
#include    "FEEulerObject.hpp"
#include    "UndoCommand.h"
#include    <cmath>
#include    <algorithm>
#include    <map>

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
    std::vector<int>    frames;
    collectAllKeyframeFrames(frames);
    if (frames.empty())
        return;
    std::sort(frames.begin(), frames.end());
    /// 找到小于当前帧的最大关键帧
    auto it = std::lower_bound(frames.begin(), frames.end(), _curFrame);
    if (it != frames.begin())
    {
        --it;
        if (*it != _curFrame)
            setCurFrame(*it, false);
    }
}

void    UiTickMgr::toFirstKeyframe()
{
    std::vector<int>    frames;
    collectAllKeyframeFrames(frames);
    if (frames.empty())
        return;
    auto it = std::min_element(frames.begin(), frames.end());
    if (*it != _curFrame)
        setCurFrame(*it, false);
}

void    UiTickMgr::toNextKeyframe()
{
    std::vector<int>    frames;
    collectAllKeyframeFrames(frames);
    if (frames.empty())
        return;
    std::sort(frames.begin(), frames.end());
    /// 找到大于当前帧的最小关键帧
    auto it = std::upper_bound(frames.begin(), frames.end(), _curFrame);
    if (it != frames.end())
    {
        setCurFrame(*it, false);
    }
}

void    UiTickMgr::toLastKeyframe()
{
    std::vector<int>    frames;
    collectAllKeyframeFrames(frames);
    if (frames.empty())
        return;
    auto it = std::max_element(frames.begin(), frames.end());
    if (*it != _curFrame)
        setCurFrame(*it, false);
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


void    UiTickMgr::drawKeyframeTimeline(QPainter& painter,AnimationItem* item)
{
    using   FuncWalk    =   std::function<void(AnimationItem*)>;
    if  (item == nullptr)
        return;

    /// 总纲行:以传入 item 的行位置作为统一 y 坐标
    int     xOffset =   _bar ? _bar->value() : 0;
    QRect   rect    =   _pTree->visualRect(item->index());
    int     rowH    =   rect.height();
    int     yStart  =   rect.top() + _timeRowHeight + 1;
    int     centerY =   yStart + rowH/2;
    int     radius  =   _pointPixel/2;

    /// 第一遍:绘制所有 animation 的范围矩形(灰色半透明背景块,上下留4px)
    painter.setBrush(_blockColor);
    painter.setPen(QPen(_blockBorder, 1));

    FuncWalk drawAnimBlocks = [&](AnimationItem* it)
    {
        if  (it == nullptr)
            return;
        auto    anim    =   it->object()->cast<FEAnimation>();
        if  (anim != nullptr && anim->clip())
        {
            auto    rng     =   anim->range();
            real    xStart  =   time2Pixel(rng.x,_framePixel,_fps) - xOffset;
            real    width   =   time2Pixel((rng.y - rng.x),_framePixel,_fps);
            QRect   temp((int)xStart, yStart, (int)width, rowH);
            QRect   tmp     =   temp.marginsRemoved(QMargins(0, 4, 0, 4));
            painter.drawRect(tmp);
        }
        int     cnt     =   it->rowCount();
        for  (int r = 0; r < cnt; ++r)
            drawAnimBlocks((AnimationItem*)it->child(r,0));
    };
    drawAnimBlocks(item);

    /// 第二遍:绘制所有 track 的关键帧(深灰半透明前景点)
    painter.setBrush(_dotColor);
    painter.setPen(QPen(_dotBorder, 1));

    FuncWalk drawKeyDots = [&](AnimationItem* it)
    {
        if  (it == nullptr)
            return;
        auto    track   =   it->object()->cast<FEKeyFrameTrack>();
        if  (track != nullptr && track->times())
        {
            auto    anim    =   it->animation();
            real    offset  =   anim ? anim->offset() : 0;
            auto&   key     =   track->times()->values();
            for  (size_t i = 0; i < key.size(); i++)
            {
                int     centerX =   int(time2Pixel(key[i] + offset,_framePixel,_fps) - xOffset);
                painter.drawEllipse(centerX - radius, centerY - radius, _pointPixel, _pointPixel);
            }
        }
        int     cnt     =   it->rowCount();
        for  (int r = 0; r < cnt; ++r)
            drawKeyDots((AnimationItem*)it->child(r,0));
    };
    drawKeyDots(item);
}

void    UiTickMgr::drawItem(QPainter& painter,AnimationItem* item)
{
    if (item == nullptr)
        return;

    /// 若为 FENode,绘制该节点所有动画的总纲(矩形范围,全部画到该节点所在行)
    auto    node    =   item->object()->cast<FENode>();
    if  (node != nullptr)
    {
        drawNodeAnimations(painter,item);
        if (!_pTree->isExpand(item))
            return;
    }
    auto    anim    =   item->object()->cast<FEAnimation>();
    if (anim != nullptr)
    {
        drawAnimation(painter,item);
        if (!_pTree->isExpand(item))
            return;
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
    /// 根据所属动画的 enable 状态选择颜色
    auto    anim    =   item->animation();
    bool    enabled =   anim && anim->isEnable();
    real    offset  =   anim ? anim->offset() : 0;

    for (size_t i = 0; i <key.size(); i++)
    {
        auto    time    =   time2Pixel(key[i] + offset, _framePixel , _fps) - xOffset;
        int     centerX =   int(time);
        int     centerY =   yStart + rowH/2;
        int     radius  =   _pointPixel/2;

        /// 选中状态优先绘制高亮色
        if (isKeyframeSelected(track, i))
        {
            painter.setBrush(_selColor);
            painter.setPen(QPen(_selBorder, 1));
        }
        else
        {
            painter.setBrush(enabled ? _dotColorEn : _dotColor);
            painter.setPen(QPen(enabled ? _dotBorderEn : _dotBorder, 1));
        }
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
    int     rowH    =   rect.height();
    int     yStart  =   rect.top()+ _timeRowHeight + 1;
    int     centerY =   yStart + rowH/2;
    int     radius  =   _pointPixel/2;

    /// 第一遍:绘制 animation 的范围矩形(灰色半透明背景块,上下留4px)
    auto    rng     =   anim->range();
    real    xStart  =   time2Pixel(rng.x,_framePixel,_fps) - xOffset;
    real    width   =   time2Pixel((rng.y - rng.x),_framePixel,_fps);
    QRect   temp((int)xStart, yStart, (int)width, rowH);
    QRect   tmp     =   temp.marginsRemoved(QMargins(0, 4, 0, 4));
    bool    bAnimSel    =   anim->flags().hasFlag(FE::FLAG_SELECTED);
    if (bAnimSel)
    {
        painter.setBrush(QColor(0, 162, 232, 80));
        painter.setPen(QPen(QColor(0, 162, 232, 200), 1));
    }
    else
    {
        painter.setBrush(_blockColor);
        painter.setPen(QPen(_blockBorder, 1));
    }
    painter.drawRect(tmp);

    /// 第二遍:绘制所有 track 的关键帧(深灰半透明前景点)
    real    offset  =   anim->offset();
    painter.setBrush(_dotColor);
    painter.setPen(QPen(_dotBorder, 1));

    if  (anim->clip())
    {
        for  (auto track : anim->clip()->tracks())
        {
            if  (!track || !track->times())
                continue;
            auto&   key     =   track->times()->values();
            for  (size_t i = 0; i < key.size(); i++)
            {
                int     centerX =   int(time2Pixel(key[i] + offset,_framePixel,_fps) - xOffset);
                painter.drawEllipse(centerX - radius, centerY - radius, _pointPixel, _pointPixel);
            }
        }
    }
}

void    UiTickMgr::drawNodeAnimations(QPainter& painter,AnimationItem* item)
{
    auto    node    =   item->object()->cast<FENode>();
    if  (node == nullptr)
        return;

    int     xOffset =   _bar ? _bar->value() : 0;
    QRect   rect    =   _pTree->visualRect(item->index());
    int     rowH    =   rect.height();
    int     yStart  =   rect.top() + _timeRowHeight + 1;
    int     centerY =   yStart + rowH/2;
    int     radius  =   _pointPixel/2;

    auto    anims   =   node->objects<FEAnimation>();

    /// 第一遍:绘制每个 animation 的范围矩形(灰色半透明背景块,上下留4px)
    for  (auto* animPtr : anims)
    {
        FE::Animation   anim(animPtr);
        if  (!anim || !anim->clip())
            continue;
        auto    rng     =   anim->range();
        real    xStart  =   time2Pixel(rng.x,_framePixel,_fps) - xOffset;
        real    width   =   time2Pixel((rng.y - rng.x),_framePixel,_fps);
        QRect   temp((int)xStart, yStart, (int)width, rowH);
        QRect   tmp     =   temp.marginsRemoved(QMargins(0, 4, 0, 4));
        bool    bAnimSel    =   anim->flags().hasFlag(FE::FLAG_SELECTED);
        if (bAnimSel)
        {
            painter.setBrush(QColor(0, 162, 232, 80));
            painter.setPen(QPen(QColor(0, 162, 232, 200), 1));
        }
        else
        {
            painter.setBrush(_blockColor);
            painter.setPen(QPen(_blockBorder, 1));
        }
        painter.drawRect(tmp);
    }

    /// 第二遍:绘制所有 track 的关键帧(深灰半透明前景点)
    painter.setBrush(_dotColor);
    painter.setPen(QPen(_dotBorder, 1));

    for  (auto* animPtr : anims)
    {
        FE::Animation   anim(animPtr);
        if  (!anim || !anim->clip())
            continue;
        real    offset  =   anim->offset();
        for  (auto track : anim->clip()->tracks())
        {
            if  (!track || !track->times())
                continue;
            auto&   key     =   track->times()->values();
            for  (size_t i = 0; i < key.size(); i++)
            {
                int     centerX =   int(time2Pixel(key[i] + offset,_framePixel,_fps) - xOffset);
                painter.drawEllipse(centerX - radius, centerY - radius, _pointPixel, _pointPixel);
            }
        }
    }
}

int     UiTickMgr::calcDeltaFrame(const int& p0, const int& p1) const
{
    return static_cast<int>(std::round(static_cast<double>(p1 - p0) / _framePixel));
}

HitResult   UiTickMgr::hitTest(const QPoint& pos)
{
    HitResult   result;
    if (_pTree == nullptr || _pTree->rootItem() == nullptr)
        return  result;

    int     xOffset =   _bar ? _bar->value() : 0;
    int     radius  =   _pointPixel / 2;

    /// 递归遍历动画树,找到鼠标所在行并检测命中
    std::function<void(AnimationItem*)> walk = [&](AnimationItem* item)
    {
        if (item == nullptr)
            return;
        if (result.type != HT_None)
            return;

        QRect   rect    =   _pTree->visualRect(item->index());
        int     yStart  =   rect.top() + _timeRowHeight + 1;
        int     yEnd    =   yStart + rect.height();
        int     centerY =   yStart + rect.height() / 2;

        /// 检测 y 是否在该行范围内
        if (pos.y() >= yStart && pos.y() <= yEnd)
        {
            auto    obj     =   item->object();
            auto    track   =   item->animKeyframeTrack();
            auto    anim    =   obj ? obj->cast<FE::FEAnimation>() : nullptr;
            auto    node    =   obj ? obj->cast<FE::FENode>() : nullptr;

            if (track != nullptr && track->times())
            {
                /// Track 行: 检测关键帧点命中
                auto    trackAnim   =   item->animation();
                real    offset      =   trackAnim ? trackAnim->offset() : 0;
                auto&   keys        =   track->times()->values();
                for (size_t i = 0; i < keys.size(); ++i)
                {
                    int     centerX    =   int(time2Pixel(keys[i] + offset, _framePixel, _fps) - xOffset);
                    if (std::abs(pos.x() - centerX) <= radius && std::abs(pos.y() - centerY) <= radius)
                    {
                        result.type        =   HT_Keyframe;
                        result.item        =   item;
                        result.keyIndex    =   i;
                        return;
                    }
                }
                /// 没命中关键帧点,但在该行内 → 空白(可框选)
                result.type    =   HT_Empty;
                result.item    =   item;
                return;
            }
            else if (anim != nullptr && anim->clip())
            {
                /// FEAnimation 行: 检测范围块命中
                auto    rng     =   anim->range();
                FE::real    xStart  =   time2Pixel(rng.x, _framePixel, _fps) - xOffset;
                FE::real    width   =   time2Pixel((rng.y - rng.x), _framePixel, _fps);
                QRect   blockRect((int)xStart, yStart + 4, (int)width, rect.height() - 8);
                if (blockRect.contains(pos))
                {
                    result.type    =   HT_Block;
                    result.item    =   item;
                    return;
                }
                result.type    =   HT_None;
                return;
            }
            else if (node != nullptr)
            {
                /// FENode 行: 检测任意动画的范围块命中
                auto    anims   =   node->objects<FE::FEAnimation>();
                for (auto* animPtr : anims)
                {
                    FE::Animation   animObj(animPtr);
                    if (!animObj || !animObj->clip())
                        continue;
                    auto    rng     =   animObj->range();
                    FE::real    xStart  =   time2Pixel(rng.x, _framePixel, _fps) - xOffset;
                    FE::real    width   =   time2Pixel((rng.y - rng.x), _framePixel, _fps);
                    QRect   blockRect((int)xStart, yStart + 4, (int)width, rect.height() - 8);
                    if (blockRect.contains(pos))
                    {
                        result.type    =   HT_Block;
                        result.item    =   item;
                        return;
                    }
                }
                result.type    =   HT_None;
                return;
            }
        }

        /// 递归子项
        int     cnt     =   item->rowCount();
        for (int r = 0; r < cnt; ++r)
        {
            walk((AnimationItem*)item->child(r, 0));
            if (result.type != HT_None)
                return;
        }
    };

    walk(_pTree->rootItem());
    return  result;
}

void    UiTickMgr::traverseTree(std::function<void(AnimationItem*)> cb)
{
    if (_pTree == nullptr || _pTree->rootItem() == nullptr)
        return;
    std::function<void(AnimationItem*)> walk = [&](AnimationItem* item)
    {
        if (item == nullptr)
            return;
        cb(item);
        int     cnt     =   item->rowCount();
        for (int r = 0; r < cnt; ++r)
            walk((AnimationItem*)item->child(r, 0));
    };
    walk(_pTree->rootItem());
}

void    UiTickMgr::clearSelection()
{
    traverseTree([this](AnimationItem* item)
    {
        auto    obj     =   item->object();
        auto    track   =   item->animKeyframeTrack();
        auto    anim    =   obj ? obj->cast<FE::FEAnimation>() : nullptr;
        auto    node    =   obj ? obj->cast<FE::FENode>() : nullptr;
        if (track)
            clearTrackKeySelection(track);
        if (anim)
            anim->flags().removeFlag(FE::FLAG_SELECTED);
        if (node)
        {
            auto    anims   =   node->objects<FE::FEAnimation>();
            for (auto* a : anims)
                a->flags().removeFlag(FE::FLAG_SELECTED);
        }
    });
}

void    UiTickMgr::initTrackKeySelection(FE::FEKeyFrameTrack* track)
{
    if (!track || !track->times())
        return;
    auto&   times   =   track->times()->values();
    FE::UintsObject   uints(new FE::FEUintsObject(track->ctx()));
    uints->values().resize(times.size(), 0);
    track->times()->setUserObject(uints);
}

void    UiTickMgr::setTrackKeySelection(FE::FEKeyFrameTrack* track, size_t keyIndex, bool selected)
{
    if (!track || !track->times())
        return;
    auto    userObj =   track->times()->userObject();
    auto    uints   =   userObj ? userObj->cast<FE::FEUintsObject>() : nullptr;
    if (!uints)
    {
        initTrackKeySelection(track);
        uints   =   track->times()->userObject()->cast<FE::FEUintsObject>();
    }
    if (uints && keyIndex < uints->values().size())
        uints->values()[keyIndex]    =   selected ? 1 : 0;
}

bool    UiTickMgr::isKeyframeSelected(FE::FEKeyFrameTrack* track, size_t keyIndex)
{
    if (!track || !track->times())
        return  false;
    auto    userObj =   track->times()->userObject();
    auto    uints   =   userObj ? userObj->cast<FE::FEUintsObject>() : nullptr;
    if (!uints || keyIndex >= uints->values().size())
        return  false;
    return  uints->values()[keyIndex] != 0;
}

void    UiTickMgr::clearTrackKeySelection(FE::FEKeyFrameTrack* track)
{
    if (!track || !track->times())
        return;
    track->times()->setUserObject(FE::Object());
}

void    UiTickMgr::collectAllKeyframeFrames(std::vector<int>& frames)
{
    int     xOffset =   _bar ? _bar->value() : 0;
    traverseTree([&](AnimationItem* item)
    {
        auto    track   =   item->animKeyframeTrack();
        if (track != nullptr && track->times())
        {
            auto    anim    =   item->animation();
            FE::real    offset  =   anim ? anim->offset() : 0;
            auto&   keys    =   track->times()->values();
            for (size_t i = 0; i < keys.size(); ++i)
            {
                int     frame   =   int(std::round((keys[i] + offset) * _fps));
                frames.push_back(frame);
            }
        }
    });
}

void    UiTickMgr::boxSelect(const QRect& rect)
{
    clearSelection();
    int     xOffset =   _bar ? _bar->value() : 0;

    traverseTree([&](AnimationItem* item)
    {
        auto    track   =   item->animKeyframeTrack();
        if (track != nullptr && track->times())
        {
            auto    anim        =   item->animation();
            FE::real    offset  =   anim ? anim->offset() : 0;
            auto&   keys        =   track->times()->values();
            bool    bAnyHit     =   false;
            for (size_t i = 0; i < keys.size(); ++i)
            {
                int     centerX    =   int(time2Pixel(keys[i] + offset, _framePixel, _fps) - xOffset);
                QRect   vRect      =   _pTree->visualRect(item->index());
                int     centerY    =   vRect.top() + _timeRowHeight + 1 + vRect.height() / 2;
                if (rect.contains(centerX, centerY))
                {
                    if (!bAnyHit)
                        initTrackKeySelection(track);
                    setTrackKeySelection(track, i, true);
                    bAnyHit =   true;
                }
            }
        }
    });
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
    if  (!_undoStack)
        return;

    /// 1. 从动画树获取当前选中项,并校验是否为动画对象
    FE::Animation   selectedAnim    =   nullptr;
    if  (_pTree != nullptr && _pTree->curItem() != nullptr)
    {
        auto    item    =   _pTree->curItem();
        /// track 项的 object 是 FEKeyFrameTrack, cast 到 FEAnimation 返回 nullptr
        /// 节点项的 object 是 FENode, cast 同样返回 nullptr
        /// 只有动画项的 object 是 FEAnimation
        auto    obj     =   item->object();
        if  (obj)
            selectedAnim    =   obj->cast<FE::FEAnimation>();
    }

    /// 2. 没有选中动画对象,提示用户
    if  (selectedAnim == nullptr)
    {
        QMessageBox::warning(this, u8"提示", u8"请先在动画树上选择一个动画对象");
        return;
    }

    /// 当前时间(秒)
    FE::real    curTime     =   FE::real(_curFrame) / FE::real(_fps);

    /// 3. 获取动画所属节点(关键帧数据来源)
    auto    ownerObj    =   selectedAnim->owner();
    auto    node        =   ownerObj ? ownerObj->cast<FE::FENode>() : nullptr;
    if  (node == nullptr)
    {
        QMessageBox::warning(this, u8"提示", u8"所选动画没有关联的节点,无法添加关键帧");
        return;
    }

    /// 4. 判断动画 clip 是否为空(没有 track)
    ///    若为空,redo 时先创建默认轨道,再添加关键帧;undo 时清空轨道
    auto    clip                =   selectedAnim->clip();
    bool    bEmptyClip          =   (clip == nullptr) || clip->tracks().empty();

    /// 5. 收集操作前快照(空 clip 时快照为空, redo 会先建 track 再添加关键帧)
    FE::AddKeyframeCmd::AnimStates   states;
    FE::AddKeyframeCmd::AnimState    st;
    st.node                     =   node;
    st.anim                     =   selectedAnim;
    st.createdNew               =   false;
    st.createdDefaultTracks     =   bEmptyClip;
    st.time                     =   curTime;
    st.snapshots                =   FE::AddKeyframeCmd::snapshotTracks(selectedAnim, curTime);
    states.push_back(std::move(st));

    /// 6. 创建刷新回调(redo/undo 后均调用)
    auto    refreshCb   =   [this]()
    {
        emit sigKeyframesChanged();
        setCurFrame(_curFrame);
    };

    /// 7. 推入 undo 栈,首次 redo 由栈自动调用
    _undoStack->beginMacro(u8"添加关键帧");
    _undoStack->push(new FE::AddKeyframeCmd(std::move(states), std::move(refreshCb)));
    _undoStack->endMacro();
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
        auto    item    =   _pTree->rootItem();
        drawKeyframeTimeline(painter,item);
        if (_pTree->isExpand(item))
        {
            drawItem(painter,_pTree->rootItem());
        }
    }
    
    /// 绘制拖动预览
    if (_isPress && _curPos != _pressPos)
    {
        int     deltaX  =   _curPos.x() - _pressPos.x();
        int     xOffset =   _bar ? _bar->value() : 0;
        int     radius  =   _pointPixel / 2;

        if (_isPressKeyframe && !_dragOrigTimes.empty())
        {
            /// 关键帧拖动预览: 在偏移位置绘制蓝色半透明圆
            painter.setBrush(QColor(0, 162, 232, 150));
            painter.setPen(QPen(QColor(0, 162, 232, 220), 1));
            traverseTree([&](AnimationItem* item)
            {
                auto    track   =   item->animKeyframeTrack();
                if (!track || !track->times())
                    return;
                auto    it      =   _dragOrigTimes.find(track);
                if (it == _dragOrigTimes.end())
                    return;
                auto    anim    =   item->animation();
                FE::real    offset  =   anim ? anim->offset() : 0;
                QRect   vRect   =   _pTree->visualRect(item->index());
                int     centerY =   vRect.top() + _timeRowHeight + 1 + vRect.height() / 2;
                for (size_t i = 0; i < it->second.times.size(); ++i)
                {
                    if (!isKeyframeSelected(track, i))
                        continue;
                    int     origX   =   int(time2Pixel(it->second.times[i] + offset, _framePixel, _fps) - xOffset);
                    int     previewX=   origX + deltaX;
                    painter.drawEllipse(previewX - radius, centerY - radius, _pointPixel, _pointPixel);
                }
            });
        }
        else if (_dragBlockItem)
        {
            /// 范围块拖动预览: 在偏移位置绘制蓝色半透明矩形
            auto    obj     =   _dragBlockItem->object();
            auto    anim    =   obj ? obj->cast<FE::FEAnimation>() : nullptr;
            QRect   vRect   =   _pTree->visualRect(_dragBlockItem->index());
            int     yStart  =   vRect.top() + _timeRowHeight + 1;
            int     rowH    =   vRect.height();

            auto    drawBlockPreview = [&](FE::FEAnimation* a)
            {
                if (!a || !a->clip())
                    return;
                auto    rng     =   a->range();
                FE::real    xStart  =   time2Pixel(rng.x, _framePixel, _fps) - xOffset;
                FE::real    width   =   time2Pixel((rng.y - rng.x), _framePixel, _fps);
                QRect   blockRect((int)xStart + deltaX, yStart + 4, (int)width, rowH - 8);
                painter.setBrush(QColor(0, 162, 232, 80));
                painter.setPen(QPen(QColor(0, 162, 232, 200), 1));
                painter.drawRect(blockRect);
            };

            if (anim)
            {
                drawBlockPreview(anim);
            }
            else
            {
                auto    node    =   obj ? obj->cast<FE::FENode>() : nullptr;
                if (node)
                {
                    auto    anims   =   node->objects<FE::FEAnimation>();
                    for (auto* a : anims)
                        drawBlockPreview(a);
                }
            }
        }
        else
        {
            /// 框选预览: 蓝色半透明矩形
            QPen    pen(QColor(0, 162, 232, 255));
            pen.setStyle(Qt::DashLine);
            painter.setPen(pen);
            painter.setBrush(QColor(0, 162, 232, 40));
            painter.drawRect(QRect(_pressPos, _curPos));
        }
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
                break;
            }

            /// 命中测试
            HitResult   hit     =   hitTest(event->pos());
            _pressPos           =   event->pos();
            _curPos             =   event->pos();
            _preDeltaFrame      =   0;
            _dragOrigTimes.clear();

            if (hit.type == HT_Keyframe)
            {
                _isPress            =   true;
                _isPressKeyframe    =   true;
                _dragBlockItem      =   nullptr;

                auto    track   =   hit.item->animKeyframeTrack();
                bool    bMod    =   event->modifiers() & (Qt::ShiftModifier | Qt::ControlModifier);
                bool    bSel    =   isKeyframeSelected(track, hit.keyIndex);

                /// 点击未选中的关键帧(无修饰键): 清除其他选择,仅选中此帧
                /// 点击已选中的关键帧(无修饰键): 保留所有选择,进入拖动流程
                /// Ctrl/Shift+点击已选中: 取消选中
                /// Ctrl/Shift+点击未选中: 追加选中
                if (!bMod && !bSel)
                    clearSelection();

                if (bMod && bSel)
                    setTrackKeySelection(track, hit.keyIndex, false);
                else if (!bSel)
                {
                    initTrackKeySelection(track);
                    setTrackKeySelection(track, hit.keyIndex, true);
                }

                /// 保存所有选中关键帧的原始时间
                _dragOrigTimes.clear();
                traverseTree([&](AnimationItem* item)
                {
                    auto    trk     =   item->animKeyframeTrack();
                    if (trk && trk->times())
                    {
                        auto&   times   =   trk->times()->values();
                        DragOrigData   data;
                        bool    bAnySel =   false;
                        for (size_t i = 0; i < times.size(); ++i)
                        {
                            data.times.push_back(times[i]);
                            if (isKeyframeSelected(trk, i))
                                bAnySel =   true;
                        }
                        if (bAnySel)
                        {
                            auto    anim    =   item->animation();
                            data.offset    =   anim ? anim->offset() : 0;
                            _dragOrigTimes[trk]    =   std::move(data);
                        }
                    }
                });
            }
            else if (hit.type == HT_Block)
            {
                _isPress            =   true;
                _isPressKeyframe    =   false;
                _dragBlockItem      =   hit.item;

                /// 选中动画
                clearSelection();
                auto    obj     =   hit.item->object();
                auto    anim    =   obj ? obj->cast<FE::FEAnimation>() : nullptr;
                if (anim)
                {
                    anim->flags().addFlag(FE::FLAG_SELECTED);
                    _dragOrigOffset  =   anim->offset();
                }
                else
                {
                    auto    node    =   obj ? obj->cast<FE::FENode>() : nullptr;
                    if (node)
                    {
                        auto    anims   =   node->objects<FE::FEAnimation>();
                        for (auto* a : anims)
                        {
                            a->flags().addFlag(FE::FLAG_SELECTED);
                        }
                        if (!anims.empty())
                            _dragOrigOffset  =   anims[0]->offset();
                    }
                }
            }
            else if (hit.type == HT_Empty)
            {
                _isPress            =   true;
                _isPressKeyframe    =   false;
                _dragBlockItem      =   nullptr;
                clearSelection();
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
                if (_isPressKeyframe)
                {
                    /// 关键帧拖动结束: 应用时间偏移
                    if (_pressPos != event->pos() && !_dragOrigTimes.empty())
                    {
                        double  deltaTime  =   static_cast<double>(event->pos().x() - _pressPos.x())
                                            / (_framePixel * _fps);

                        /// 检查是否为整体拖动: 所有选中关键帧属于同一个动画,
                        /// 且覆盖了该动画全部 track 的全部关键帧
                        bool    bAllDrag    =   false;
                        FE::Animation    firstAnim;
                        size_t  selTrackCnt =   0;
                        size_t  selKeyTotal =   0;
                        traverseTree([&](AnimationItem* item)
                        {
                            auto    trk     =   item->animKeyframeTrack();
                            if (trk && trk->times())
                            {
                                bool    bTrackHasSel =   false;
                                for (size_t i = 0; i < trk->times()->values().size(); ++i)
                                {
                                    if (isKeyframeSelected(trk, i))
                                    {
                                        bTrackHasSel =   true;
                                        selKeyTotal++;
                                    }
                                }
                                if (bTrackHasSel)
                                {
                                    selTrackCnt++;
                                    if (!firstAnim)
                                        firstAnim   =   item->animation();
                                }
                            }
                        });
                        if (firstAnim && firstAnim->clip())
                        {
                            size_t  totalTracks =   firstAnim->clip()->tracks().size();
                            size_t  totalKeys   =   0;
                            for (auto& trk : firstAnim->clip()->tracks())
                            {
                                if (trk->times())
                                    totalKeys   +=  trk->times()->values().size();
                            }
                            if (totalTracks == selTrackCnt && totalKeys == selKeyTotal)
                            {
                                bAllDrag    =   true;
                                traverseTree([&](AnimationItem* item)
                                {
                                    auto    trk     =   item->animKeyframeTrack();
                                    if (trk && trk->times())
                                    {
                                        for (size_t i = 0; i < trk->times()->values().size(); ++i)
                                        {
                                            if (isKeyframeSelected(trk, i))
                                            {
                                                if (item->animation() != firstAnim)
                                                    bAllDrag    =   false;
                                            }
                                        }
                                    }
                                });
                            }
                        }

                        if (bAllDrag)
                        {
                            /// 整体拖动: 修改 _offset
                            FE::real    newOffset   =   firstAnim->offset() + FE::real(deltaTime);
                            if  (_undoStack)
                            {
                                FE::MoveOffsetCmd::AnimOffsets  offsets;
                                offsets.push_back({firstAnim, firstAnim->offset(), newOffset});
                                auto    refreshCb   =   [this]()
                                {
                                    emit sigKeyframesChanged();
                                    setCurFrame(_curFrame);
                                };
                                _undoStack->beginMacro(u8"拖动关键帧");
                                _undoStack->push(new FE::MoveOffsetCmd(std::move(offsets), std::move(refreshCb)));
                                _undoStack->endMacro();
                            }
                            else
                            {
                                firstAnim->setOffset(newOffset);
                                emit sigKeyframesChanged();
                            }
                        }
                        else
                        {
                            /// 部分拖动: 逐 track 修改选中关键帧时间
                            if  (_undoStack)
                            {
                                /// 收集 old/new 数据,直接应用后创建命令(redo 为 no-op)
                                FE::MoveKeyframesCmd::TrackMoves    moves;
                                for (auto& [track, origData] : _dragOrigTimes)
                                {
                                    if (!track->times())
                                        continue;
                                    auto&   times   =   track->times()->values();
                                    if (times.size() != origData.times.size())
                                        continue;

                                    FE::MoveKeyframesCmd::TrackMove mv;
                                    mv.track    =   track;
                                    mv.oldTimes =   origData.times;
                                    mv.oldValues=   FE::MoveKeyframesCmd::readAllValues(track->values());

                                    /// 应用新时间到 track
                                    for (size_t i = 0; i < times.size(); ++i)
                                    {
                                        if (!isKeyframeSelected(track, i))
                                            continue;
                                        FE::real    newTime =   origData.times[i] + FE::real(deltaTime);
                                        /// 钳制: 时间线时间(局部+offset)不能小于0
                                        if (newTime + origData.offset < 0)
                                            newTime =   -origData.offset;
                                        times[i]    =   newTime;
                                    }
                                    track->sortKeyFames();

                                    /// 读取排序后的新状态
                                    mv.newTimes =   track->times()->values();
                                    mv.newValues=   FE::MoveKeyframesCmd::readAllValues(track->values());
                                    moves.push_back(std::move(mv));
                                }

                                auto    refreshCb   =   [this]()
                                {
                                    emit sigKeyframesChanged();
                                    setCurFrame(_curFrame);
                                };
                                _undoStack->beginMacro(u8"拖动关键帧");
                                _undoStack->push(new FE::MoveKeyframesCmd(std::move(moves), std::move(refreshCb)));
                                _undoStack->endMacro();
                            }
                            else
                            {
                                for (auto& [track, origData] : _dragOrigTimes)
                                {
                                    if (!track->times())
                                        continue;
                                    auto&   times   =   track->times()->values();
                                    if (times.size() != origData.times.size())
                                        continue;

                                    for (size_t i = 0; i < times.size(); ++i)
                                    {
                                        if (!isKeyframeSelected(track, i))
                                            continue;
                                        FE::real    newTime =   origData.times[i] + FE::real(deltaTime);
                                        if (newTime + origData.offset < 0)
                                            newTime =   -origData.offset;
                                        times[i]    =   newTime;
                                    }
                                    track->sortKeyFames();
                                }
                                emit sigKeyframesChanged();
                            }
                        }
                    }
                    /// 操作完成,清除关键帧选择标记
                    traverseTree([&](AnimationItem* item)
                    {
                        auto    trk     =   item->animKeyframeTrack();
                        if (trk)
                            clearTrackKeySelection(trk);
                    });
                }
                else if (_dragBlockItem)
                {
                    /// 范围块拖动结束: 应用 offset 变更
                    if (_pressPos != event->pos())
                    {
                        double  deltaTime  =   static_cast<double>(event->pos().x() - _pressPos.x())
                                            / (_framePixel * _fps);
                        FE::real    newOffset   =   _dragOrigOffset + FE::real(deltaTime);

                        auto    obj     =   _dragBlockItem->object();
                        auto    anim    =   obj ? obj->cast<FE::FEAnimation>() : nullptr;
                        if  (_undoStack)
                        {
                            FE::MoveOffsetCmd::AnimOffsets  offsets;
                            if (anim)
                            {
                                offsets.push_back({anim, anim->offset(), newOffset});
                            }
                            else
                            {
                                auto    node    =   obj ? obj->cast<FE::FENode>() : nullptr;
                                if (node)
                                {
                                    auto    anims   =   node->objects<FE::FEAnimation>();
                                    for (auto* animPtr : anims)
                                        offsets.push_back({FE::Animation(animPtr), animPtr->offset(), newOffset});
                                }
                            }
                            auto    refreshCb   =   [this]()
                            {
                                emit sigKeyframesChanged();
                                setCurFrame(_curFrame);
                            };
                            _undoStack->beginMacro(u8"拖动动画范围");
                            _undoStack->push(new FE::MoveOffsetCmd(std::move(offsets), std::move(refreshCb)));
                            _undoStack->endMacro();
                        }
                        else
                        {
                            if (anim)
                            {
                                anim->setOffset(newOffset);
                            }
                            else
                            {
                                auto    node    =   obj ? obj->cast<FE::FENode>() : nullptr;
                                if (node)
                                {
                                    auto    anims   =   node->objects<FE::FEAnimation>();
                                    for (auto* animPtr : anims)
                                        animPtr->setOffset(newOffset);
                                }
                            }
                            emit sigKeyframesChanged();
                        }
                    }
                    _dragBlockItem  =   nullptr;
                }
                else
                {
                    /// 框选结束
                    if (_pressPos != event->pos())
                    {
                        QRect   selRect    =   QRect(_pressPos, event->pos()).normalized();
                        boxSelect(selRect);
                    }
                }

                _isPress            =   false;
                _isPressKeyframe    =   false;
                _preDeltaFrame      =   0;
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

    update();
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
        /// 关键帧拖动 / 范围块拖动 / 框选: 仅更新显示
        update();
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
