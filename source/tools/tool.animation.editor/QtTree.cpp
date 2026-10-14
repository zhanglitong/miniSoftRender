#include    "QtTree.h"
#include    <algorithm>
#include    <fstream>
#include    <cstdio>

#include    <QWidget>
#include    <QPainter>
#include    <QPaintEvent>
#include    <QVBoxLayout>
#include    <QScrollArea>
#include    <QPoint>
#include    <QApplication>
#include    <QKeyEvent>
#include    "MainWindow.h"
#include    "animation/FEAnimation.hpp"


namespace   FE
{
    /// 同级拖动三区判定: 行中间该百分比范围视为"在item上"(嵌入目标/修改父节点),
    /// 其余为上/下边缘"两个item之间"(插到目标前/后/修改顺序),边缘带最小3px
    static  const int   DROP_MID_PERCENT    =   60;
    inline  bool    ptInRect(FERect *lprc,int2 pt)
    {
        return  pt.x >= lprc->left && pt.x <= lprc->right && pt.y >= lprc->top && pt.y <= lprc->bottom;
    }

    inline  void    setRect( FERect* lprc,int xLeft,int yTop,int xRight,int yBottom)
    {
        lprc->left      =    xLeft;
        lprc->top       =    yTop;
        lprc->right     =    xRight;
        lprc->bottom    =    yBottom;
    }

    QtTree::QtTree(QWidget* parent)
        :QWidget(parent)
    {
        _icon.load(":/EditorRes/res/ico_Set.png");
        /// PNG 无 alpha 通道,将白色背景设为透明
        _icon.setMask(_icon.createMaskFromColor(Qt::white));
        _bmpHeight      =   (std::max)(_icon.height(),16);
        _bmpWidth       =   (std::max)(_icon.width(),512);
        QVBoxLayout*    vlayout = new QVBoxLayout(this);
        vlayout->setSpacing(0);
        vlayout->setContentsMargins(0, 0, 0, 0);
        /// 接收键盘事件(Ctrl+C/X/V 剪贴板操作),点击控件获得焦点
        setFocusPolicy(Qt::StrongFocus);
        _vScrollBar     =   new QScrollBar(Qt::Vertical, this);
        _vScrollBar->setValue(0);

        _vScrollBar->setLayoutDirection(Qt::RightToLeft);

        vlayout->addWidget(_vScrollBar);

        updateScroll();

        /// 连接valueChanged信号到自定义槽函数
        QObject::connect(_vScrollBar,   &QScrollBar::valueChanged,  this,   &QtTree::onValueChanged);
        QObject::connect(this,          &QtTree::repaintItem,       this,   &QtTree::onPaint);

        /// 拖拽自动滚动定时器(光标贴近视口上/下边缘时周期滚动)
        _autoScrollTimer    =   new QTimer(this);
        QObject::connect(_autoScrollTimer,  &QTimer::timeout,   this,   &QtTree::onAutoScrollTimeout);
        _renameEditor       =   nullptr;
    }

    QtTree::~QtTree()
    {
    }

    
    void    QtTree::setup(Scene scene)
    {   
        _scene      =   scene;
        if (!roots().empty())
            updateScroll();
        if (_scene)
        {
            _scene->nodeTree().eventsAddNode()      +=  {this,[this](const FENode* )
            {
                update();
            }};
            _scene->nodeTree().eventsRemoveNode()   +=  {this,[this](const FENode* )
            {
                update();
            }};
            _scene->nodeTree().eventsClear() += {this,[this]()
            {
                update();
            }};
        }
    }

    void    QtTree::destroy()
    {
        if (_scene)
        {
            _scene->nodeTree().eventsAddNode()      -=  this;
            _scene->nodeTree().eventsRemoveNode()   -=  this;
            _scene->nodeTree().eventsClear()        -=  this;
        }
        _scene              =   nullptr;
        _curItem            =   nullptr;
        _contextItem        =   nullptr;
        _cutItem            =   nullptr;
        _dragItem           =   nullptr;
        _dropTarget         =   nullptr;
        _dragging           =   false;
        _dropValid          =   false;
        _dropMode           =   DM_None;
        if (_autoScrollTimer)
            _autoScrollTimer->stop();
        _autoScrollStep     =   0;
        cancelRename();
        _itemDatas.clear();
    }

    void    QtTree::reset()
    {
        _curItem            =   nullptr;
        _contextItem        =   nullptr;
        _itemDatas.clear();
    }

    int     QtTree::getIconIndex(FEObject* object)
    {
        if(dynamic_cast<FENode*>(object))
            return  ID_NODE;
        else if(dynamic_cast<FEMesh*>(object))
            return  ID_GEO;
        else if(dynamic_cast<FEMaterial*>(object))
            return  ID_MATERIAL;
        return  ID_OBJECT;
    }

    void    QtTree::postRepaint()
    {
        emit repaintItem();
    }

