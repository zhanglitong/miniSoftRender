#pragma     once
#include    <vector>
#include    <map>
#include    <algorithm>
#include    <QWidget>
#include    <QScrollBar>
#include    <QMenu>

#include    "FEObject.h"
#include    "node/FENode.hpp"
#include    "graphic/FEScene.h"
#include    "FEDelegate.hpp"

using   FE::FEObject;

Q_DECLARE_METATYPE(FE::Object)
Q_DECLARE_METATYPE(FE::Objects)
Q_DECLARE_METATYPE(FE::int2)
Q_DECLARE_METATYPE(const FE::int2&)

namespace   FE
{
    struct  FERect
    {
        int left;
        int top;
        int right;
        int bottom;
    };

    static  auto    getName(FEObject& item)
    {
        auto    node    =   item.cast<FENode>();
        if (node)
            return  node->name();
        else
            return  item.className(); 
    }

    static  bool    isSelect(FEObject& item)
    {
      return  item.flags().hasFlag(FE::FLAG_SELECTED); 
    }
    static  void    setSelected(FEObject& item,bool bSelect)
    {
        if (bSelect)
            item.flags().addFlag(FE::FLAG_SELECTED);
        else
            item.flags().removeFlag(FE::FLAG_SELECTED);
    }

    static  bool    isVisible(FEObject& item)
    {
        return  item.flags().hasFlag(FE::FLAG_VISIBLE); 
    }
    static  void    setVisible(FEObject& item,bool bVisible)
    {
        if (bVisible)
            item.flags().addFlag(   ::FLAG_VISIBLE);
        else
            item.flags().removeFlag(FE::FLAG_VISIBLE);
    }

    static  bool    isExpand(FEObject& item)
    {
        return  item.flags().hasFlag(FE::FLAG_EXPAND); 
    }
    static  void    setExpand(FEObject& item,bool bExpand)
    {
        if (bExpand)
            item.flags().addFlag(FE::FLAG_EXPAND);
        else
            item.flags().removeFlag(FE::FLAG_EXPAND);
    }
    static  bool    nameIsValid(FEObject& object)
    {
        auto    node    =   object.cast<FENode>();
        if (node)
            return  node->nameIsValid();
        else
            return  true;
    }
    static  bool    isEmpty(FEObject& object)
    {
        return  object.objectCount() == 0;
    }
    static  aabb3dr objectAabb(FEObject& object)
    {
        auto    node    =   object.as<FENode>();
        if (node)
            return  node->globalAabb();
        else
            return  {};
    }
    static  Node    objectRoot(FEObject& object)
    {
        auto    node    =   object.as<FENode>();
        if (node)
            return  node->root();
        else
            return  nullptr;
    }
    static  void    setObjectSelected(FEObject& object)
    {
        object.flags().addFlag(FE::FLAG_SELECTED);
    }

    struct  ItemData
    {
    public:        
        FERect          checkBox;
        FERect          expands;
        FERect          text;
        Object          item;
        int             parentIndex;
        FEObject*       obj;
    public:
        inline  auto    getName() 
        {
            return  FE::getName(*item);
        }
        inline  bool    isSelect() 
        {
            return  FE::isSelect(*item);
        }
        inline  bool    isVisible() 
        {
            return  FE::isVisible(*item);
        }
        inline  bool    isExpand() 
        {
            return  FE::isExpand(*item);
        }
        inline  bool    nameIsValid()
        {
            return  FE::nameIsValid(*item);
        }
    };
    typedef std::vector<ItemData>   ArrayItemData;

    /// <summary>
    /// 参数选择的item集合，可以多选,第二个参数通知是否是多选状态
    /// </summary>
    using   EventSelects            =   FETMultiDelegate<void(Object,bool)>;
    using   EventLButtonDbClickeds  =   FETMultiDelegate<void(const int2&,Object)>;  
    /// <summary>
    /// 关于快速索引算法,之前采用记录已经展开的节点的方式
    /// </summary>
    class   QtTree : public QWidget
    {
        Q_OBJECT
    public:
        EventSelects            _selectEvts;
        EventLButtonDbClickeds  _lbDbClickeds;
    protected:
        Scene           _scene          =   nullptr;
        /// 只是用来保存上下文对象,不做任何操作
        Object          _contextItem    =   nullptr;
        /// 当前选择的节点
        Object          _curItem        =   nullptr;
        ArrayItemData   _itemDatas;
        /// 窗口绘制的对齐,边缘留白
        int             _leftMargin     =   4;
        int             _topMargin      =   4;
        int             _rowHeight      =   32;
        int             _space          =   16;
        FERect          _rect           =   {};
        /// 水平滚动的位置，在滚动事件中作处理
        int             _hscrollPos     =   0;
        /// 水平方向滚动条的宽度
        int             _hscrollMax     =   0;
        /// 图标大小
        int             _bmpWidth       =   16;
        int             _bmpHeight      =   16;
        /// 需要查找的字符串地址(不用参数目的是减少开销) 
        char*           _pSearchPtr         =   nullptr;
        /// 查找字符串的长度
        unsigned        _nameLens;
        //  需要查找的Item的Index
        unsigned        _searchInsIndex;
        /// 从哪一个item开始查找
        Object         _startSearchItem    =   nullptr;
        /// 查找标记
        bool            _startFlag          =   false;
        /// 为了实现向上搜索，用一个数组记录下来已经找到的item
        /// 如果点向上搜索，检测数组中是否有数据，如果有，则定位到最后一项
        /// 同时删除最后一项
        Objects         _arFinds;
        /// 隐藏队列
        Objects         _hideQuue;
        int2            _downPos;
        int2            _upPos;
        QPixmap         _icon;
        QScrollBar*     _vScrollBar  =   nullptr;
    public:
        QtTree(QWidget* parent = nullptr);
        
