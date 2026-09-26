#include    "QtTree.h"
#include    <algorithm>

#include    <QWidget>
#include    <QPainter>
#include    <QPaintEvent>
#include    <QVBoxLayout>
#include    <QScrollArea>
#include    <QPoint>
#include    "MainWindow.h"
#include    "animation/FEAnimation.hpp"


namespace   FE
{
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
        _icon.load(":/EditorRes/res/buttons.bmp");
        /// BMP 无 alpha 通道,将白色背景设为透明
        _icon.setMask(_icon.createMaskFromColor(Qt::white));
        _bmpHeight      =   (std::max)(_icon.height(),16);
        _bmpWidth       =   (std::max)(_icon.width(),512);
        QVBoxLayout*    vlayout = new QVBoxLayout(this);
        vlayout->setSpacing(0);
        vlayout->setContentsMargins(0, 0, 0, 0);
        _vScrollBar     =   new QScrollBar(Qt::Vertical, this);
        _vScrollBar->setValue(0); 

        _vScrollBar->setLayoutDirection(Qt::RightToLeft);

        vlayout->addWidget(_vScrollBar);

        updateScroll();

        /// 连接valueChanged信号到自定义槽函数
        QObject::connect(_vScrollBar,   &QScrollBar::valueChanged,  this,   &QtTree::onValueChanged);
        QObject::connect(this,          &QtTree::repaintItem,       this,   &QtTree::onPaint);
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

        /// 绘制文字,选中时使用高亮文字色保证可读性
        /// 如果是 FENode 且有动画组件,在名称后追加 [A数量]
        QString text    =   item.getName();
        if (item.item)
        {
            auto    node    =   item.item->cast<FENode>();
            if (node)
            {
                auto    anims   =   node->objects<FEAnimation>();
                if (!anims.empty())
                    text    +=  QString(" [A%1]").arg(anims.size());
            }
        }
        QRect   qRT(xStart, y, _hscrollMax, _bmpHeight);
        QRect   br;
        int     flags   =   Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine;

        auto    oldPen  =   hDC.pen();
        if (item.isSelect())
            hDC.setPen(QPen(palette().color(QPalette::HighlightedText)));
        hDC.drawText(qRT, flags, text, &br);
        hDC.setPen(oldPen);
    }
    void    QtTree::paintItem(QPainter& hDC,FEObject& item,int& x,int& y,int parentIndex,int& rowIndex)
    {
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
                hDC.fillRect(rowRt, palette().color(QPalette::Highlight));
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
    }


    void    QtTree::mousePressEvent(QMouseEvent*event)
    {
        _downPos    =   int2(event->position().x(),event->position().y());
        switch (event->button())
        {
        case Qt::LeftButton:
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
    void    QtTree::mouseMoveEvent(QMouseEvent*)
    {}
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
}