    void    QtTree::expandItem( ItemData& item )
    {
        expandItem(*item.item);
    }

    void    QtTree::expandItem(FEObject& item)
    {
        if (isExpand(item))
            FE::setExpand(item,false);
        else    
            FE::setExpand(item,true);
        updateScroll();
    }

    void    QtTree::checkItem(FEObject& item)
    {
        auto    node    =   item.as<FENode>();
        if(node)
        {
            node->flags().addFlag(FE::FLAG_VISIBLE | FE::FLAG_UPDATE | FENode::FLAG_PROP_STATE);
            node->fireChanged();
        }
        else
        {
            item.flags().addFlag(FE::FLAG_VISIBLE | FE::FLAG_UPDATE);
        }
    }

    void    QtTree::unCheckItem(FEObject& item)
    {
        auto    node    =   item.cast<FENode>();
        if(node)
        {
            node->flags().removeFlag(FE::FLAG_VISIBLE);
            node->flags().addFlag(FENode::FLAG_PROP_STATE);
            node->fireChanged();
        }
        else
        {
            item.flags().removeFlag(FE::FLAG_VISIBLE);
        }
    }

    bool    QtTree::selectItem(FEObject& item)
    {
        /// 取消旧选中项
        if (_curItem)
            FE::setSelected(*_curItem, false);

        _curItem    =   &item;
        FE::setSelected(*_curItem, true);
        _selectEvts(_curItem, false);
        emit signalSelect(_curItem);

        return  true;
    }

    inline  bool    traverseShow(const FEObject& object,const FEObject&,const FEObject::FETrvsCtx&,uint)
    {
        FE::setVisible((FEObject&)object,true);
        object.traverseObject(traverseShow);
        return  true;
    }

    void    QtTree::showModel( FEObject& item)
    {
        FE::setVisible(item,true);
        item.traverseObject(traverseShow);
    }

    inline  bool    traverseHide(const FEObject& object,const FEObject&,const FEObject::FETrvsCtx&,uint)
    {
        FE::setVisible((FEObject&)object,false);
        object.traverseObject(traverseHide);
        return  true;
    }
    void    QtTree::hideModel( FEObject& item)
    {
        FE::setVisible(item,false);
        item.traverseObject(traverseHide);
    }

    int     QtTree::getVScroll()
    {
        return  _vScrollBar->value();
    }
    int     QtTree::getHScroll()
    {
        return  0;
    }