        virtual ~QtTree();
        /// <summary>
        /// 获取图标
        /// </summary>
        /// <returns></returns>
        QPixmap         icon() const
        {
            return  _icon;
        }
        int             rowHeight() const
        {
            return  _rowHeight;
        }
        const   Nodes&  roots()
        {
            if (_scene == nullptr)
                return  {};
            else
                return  _scene->nodeTree().topLevelNodes();
        }
        /// <summary>
        /// 设置app
        /// </summary>
        virtual void    setup(Scene scene);
        /// <summary>
        /// 释放插件
        /// </summary>
        virtual void    destroy();
        /// <summary>
        /// 鼠标右键上下文菜单所在的item
        /// </summary>
        virtual Node    ctxNode()
        {
            if (_contextItem)
                return  _contextItem->cast<FENode>();
            else
                return  nullptr;
        }
        virtual Object  ctxObject()
        {
            return  _contextItem;
        }
        /// <summary>
        /// 重写计算窗口滚动信息
        /// </summary>
        virtual void    updateScroll();
        /// <summary>
        ///
        /// </summary>
        /// <param name="item">当前选中树节点</param>
        /// <param name="expandTo">是否打开</param>
        /// <param name="beCenter">是否聚焦</param>
        /// <param name="forceVisible">强制可见</param>
        /// <param name="isSelf">是否是本身发起的树节点选择请求</param>
        virtual void    setTreeItemSelect(Object item, bool expandTo, bool beCenter, bool forceVisible, bool isSelf);
        /// <summary>
        ///  展开并定位到item
        ///  从item找父节点将他展开,直到根节点
        ///  然后计算到item的位置
        ///  然后将滚动条滚到这个位置
        /// </summary>
        virtual bool    gotoItem(Object node);
        /// <summary>
        /// 获取所有可是item的高度信息，用作计算滚动信息
        /// </summary>
        /// <returns></returns>
        virtual int     getItemHeight();
    protected:
        /// <summary>
        /// 重写以下方法响应事件处理
        /// </summary>
        /// <param name="event"></param>
        virtual void    onLButtonDown(QMouseEvent *event);
        virtual void    onLButtonUp(QMouseEvent *event);

        virtual void    onRButtonDown(QMouseEvent *event);
        virtual void    onRButtonUp(QMouseEvent *event);

        virtual void    onMButtonDown(QMouseEvent *event);
        virtual void    onMButtonUp(QMouseEvent *event);

        virtual void    onLButtonDbClicked(QMouseEvent *event);
        virtual void    onRButtonDbClicked(QMouseEvent *event);
        virtual void    onMButtonDbClicked(QMouseEvent *event);

        virtual void    onLButtonClicked(QMouseEvent *event);
        virtual void    onRButtonClicked(QMouseEvent *event);
        virtual void    onMButtonClicked(QMouseEvent *event);
    protected:
        void    closeEvent(QCloseEvent *event)  override ;
        /// <summary>
        /// 暂时保留，无用
        /// </summary>
        /// <param name="item"></param>
        void    pushItemToHideQueue(Object item);
        Object  popItemFromHideQueue(Object item);
        /// <summary>
        /// 获取图标的索引
        /// </summary>
        int     getIconIndex(FEObject* node);
        /// <summary>
        /// 重绘制
        /// </summary>
        void    postRepaint();
        /// <summary>
        /// 展开item
        /// </summary>
        void    expandItem(ItemData& item);
        /// <summary>
        /// 展开item
        /// </summary>
        void    expandItem(FEObject& item);
        /// <summary>
        /// check item
        /// </summary>
        void    checkItem(FEObject& item);
        /// <summary>
        /// uncheck item
        /// </summary>
        void    unCheckItem(FEObject& item);

        bool    selectItem(FEObject& item);
        /// <summary>
        /// 显示模型
        /// </summary>
        void    showModel(FEObject& item);
        /// <summary>
        /// 隐藏模型
        /// </summary>
        void    hideModel(FEObject& item);
    protected:
        int     getVScroll();
        int     getHScroll();
        void    drawItem(QPainter& hDC, ItemData& item, int x, int y);
        void    paintItem(QPainter& hDC,FEObject& item,int& x,int& y,int parentIndex);
    protected:
        void    paintEvent(QPaintEvent *event)              override ;
        void    mousePressEvent(QMouseEvent *event)         override ;
        void    mouseReleaseEvent(QMouseEvent *event)       override ;
        void    mouseDoubleClickEvent(QMouseEvent *event)   override ;
        void    mouseMoveEvent(QMouseEvent *event)          override ;
        void    wheelEvent(QWheelEvent *event)              override ;
        void    resizeEvent(QResizeEvent *event)            override ;
   
    public slots:
        void    onValueChanged(int va);
        void    onPaint();
    Q_SIGNALS:
        void    repaintItem();
        void    signalLButtonDbClicked(const QPoint& pt,Object node);
        void    signalContextMenu(const QPoint& pt,Object node);
    };
}

using   QtTree  =   FE::QtTree;
