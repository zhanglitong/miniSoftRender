#include    "QtTree.h"
#include    <algorithm>

#include    <QWidget>
#include    <QPainter>
#include    <QPaintEvent>
#include    <QVBoxLayout>
#include    <QScrollArea>
#include    <QPoint>


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

    enum    ImageIndex
    {
        ID_UNCHECK,
        ID_CHECK,
        ID_HALF,
        ID_SUB,
        ID_PLUS,
        /// 工程图标索引
        ID_PRJ,
        /// 节点
        ID_NODE,
        /// 几何体
        ID_GEO,
        /// 图层
        ID_LAYER,
        /// image
        ID_LAYER_IMAGE,
        /// dem
        ID_LAYER_DEM,
        /// 标会图层图标索引
        ID_LAYER_MARK,
        /// 动画图标
        ID_ANIMATION,
        /// 属性图标
        ID_ATTRIBUTE,
        /// 材质
        ID_MATERIAL,
        /// 数据数据
        ID_DATA_OBJECT,
        /// 定时器图标
        ID_TIMER,
        /// 矢量图层
        ID_LAYER_VEC,
        /// 视频
        ID_VIDEO,
        /// 声音
        ID_SOUND,
        /// UI
        ID_UI,
        /// 图片
        ID_IMAGE,
        /// 倾斜摄影
        ID_QXSY,
        /// 数据集合
        ID_FILES,
        /// 日光
        ID_SUN,
        /// 聚光灯
        ID_SPOT,
        /// 水面
        ID_WATER,
        /// 音乐
        ID_MUSIC,
        /// <summary>
        /// 对象
        /// </summary>
        ID_OBJECT,
    };

    QtTree::QtTree(QWidget* parent)
        :QWidget(parent)
    {
        _icon.load(":/EditorRes/res/buttons.bmp");
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

    
    void    QtTree::setApp(Scene scene)
    {   
        _scene      =   scene;
        if (!roots().empty())
            updateScroll();
    }

    void    QtTree::destroy()
    {
        _scene              =   nullptr;
        _curItem            =   nullptr;
        _startSearchItem    =   nullptr;
        _contextItem        =   nullptr;
        _nameLens           =   0;
        _searchInsIndex     =   0;
        _hideQuue.clear();
        _arFinds.clear();
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
        auto    old     =   _curItem;
        
        if (_curItem)
        {
            FE::setSelected(*_curItem,false);
        }
        _curItem    =   &item;
        if (_curItem)
        {
            FE::setSelected(*_curItem,true);
        }
        if (_selectEvt)
        {
            _selectEvt(old,_curItem);
        }
        return  true;
    }

    void    QtTree::pushItemToHideQueue(Object item)
    {
        Object   pNodeItem = item;
        if (pNodeItem == 0)
        {
            return;
        }
        auto itr = std::find(_hideQuue.begin(), _hideQuue.end(), pNodeItem);
        if (itr == _hideQuue.end())
        {
            _hideQuue.push_back((Object)pNodeItem);
        }
    }

    Object  QtTree::popItemFromHideQueue(Object item)
    {
        if (_hideQuue.empty())
        {
            return  nullptr;
        }
        if (item)
        {
            /// TODO:
            Object   pNodeItem = nullptr;
            if (pNodeItem == 0)
                return nullptr;
            auto    itr = std::find(_hideQuue.begin(), _hideQuue.end(), pNodeItem);
            if (itr != _hideQuue.end())
            {
                item = *itr;
                _hideQuue.erase(itr);
            }
            else
            {
                item = nullptr;
            }
        }
        else
        {
            item = _hideQuue.back();
            _hideQuue.pop_back();
        }
        return  item;
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
        /// 滚动条窗口宽度
        int     scrollW =   _vScrollBar->width();
        _hscrollMax     =   _rect.right - _rect.left - scrollW - _leftMargin * 2;

        int     icoIdx  =   getIconIndex(item.item);
        int     xStart  =   x;
        int     yOff    =   (_rowHeight - _bmpHeight) / 2;
                y       +=  yOff;
        /// 绘制展开(+ - )图标
        if (item.isExpand())
        {
            setRect(&item.expands,xStart,y,xStart + _bmpHeight,y + _bmpHeight);
            hDC.drawPixmap(xStart,y,_bmpHeight,_bmpHeight,_icon,ID_SUB * _bmpHeight,0,_bmpHeight,_bmpHeight);
            xStart  +=  _bmpHeight;
            xStart  +=  0;
            hDC.drawPixmap(xStart,y,_bmpHeight,_bmpHeight,_icon,icoIdx * _bmpHeight,0,_bmpHeight,_bmpHeight);
            xStart +=   _bmpHeight;
            xStart +=   0;
        }
        else
        {   
            if (item.item->objectCount() != 0)
            {
                setRect(&item.expands, xStart, y, xStart + _bmpHeight, y + _bmpHeight);

                hDC.drawPixmap(xStart,y,_bmpHeight,_bmpHeight,_icon,ID_PLUS * _bmpHeight,0,_bmpHeight,_bmpHeight);
                xStart += _bmpHeight;
                xStart += 0;

                hDC.drawPixmap(xStart,y,_bmpHeight,_bmpHeight,_icon,icoIdx * _bmpHeight,0,_bmpHeight,_bmpHeight);
                xStart += _bmpHeight;
                xStart += 0;
            }
            else
            {
                setRect(&item.expands, xStart, y, xStart + _bmpHeight, y + _bmpHeight);
                hDC.drawPixmap(xStart,y,_bmpHeight,_bmpHeight,_icon,    ID_SUB * _bmpHeight,0,_bmpHeight,_bmpHeight);
                xStart += _bmpHeight;
                xStart += 0;

                hDC.drawPixmap(xStart,y,_bmpHeight,_bmpHeight,_icon,    icoIdx * _bmpHeight,0,_bmpHeight,_bmpHeight);
                xStart +=   _bmpHeight;
                xStart +=   0;
            }
        }
        /// 绘制checkbox图标
        if (item.isVisible())
        {
            setRect(&item.checkBox,xStart,y,xStart + _bmpHeight,y + _bmpHeight);
            hDC.drawPixmap(xStart,y,_bmpHeight,_bmpHeight,_icon,    ID_CHECK * _bmpHeight,0,_bmpHeight,_bmpHeight);
           
            xStart  +=  _bmpHeight;
            xStart  +=  4;
        }
        else
        {
            setRect(&item.checkBox,xStart,y,xStart + _bmpHeight,y + _bmpHeight);
            hDC.drawPixmap(xStart,y,_bmpHeight,_bmpHeight,_icon,    ID_UNCHECK * _bmpHeight,0,_bmpHeight,_bmpHeight);

            xStart  +=  _bmpHeight;
            xStart  +=  4;
        }
        
        FERect  rect    =   {xStart,y,_hscrollMax,y + _bmpHeight};
        FERect  srcRT   =   rect;
        /// 绘制文字
        /// TCHAR   buf[512]=   {0};
        /// getItemTextIn(item.item,buf);

        QRect   qRT(xStart,y, _hscrollMax, _bmpHeight);
        QString text    =   item.getName();
        QRect   br;
        int     flags   =   Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine;
        hDC.drawText(qRT,flags,text,&br);
        
        rect.right      =   rect.left + br.width();
        
        if (item.isSelect())
        {
            auto    oldPen  =   hDC.pen();
            // 设置边框
            QPen pen(Qt::blue);        // 蓝色边框
            pen.setWidth(1);           // 线宽2像素
            pen.setStyle(Qt::DashLine); // 虚线样式
            hDC.setPen(pen);

            // 设置填充
            QBrush brush(Qt::green, Qt::Dense3Pattern); // 绿色+密集点图案
            hDC.setBrush(brush);
            QRect   rtTmp(rect.left,rect.top, rect.right - rect.left, rect.bottom - rect.top);
            hDC.drawRect(rtTmp);
            hDC.setPen(oldPen);
        }
        QRect   rtTmp(rect.left,rect.top, rect.right - rect.left, rect.bottom - rect.top);
        hDC.drawText(rtTmp,flags,text);

        rect.right  +=  40;
        rect.top    -=  4;
        rect.bottom +=  4;
        item.text   =   rect;
    }
    void    QtTree::paintItem(QPainter& hDC,FEObject& item,int& x,int& y,int parentIndex)
    {
        auto    rt      =   rect();

        /// 递归结束条件
        int     height  =   rt.height();
        if (y > height)
        {
            return;
        }
        int     xOffset    =   _space;
        int     yOffset    =   _rowHeight;
        if (!nameIsValid(item))
        {
            return;
        }
        if (!isEmpty(item))
        {
            if (isExpand(item))
            {
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
                    _itemDatas.push_back(temp);
                }
                x   +=  xOffset;
                y   +=  yOffset;
                /// 绘制数据节点
                item.traverseObject([this,&hDC,&x,&y,parentIndex](const FEObject& object,const FEObject&,const FEObject::FETrvsCtx&,uint)->bool
                {
                    paintItem(hDC,(FEObject&)object,x,y,parentIndex);
                    return  true;
                });
                /// for (auto itr = item->begin() ; itr != item->end();++ itr )
                /// {
                ///     paintItem(hDC,*itr,x,y,parentIndex);
                /// }
                x   -=  xOffset;
            }
            else
            {
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
                    _itemDatas.push_back(temp);
                }
                y   +=  yOffset;
            }
        }
        else
        {
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
                _itemDatas.push_back(temp);
            }
            y   +=  yOffset;
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

        float   fontSize    =   (float)font().pointSize();
        _rowHeight          =   _bmpHeight  + _bmpHeight/4;

        _itemDatas.clear();

        int     width   =   rt.width();
        int     height  =   rt.height();

        int     pos     =   getVScroll();
        int     x       =   _leftMargin - getHScroll();
        int     y       =   _topMargin  - pos;
        int     i       =   0;
        for (auto node: roots())
        {
            if (y > height)
                break;
            FEObject&   object  =   *node;
            paintItem(painter, object, x, y, i);
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
            if (_lbDbClicked)
            {
                _lbDbClicked(int2(pt.x,pt.y),item.item);
            }
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
        UNUSED(expandTo,beCenter,forceVisible,isSelf);
        
        auto    node    =   item->as<FENode>();
        UNUSED(node);
        if(beCenter)
        {
            /// CELLSceneBrowser& sceneBrowser = CELLSceneBrowser::Get(_app->ctx());
            /// auto    root    =   dynamic_cast<const FE::CELLNodeLonlat*>(objectRoot(*item).get());
            /// if (root == nullptr)
            /// {
            ///     sceneBrowser.moveTo(FE::objectAabb(*item), 1.0, CELLSceneBrowser::AabbFaceIndex::None);
            /// }
            /// else
            /// {
            ///     if (node)
            ///     {
            ///         auto    box =   node->globalAabb();
            ///         sceneBrowser.moveTo(box, 1.0, CELLSceneBrowser::AabbFaceIndex::None);
            ///     }
            /// }
        }
        /// if(forceVisible && node)
        /// {
        ///     node->addFlagWithChild(FENode::FLAG_VISIBLE);
        /// }
        FE::setObjectSelected(*item);
        /// if (node)
        /// {
        ///     _app->setContextNode(node);
        ///     prj->setSelectNode(node,false,true,this);
        /// }
    }

    bool    QtTree::gotoItem(Object pNode)
    {
        Objects     routes;
        int         temp    =   0;
        bool        bFind   =   false;
        for (auto node : roots())
        {
            node->traverseObject([this,&temp,&pNode,&routes,&bFind](const FEObject& object,const FEObject& parent,const FEObject::FETrvsCtx&,uint depth)->bool
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
            },1,true);
        }
        
        if (!bFind)
            return  false;

        /// 首先把所有父节点状态展开
        /// 目录可以计算节点的偏移量

        for (auto& var : routes)
        {
            FE::setExpand(*var,true);
        }
        int     diff    =   0;
        for (auto node : roots())
        {
            diff += _rowHeight;
            if (!isExpand(*node))
                continue;
            auto    result  =   node->traverseObject([this,&diff,&pNode,&bFind](const FEObject& object,const FEObject&,const FEObject::FETrvsCtx& ctx,uint depth)->bool
            {
                if (pNode.get() == &object)
                {
                    bFind   =   true;
                    return  false;
                }
                if (!nameIsValid((FEObject&)object))
                {
                    return  true;
                } 

                diff += _rowHeight;
                if (!isExpand((FEObject&)object) || object.objectCount() == 0)
                    return  true;
                else
                    return  object.traverseObject(ctx.callback,depth + 1,false);
            },1,false);
        }
        _vScrollBar->setValue(diff);
        /// 设置滚动位置 
        postRepaint();
        return  true;
    }

    int     QtTree::getItemHeight()
    {
        int     diff    =   0;
        if (roots().empty())
        {
            return  0;
        }
        for (auto node : roots())
        {   
            diff += _rowHeight;
            if (!isExpand(*node))
                continue;
            auto    result  =   node->traverseObject([this,&diff](const FEObject& object,const FEObject&,const FEObject::FETrvsCtx& ctx,uint depth)->bool
            {
                if (!nameIsValid((FEObject&)object))
                {
                    return  true;
                } 
                diff += _rowHeight;
                if (!isExpand((FEObject&)object) || object.objectCount() == 0)
                    return  true;
                else if(isExpand((FEObject&)object))
                    return  object.traverseObject(ctx.callback,depth + 1,false);
                else
                    return  true;
            },1,false);
            UNUSED(result);
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