    void    QtTree::drawItem(QPainter& hDC, ItemData& item, int x, int y)
    {
        /// 剪切待粘贴的节点内容半透明绘制(资源管理器剪切风格),行背景保持正常
        bool    isCut   =   (item.item == _cutItem);
        if (isCut)
            hDC.setOpacity(0.45);

        /// 滚动条可见时才扣除其宽度,隐藏时内容占满整行
        int     scrollW =   _vScrollBar->isVisible() ? _vScrollBar->width() : 0;
        _hscrollMax     =   _rect.right - _rect.left - scrollW - _leftMargin * 2;

        int     icoIdx  =   getIconIndex(item.item);
        int     xStart  =   x;
        int     yOff    =   (_rowHeight - _bmpHeight) / 2;
                y       +=  yOff;

        /// 绘制展开(+ - )图标:叶子节点与展开节点都画 SUB,未展开画 PLUS
        int     expandId    =   (item.isExpand() || item.item->objectCount() == 0) ? ID_SUB : ID_PLUS;
        setRect(&item.expands, xStart, y, xStart + _bmpHeight, y + _bmpHeight);
        hDC.drawPixmap(xStart, y, _bmpHeight, _bmpHeight, _icon, expandId * _bmpHeight, 0, _bmpHeight, _bmpHeight);
        xStart  +=  _bmpHeight;

        /// 绘制节点类型图标
        hDC.drawPixmap(xStart, y, _bmpHeight, _bmpHeight, _icon, icoIdx * _bmpHeight, 0, _bmpHeight, _bmpHeight);
        xStart  +=  _bmpHeight;

        /// 绘制checkbox图标
        int     checkId =   item.isVisible() ? ID_CHECK : ID_UNCHECK;
        setRect(&item.checkBox, xStart, y, xStart + _bmpHeight, y + _bmpHeight);
        hDC.drawPixmap(xStart, y, _bmpHeight, _bmpHeight, _icon, checkId * _bmpHeight, 0, _bmpHeight, _bmpHeight);
        xStart  +=  _bmpHeight;
        xStart  +=  4;

        /// 绘制文字,选中行已由 paintItem 填充浅蓝底
        /// 如果是 FENode 且有动画组件,在名称前绘制 [A数量] 前缀
        /// 动画状态着色: 全部可用→蓝色, 全部不可用→灰色, 混合→红色
        /// (FE 无 enabled() 接口, 以 isValid() 映射: 有 clip+owner 视为可用)
        QString text    =   item.getName();
        bool    hasEnabled  =   false;
        bool    hasDisabled =   false;
        if (item.item)
        {
            auto    node    =   item.item->cast<FENode>();
            if (node)
            {
                auto    anims   =   node->objects<FEAnimation>();
                if (!anims.empty())
                {
                    text    =   QString("[A%1] ").arg(anims.size()) + text;
                    for (auto anim : anims)
                    {
                        if (anim->isValid())
                            hasEnabled  =   true;
                        else
                            hasDisabled =   true;
                    }
                }
            }
        }
        QRect   qRT(xStart, y, _hscrollMax, _bmpHeight);
        QRect   br;
        int     flags   =   Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine;

        QColor  textColor   =   palette().color(QPalette::Text);
        if (hasEnabled && hasDisabled)
            textColor   =   QColor(255, 0, 0);      /// 既有可用也有不可用
        else if (hasEnabled)
            textColor   =   QColor(0, 0, 255);      /// 全部可用(蓝色)
        else if (hasDisabled)
            textColor   =   QColor(128, 128, 128);  /// 全部不可用(灰色)
        auto    oldPen  =   hDC.pen();
        hDC.setPen(QPen(textColor));
        hDC.drawText(qRT, flags, text, &br);
        hDC.setPen(oldPen);
        if (isCut)
            hDC.setOpacity(1.0);
    }
    void    QtTree::paintItem(QPainter& hDC,FEObject& item,int& x,int& y,int parentIndex,int& rowIndex,bool parMatch)
    {
        UNUSED(parMatch);
        auto    rt      =   rect();
        int     height  =   rt.height();
        if (y > height)
            return;
        if (!nameIsValid(item))
            return;

        int     xOffset    =   _space;
        int     yOffset    =   _rowHeight;

        /// 绘制行背景:选中态整行高亮,否则奇偶行交替底色
        {
            int     scrollW =   _vScrollBar->isVisible() ? _vScrollBar->width() : 0;
            int     rowLeft =   _leftMargin;
            int     rowRight=   width() - scrollW;
            QRect   rowRt(rowLeft, y, rowRight - rowLeft, yOffset);

            if (FE::isSelect(item))
            {
                /// QTreeView 风格选中:浅蓝整行底色 + 深蓝整行边框
                /// NoBrush 防止覆盖底色,adjusted 收回右/下边框防止被滚动条与下一行覆盖
                hDC.fillRect(rowRt, QColor(204, 232, 255));
                hDC.setPen(QPen(QColor(0, 120, 215)));
                auto    oldBrush    =   hDC.brush();
                hDC.setBrush(Qt::NoBrush);
                hDC.drawRect(rowRt.adjusted(0, 0, -1, -1));
                hDC.setBrush(oldBrush);
            }
            else
            {
                QColor  bg  =   (rowIndex % 2 == 0)
                                ? palette().color(QPalette::Base)
                                : palette().color(QPalette::AlternateBase);
                hDC.fillRect(rowRt, bg);
            }
        }

        /// 绘制当前节点内容
        if (y + yOffset > 0)
        {
            ItemData    temp    =
            {
                 x,                     y,  x + _bmpHeight,         y + _bmpHeight
                ,x + _bmpHeight + 4,    y,  x + _bmpHeight * 2 + 4, y + _bmpHeight
                ,x + _bmpHeight * 2 + 4,y,  x + _bmpHeight * 6 + 4, y + _bmpHeight
                ,&item,parentIndex
            };
            drawItem(hDC,temp,x,y);
            /// text 命中区域扩大为整行,便于点击空白处也能选中
            int     scrollW =   _vScrollBar->isVisible() ? _vScrollBar->width() : 0;
            temp.text.left   =   _leftMargin;
            temp.text.right  =   width() - scrollW;
            temp.text.top    =   y;
            temp.text.bottom =   y + yOffset;
            _itemDatas.push_back(temp);
        }

        ++rowIndex;

        bool    hasChildren =   !isEmpty(item);
        bool    expanded    =   hasChildren && isExpand(item);

        y   +=  yOffset;

        /// 仅展开且有子节点时递归绘制子节点
        if (expanded)
        {
            x   +=  xOffset;
            item.traverseObject([this,&hDC,&x,&y,parentIndex,&rowIndex](const FEObject& object,const FEObject&,const FEObject::FETrvsCtx&,uint)->bool
            {
                paintItem(hDC,(FEObject&)object,x,y,parentIndex,rowIndex);
                return  true;
            });
            x   -=  xOffset;
        }
    }

    void    QtTree::paintEvent(QPaintEvent *event) 
    {
        UNUSED(event);
        QPainter    painter(this);
        
        QColor      backgroundColor = palette().color(backgroundRole());
        auto        rt  =   rect();
        // 在这里添加你的绘制代码
        painter.setBrush(backgroundColor);
        painter.drawRect(rt);
        
        // 设置画笔：白色，2像素宽
        QPen pen(Qt::lightGray, 2, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
        painter.setPen(pen);

        // 绘制边框。使用 adjusted(0, 0, -1, -1) 确保边框完全显示在窗口内部
        painter.drawRect(rt.adjusted(0, 0, -1, -1));

        painter.setPen(QPen()); 

        _rowHeight          =   _bmpHeight  + _bmpHeight/4;

        _itemDatas.clear();

        int     height  =   rt.height();

        int     pos     =   getVScroll();
        int     x       =   _leftMargin - getHScroll();
        int     y       =   _topMargin  - pos;
        int     i       =   0;
        int     rowIndex    =   0;
        for (auto node: roots())
        {
            if (y > height)
                break;
            FEObject&   object  =   *node;
            paintItem(painter, object, x, y, i, rowIndex);
        }

        /// 拖拽视觉(投放指示 + 跟随光标的拖拽图像)
        drawDragVisual(painter);
    }


    void    QtTree::mousePressEvent(QMouseEvent*event)
    {
        _downPos    =   int2(event->position().x(),event->position().y());
        setFocus();
        switch (event->button())
        {
        case Qt::LeftButton:
            /// 记录潜在拖拽起点(仅 FENode 可拖,动画等对象不参与)
            _dragItem   =   itemAt(event->pos());
            if (_dragItem && _dragItem->cast<FENode>() == nullptr)
                _dragItem   =   nullptr;
            onLButtonDown(event);
            break;
        case Qt::RightButton:
            onRButtonDown(event);
            break;
        case Qt::MiddleButton:
            onMButtonDown(event);
            break;
        default:
            break;
        }
        postRepaint();
    }
    void    QtTree::mouseReleaseEvent(QMouseEvent *event)
    {
        _upPos          =   int2(event->pos().x(),event->pos().y());

        /// 拖拽投放优先于点击处理(仅在控件内释放生效,控件外取消)
        if (_dragging && event->button() == Qt::LeftButton)
        {
            Object  dragItem    =   _dragItem;
            Object  dropTarget  =   _dropTarget;
            DropMode    mode    =   _dropMode;
            bool    valid       =   _dropValid && rect().contains(event->pos());
            _dragItem   =   nullptr;
            _dragging   =   false;
            _dropTarget =   nullptr;
            _dropValid  =   false;
            _dropMode   =   DM_None;
            if (_autoScrollTimer)
                _autoScrollTimer->stop();
            _autoScrollStep =   0;
            update();
            if (valid && dragItem)
                emit signalNodeDropped(dragItem, dropTarget, mode);
            return;
        }
        _dragItem   =   nullptr;

        auto    off     =   _upPos - _downPos;
        bool    isClick =   abs(off.x) <= 4 && abs(off.y) <= 4;
        switch (event->button())
        {
        case Qt::LeftButton:
            onLButtonUp(event);
            if (isClick)
                onLButtonClicked(event);
            break;
        case Qt::RightButton:
            onRButtonUp(event);
            if (isClick)
                onLButtonClicked(event);
            break;
        case Qt::MiddleButton:
            onMButtonUp(event);
            if (isClick)
                onMButtonClicked(event);
            break;
        default:
            break;
        }
        postRepaint();
    }
    void    QtTree::mouseDoubleClickEvent(QMouseEvent *event)
    {
        switch (event->button())
        {
        case Qt::LeftButton:
            onLButtonDbClicked(event);
            break;
        case Qt::RightButton:
            onRButtonDbClicked(event);
            break;
        case Qt::MiddleButton:
            onMButtonDbClicked(event);
            break;
        default:
            break;
        }
    }
    void    QtTree::mouseMoveEvent(QMouseEvent* event)
    {
        if (_dragItem == nullptr)
            return;
        if (!(event->buttons() & Qt::LeftButton))
        {
            /// 按键已松开(如鼠标在控件外弹起),复位拖拽状态
            _dragItem           =   nullptr;
            _dragging           =   false;
            _dropValid          =   false;
            _dropTarget         =   nullptr;
            _dropMode           =   DM_None;
            if (_autoScrollTimer)
                _autoScrollTimer->stop();
            _autoScrollStep     =   0;
            return;
        }
        if (!_dragging)
        {
            QPoint  downPt(_downPos.x, _downPos.y);
            if ((event->pos() - downPt).manhattanLength() < QApplication::startDragDistance())
                return;
            _dragging   =   true;
        }
        _dragPos    =   event->pos();
        updateAutoScroll();
        updateDropState();
    }
    void    QtTree::wheelEvent(QWheelEvent* evt)
    {
        int     delta   =   evt->angleDelta().y();
        auto    val     =   _vScrollBar->value();
        if (delta > 0)
            _vScrollBar->setValue(val - _rowHeight);
        else if (delta < 0)
            _vScrollBar->setValue(val + _rowHeight);
    }

    void    QtTree::resizeEvent(QResizeEvent*)
    {
        /// 窗口大小赋值
        _rect.left      =   rect().left();
        _rect.right     =   rect().right();
        _rect.top       =   rect().top();
        _rect.bottom    =   rect().bottom();

        updateScroll();
    }

    void    QtTree::onLButtonDown(QMouseEvent *)
    {}
    void    QtTree::onLButtonUp(QMouseEvent *)
    {}

    void    QtTree::onRButtonDown(QMouseEvent*evt)
    {
        int2       pt      =   { evt->pos().x(),evt->pos().y() };
        Object     ptItem  =   nullptr;
        for (size_t i = 0; i < _itemDatas.size(); ++i)
        {
            ItemData&   item = _itemDatas[i];
            if (ptInRect(&item.text, pt))
            {
                ptItem  =   item.item;
                break;
            }
        }
        /// 保存上下文材质
        _contextItem    =   ptItem;
        emit signalContextMenu(evt->pos(),ptItem);
    }
    void    QtTree::onRButtonUp(QMouseEvent *)
    {}

    void    QtTree::onMButtonDown(QMouseEvent *)
    {}
    void    QtTree::onMButtonUp(QMouseEvent *)
    {}

    void    QtTree::onLButtonDbClicked(QMouseEvent*event)
    {
        int2    pt = { event->pos().x(),event->pos().y() };
        for (size_t i = 0; i < _itemDatas.size(); ++i)
        {
            ItemData&   item = _itemDatas[i];
            if (!ptInRect(&item.text, pt))
                continue;
            setTreeItemSelect(item.item, false, true, false, true);
            _lbDbClickeds(int2(pt.x,pt.y),item.item);
            emit signalLButtonDbClicked(QPoint(pt.x,pt.y),item.item);
            break;
        }
    }
    void    QtTree::onRButtonDbClicked(QMouseEvent*)
    {}
    void    QtTree::onMButtonDbClicked(QMouseEvent*)
    {}

    void    QtTree::onLButtonClicked(QMouseEvent *event)
    {
        int2    pt  =   {event->pos().x(),event->pos().y()};
        for (size_t i = 0 ;i < _itemDatas.size() ; ++ i)
        {
            ItemData&   item    =   _itemDatas[i];
            if (ptInRect(&item.checkBox,pt))
            {
                if (item.isVisible())
                {
                    unCheckItem(*item.item);
                    hideModel(*item.item);
                }
                else
                {
                    checkItem(*item.item);
                    showModel(*item.item);
                }
                break;
            }
            else if (ptInRect(&item.expands,pt))
            {
                expandItem(item);
                break;
            }
            else if (ptInRect(&item.text,pt))
            {
                selectItem(*item.item);
                break;
            }
        }
    }
    void    QtTree::onRButtonClicked(QMouseEvent*)
    {}
    void    QtTree::onMButtonClicked(QMouseEvent*)
    {}

    void    QtTree::closeEvent(QCloseEvent *event)
    {
        UNUSED(event);
        destroy();
    }

    void    QtTree::updateScroll()
    {
        int pageSize    =   (std::max)(_rect.bottom - _rect.top,_rowHeight);
        int allSize     =   getItemHeight();
        
        if (allSize <=  pageSize)
            _vScrollBar->hide();
        else
            _vScrollBar->show();
        int nMax    =   (std::max<int>)(0,allSize - pageSize) + _rowHeight;

        _vScrollBar->setMinimum(0);
        _vScrollBar->setMaximum(nMax);
        _vScrollBar->setPageStep(pageSize);
    }

    void    QtTree::setTreeItemSelect(Object item, bool expandTo, bool beCenter, bool forceVisible, bool isSelf)
    {
        UNUSED(expandTo, beCenter, forceVisible, isSelf);

        /// 仅设置对象的选中标记,不更新 _curItem(由 selectItem 负责)
        if (item)
            FE::setObjectSelected(*item);
    }

    bool    QtTree::gotoItem(Object pNode)
    {
        Objects     routes;
        bool        bFind   =   false;
        for (auto node : roots())
        {
            node->traverseObject([this,&pNode,&routes,&bFind](const FEObject& object,const FEObject& parent,const FEObject::FETrvsCtx&,uint depth)->bool
            {
                if (!bFind && routes.size() != depth)
                {
                    routes.resize(depth);
                    routes[depth - 1]   =   (FEObject*)&parent;
                }
                if (pNode.get() == &object)
                {
                    bFind   =   true;
                    return  false;
                }
                return  true;
            }, 1, true);
        }

        if (!bFind)
            return  false;

        /// 展开所有父节点,使目标节点可见
        for (auto& var : routes)
            FE::setExpand(*var, true);

        /// 计算目标节点的垂直偏移并滚动到该位置
        int     diff    =   0;
        for (auto node : roots())
        {
            diff += _rowHeight;
            if (!isExpand(*node))
                continue;
            node->traverseObject([this,&diff,&pNode](const FEObject& object,const FEObject&,const FEObject::FETrvsCtx& ctx,uint depth)->bool
            {
                if (pNode.get() == &object)
                    return  false;
                if (!nameIsValid((FEObject&)object))
                    return  true;
                diff += _rowHeight;
                if (!isExpand((FEObject&)object) || object.objectCount() == 0)
                    return  true;
                return  object.traverseObject(ctx.callback, depth + 1, false);
            }, 1, false);
        }
        _vScrollBar->setValue(diff);
        postRepaint();
        return  true;
    }

    int     QtTree::getItemHeight()
    {
        int     diff    =   0;
        if (roots().empty())
            return  0;
        for (auto node : roots())
        {
            diff += _rowHeight;
            if (!isExpand(*node))
                continue;
            node->traverseObject([this,&diff](const FEObject& object,const FEObject&,const FEObject::FETrvsCtx& ctx,uint depth)->bool
            {
                if (!nameIsValid((FEObject&)object))
                    return  true;
                diff += _rowHeight;
                /// 未展开或叶子节点不再递归
                if (!isExpand((FEObject&)object) || object.objectCount() == 0)
                    return  true;
                return  object.traverseObject(ctx.callback, depth + 1, false);
            }, 1, false);
        }
        return  diff;
    }

    void    QtTree::onValueChanged(int val)
    {
        UNUSED(val);
        repaint();
    }

    void    QtTree::onPaint()
    {
        repaint();
    }

    /// 按控件坐标反查行节点(未命中返回空)
    Object  QtTree::itemAt(const QPoint& pt) const
    {
        int2    p   =   { pt.x(), pt.y() };
        for (size_t i = 0; i < _itemDatas.size(); ++i)
        {
            const ItemData& item    =   _itemDatas[i];
            if (p.x >= item.text.left && p.x <= item.text.right
                && p.y >= item.text.top && p.y <= item.text.bottom)
                return  item.item;
        }
        return  nullptr;
    }

    bool    QtTree::canDropOn(Object target, DropMode mode) const
    {
        if (_dragItem == nullptr)
            return  false;
        auto    dragNode    =   _dragItem->cast<FENode>();
        if (dragNode == nullptr)
            return  false;
        /// 空白区域 = 移动到顶层;已为顶层(无父)则属无意义投放
        if (target == nullptr)
            return  dragNode->parent() != nullptr;
        auto    targetNode  =   target->cast<FENode>();
        if (targetNode == nullptr)
            return  false;
        /// 禁止拖到自身
        if (targetNode == dragNode)
            return  false;
        /// 禁止拖到自身后代(防止成环): FE parent() 返回 Node,用 .get() 比较
        for (auto p = targetNode->parent().get(); p != nullptr; p = p->parent().get())
        {
            if (p == dragNode)
                return  false;
        }
        /// 同父: 嵌入兄弟 = 修改父节点(恒合法);前/后插入 = 兄弟排序,顺序无变化则非法
        if (targetNode->parent().get() == dragNode->parent().get())
        {
            if (mode == DM_Into)
                return  true;
            auto    parent  =   dragNode->parent();
            if (parent == nullptr)
                return  false;
            auto&   chs         =   parent->children();
            size_t  dragIdx     =   0;
            size_t  targetIdx   =   0;
            for (size_t i = 0; i < chs.size(); ++i)
            {
                if (chs[i].get() == dragNode)      dragIdx     =   i;
                if (chs[i].get() == targetNode)    targetIdx   =   i;
            }
            /// 目标在拖拽节点之后时,移除自身后其索引前移一位
            size_t  art         =   (targetIdx > dragIdx) ? targetIdx - 1 : targetIdx;
            size_t  newIndex    =   (mode == DM_After) ? art + 1 : art;
            return  newIndex != dragIdx;
        }
        /// 跨父: 嵌入 = 目标子节点;前/后插入 = 新父对应位置,均为真实变化恒合法
        return  true;
    }

    void    QtTree::updateAutoScroll()
    {
        /// 光标进入视口上/下边缘 24px 区域后自动滚动,越靠边速度越快(4~24px/次)
        const   int     zonePx  =   24;
        int     step    =   0;
        if (_dragPos.y() < zonePx)
            step    =   -(zonePx - _dragPos.y() + 4);
        else if (_dragPos.y() > height() - zonePx)
            step    =   _dragPos.y() - (height() - zonePx) + 4;
        _autoScrollStep =   step;
        if (step == 0)
        {
            if (_autoScrollTimer)
                _autoScrollTimer->stop();
        }
        else if (_autoScrollTimer && !_autoScrollTimer->isActive())
            _autoScrollTimer->start(40);
    }

    void    QtTree::updateDropState()
    {
        if (rect().contains(_dragPos))
        {
            Object  target  =   itemAt(_dragPos);
            /// 三区判定: 行中间 DROP_MID_PERCENT%(边缘带之外)视为"在item上" = 嵌入目标,
            /// 上/下边缘视为"两个item之间" = 插到目标前/后;空白区域移到顶层
            DropMode    zone    =   DM_Into;
            if (target == nullptr)
                zone    =   DM_Top;
            else if (target->cast<FENode>() != nullptr)
            {
                for (size_t i = 0; i < _itemDatas.size(); ++i)
                {
                    const ItemData& item    =   _itemDatas[i];
                    if (item.item != target)
                        continue;
                    int     rowH    =   item.text.bottom - item.text.top;
                    int     edge    =   (std::max)(3, rowH * (100 - DROP_MID_PERCENT) / 200);
                    int     y       =   _dragPos.y();
                    zone    =   (y < item.text.top + edge)          ? DM_Before
                              : (y > item.text.bottom - edge)   ? DM_After
                              : DM_Into;
                    break;
                }
            }
            _dropValid  =   canDropOn(target, zone);
            _dropTarget =   _dropValid ? target : nullptr;
            _dropMode   =   _dropValid ? zone : DM_None;
        }
        else
        {
            _dropValid  =   false;
            _dropTarget =   nullptr;
            _dropMode   =   DM_None;
        }
        update();
    }

    void    QtTree::onAutoScrollTimeout()
    {
        /// 拖拽已结束或光标已离开边缘: 停止定时器
        if (!_dragging || _autoScrollStep == 0)
        {
            if (_autoScrollTimer)
                _autoScrollTimer->stop();
            return;
        }
        /// 滚动后重新计算投放判定(行矩形随滚动偏移变化,_itemDatas 由重绘刷新)
        _vScrollBar->setValue(_vScrollBar->value() + _autoScrollStep);
        updateDropState();
    }

    void    QtTree::drawDragVisual(QPainter& hDC)
    {
        if (!_dragging)
            return;
        const QColor    indColor(0, 120, 215);

        /// 1. 投放位置指示
        if (_dropValid)
        {
            if (_dropMode == DM_Top)
            {
                /// 顶层指示线(空白区域投放 = 移动到顶层)
                auto    oldPen  =   hDC.pen();
                hDC.setPen(QPen(indColor, 2));
                hDC.drawLine(_leftMargin, _topMargin + 1, width() - _leftMargin, _topMargin + 1);
                hDC.setPen(oldPen);
            }
            else
            {
                for (size_t i = 0; i < _itemDatas.size(); ++i)
                {
                    const ItemData& item    =   _itemDatas[i];
                    if (item.item != _dropTarget)
                        continue;
                    QRect   rowRt(item.text.left, item.text.top
                        , item.text.right - item.text.left, item.text.bottom - item.text.top);
                    auto    oldPen      =   hDC.pen();
                    auto    oldBrush    =   hDC.brush();
                    if (_dropMode == DM_Into)
                    {
                        /// 嵌入目标: 半透明底色 + 边框
                        hDC.fillRect(rowRt, QColor(indColor.red(), indColor.green(), indColor.blue(), 60));
                        hDC.setPen(QPen(indColor, 2));
                        hDC.setBrush(Qt::NoBrush);
                        hDC.drawRect(rowRt.adjusted(0, 0, -1, -1));
                    }
                    else
                    {
                        /// 同级排序: 目标行上/下边缘插入线(QTreeWidget 风格)
                        int     lineY   =   (_dropMode == DM_Before) ? rowRt.top() : rowRt.bottom();
                        hDC.setPen(QPen(indColor, 2));
                        hDC.drawLine(rowRt.left(), lineY, rowRt.right(), lineY);
                    }
                    hDC.setPen(oldPen);
                    hDC.setBrush(oldBrush);
                    break;
                }
            }
        }

        /// 2. 跟随光标的拖拽图像(图标+名称,半透明背景框,光标右下偏移)
        if (_dragItem)
        {
            const QString   name    =   FE::getName(*_dragItem);
            const int       icoIdx  =   getIconIndex(_dragItem.get());
            const QFontMetrics  fm  =   hDC.fontMetrics();
            const int       pad     =   6;
            const int       gap     =   4;
            const int       textW   =   fm.horizontalAdvance(name);
            const int       boxW    =   pad + _bmpHeight + gap + textW + pad;
            const int       boxH    =   _rowHeight;
            const int       dx      =   _dragPos.x() + 12;
            const int       dy      =   _dragPos.y() + 8;

            auto    oldPen      =   hDC.pen();
            auto    oldBrush    =   hDC.brush();

            /// 半透明背景框 + 边框
            QColor  bg  =   palette().color(QPalette::Base);
            bg.setAlpha(200);
            hDC.fillRect(QRect(dx, dy, boxW, boxH), bg);
            hDC.setPen(QPen(_dropValid ? indColor : palette().color(QPalette::Mid), 1));
            hDC.setBrush(Qt::NoBrush);
            hDC.drawRect(QRect(dx, dy, boxW - 1, boxH - 1));

            /// 图标
            const int   icoY    =   dy + (boxH - _bmpHeight) / 2;
            hDC.drawPixmap(dx + pad, icoY, _bmpHeight, _bmpHeight
                , _icon, icoIdx * _bmpHeight, 0, _bmpHeight, _bmpHeight);

            /// 名称文本
            hDC.setPen(QPen(palette().color(QPalette::Text)));
            hDC.drawText(QRect(dx + pad + _bmpHeight + gap, dy, textW, boxH)
                , Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine, name);

            hDC.setPen(oldPen);
            hDC.setBrush(oldBrush);
        }
    }

    /// 开始内联重命名(F2 激活): 覆盖当前选中行的文本区域,预填节点名称,回车提交/Esc 取消
    void    QtTree::startRename()
    {
        cancelRename();
        if (_curItem == nullptr)
            return;
        auto    node    =   _curItem->cast<FENode>();
        if (node == nullptr)
            return;
        /// 定位当前选中行矩形(text 区域为整行,编辑器覆盖文本起点到行尾)
        int     textX   =   0;
        int     textY   =   0;
        int     textW   =   0;
        bool    found   =   false;
        for (auto& data : _itemDatas)
        {
            if (data.item == _curItem)
            {
                textX   =   data.text.left + _bmpHeight * 2 + 4;
                textY   =   data.text.top;
                textW   =   data.text.right - textX;
                found   =   true;
                break;
            }
        }
        if (!found || textW <= 0)
            return;
        _renameItem =   _curItem;
        auto    edit    =   new QLineEdit(this);
        edit->setText(C2Q(node->name()));
        edit->setGeometry(textX, textY, textW, _rowHeight);
        edit->selectAll();
        edit->setFocus();
        edit->show();
        _renameEditor   =   edit;
        connect(edit, &QLineEdit::editingFinished, this, [this, edit]()
        {
            commitRename();
        });
    }

    /// 提交重命名: 新名称非空且变化时通知主窗口(undo 命令);否则取消
    void    QtTree::commitRename()
    {
        if (_renameEditor == nullptr || _renameItem == nullptr)
            return;
        auto    node        =   _renameItem->cast<FENode>();
        QString newName     =   _renameEditor->text().trimmed();
        QString oldName     =   node ? C2Q(node->name()) : QString();
        if (node && !newName.isEmpty() && newName != oldName)
            emit    signalRenameRequested(_renameItem, newName);
        cancelRename();
    }

    /// 取消重命名: 销毁编辑器并清空状态
    void    QtTree::cancelRename()
    {
        if (_renameEditor)
        {
            _renameEditor->disconnect();
            _renameEditor->deleteLater();
            _renameEditor   =   nullptr;
        }
        _renameItem =   nullptr;
    }

    static void dbglog(const char* msg)
    {
        std::ofstream f("d:/miniSoftRender/clipdebug.log", std::ios::app);
        if (f.is_open()) f << msg << std::endl;
    }

    void    QtTree::keyPressEvent(QKeyEvent *event)
    {
        if (event->modifiers() & Qt::ControlModifier)
        {
            switch (event->key())
            {
                case Qt::Key_C:
                {
                    auto fn = _curItem ? _curItem->cast<FENode>() : nullptr;
                    char buf[256];
                    sprintf(buf, "[QtTree] Ctrl+C _curItem=%p cast<FENode>=%p", (void*)_curItem.get(), (void*)fn);
                    dbglog(buf);
                    if (fn)
                        emit signalCopyNode(_curItem);
                    return;
                }
                case Qt::Key_X:
                    if (_curItem && _curItem->cast<FENode>())
                        emit signalCutNode(_curItem);
                    return;
                case Qt::Key_V:
                    dbglog("[QtTree] Ctrl+V");
                    emit signalPasteNode();
                    return;
                default:
                    break;
            }
        }
        if (event->key() == Qt::Key_Escape)
        {
            /// 重命名中 Esc 优先取消编辑器(不触发剪切取消)
            if (_renameEditor)
            {
                cancelRename();
                return;
            }
            if (_cutItem)
            {
                /// Esc 取消剪切: 清绘制状态并通知主窗口清空剪贴板
                _cutItem    =   nullptr;
                update();
                emit    signalCancelCut();
                return;
            }
        }
        /// F2 激活内联重命名(仅 FENode 可重命名);重命名中 F2 重新激活当前项
        if (event->key() == Qt::Key_F2)
        {
            if (_curItem && _curItem->cast<FENode>())
            {
                if (_renameEditor)
                    commitRename();
                startRename();
            }
            return;
        }
        QWidget::keyPressEvent(event);
    }
}
