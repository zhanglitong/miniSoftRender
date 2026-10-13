#pragma     once
#include    <vector>
#include    <map>
#include    <algorithm>
#include    <QWidget>
#include    <QScrollBar>
#include    <QMenu>
#include    <QTimer>
#include    <QLineEdit>

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
        /// 对 FENode: FLAG_SELECTED 改变会影响 renderBits(), 需要打 FLAG_PROP_STATE
        /// 让 FEFactoryRender 更新 instance buffer 到 GPU, shader 才能看到 RF_SELECTED
        auto*   node    =   item.cast<FENode>();
        if (node)
        {
            node->flags().addFlag(FENode::FLAG_PROP_STATE);
            node->fireChanged();
        }
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

    /// 拖拽投放模式
    enum    DropMode
    {
        DM_None,
        /// 嵌入目标: 成为目标节点子节点
        DM_Into,
        /// 同级排序: 插到目标之前
        DM_Before,
        /// 同级排序: 插到目标之后
        DM_After,
        /// 空白区域: 移到顶层
        DM_Top,
    };

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
        /// 水平方向滚动条的宽度
        int             _hscrollMax     =   0;
        /// 图标大小
        int             _bmpWidth       =   16;
        int             _bmpHeight      =   16;
        int2            _downPos;
        int2            _upPos;
        QPixmap         _icon;
        QScrollBar*     _vScrollBar  =   nullptr;
        /// 剪切待粘贴的节点(整行内容半透明绘制区分,粘贴/状态清除后置空)
        Object          _cutItem        =   nullptr;
        /// 左键按下的潜在拖拽节点(仅 FENode 可拖)
        Object          _dragItem       =   nullptr;
        /// 拖拽是否已激活(超过系统拖拽距离阈值)
        bool            _dragging       =   false;
        /// 拖拽当前光标位置
        QPoint          _dragPos;
        /// 当前投放目标(空表示移动到顶层)
        Object          _dropTarget     =   nullptr;
        /// 当前投放是否合法(非法目标/控件外为 false)
        bool            _dropValid      =   false;
        /// 当前投放模式(三区判定结果: 指示线绘制/投放信号语义)
        DropMode        _dropMode       =   DM_None;
        /// 拖拽自动滚动定时器(光标贴近视口上/下边缘时周期滚动)
        QTimer*         _autoScrollTimer    =   nullptr;
        /// 每次自动滚动步长(像素,负值向上,越靠边越快)
        int             _autoScrollStep     =   0;
        /// 内联重命名编辑器(F2 激活,覆盖当前选中行的文本区域)
        QLineEdit*      _renameEditor       =   nullptr;
        /// 正在重命名的节点(仅 FENode)
        Object          _renameItem         =   nullptr;
    public:
        QtTree(QWidget* parent = nullptr);
        
        virtual ~QtTree();
        /// <summary>
        /// 获取图标
        /// </summary>
        /// <returns></returns>
        inline  QPixmap icon() const
        {
            return  _icon;
        }
        inline  int     rowHeight() const
        {
            return  _rowHeight;
        }
        /// <summary>
        /// 当前选中的items
        /// </summary>
        /// <returns></returns>
        inline  Objects  selected() const
        {
            if (_curItem)
                return  {_curItem};
            else
                return  {};
        }
        /// 当前选中项(可为 null)
        Object  currentItem() const { return _curItem; }
        /// <summary>
        /// 根节点
        /// </summary>
        /// <returns></returns>
        const   Nodes&  roots()
        {
            static  Nodes   s_empty;
            if (_scene == nullptr)
                return  s_empty;
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
        virtual void    reset();
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
        /// 设置剪切待粘贴节点(模型树中该行内容半透明绘制区分);传空清除剪切状态
        void    setCutNode(Object node)
        {
            _cutItem    =   node;
            update();
        }
        /// 编程式选中节点(粘贴/重挂后调用): 与鼠标点选等效,同步 _curItem 并广播事件
        void    selectNode(Object item)
        {
            if (item)
                selectItem(*item);
        }
        /// 按控件坐标反查行节点(未命中返回空)
        Object  itemAt(const QPoint& pt) const;
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
        /// 拖拽投放合法性(按投放模式): 空目标=顶层;同父嵌入=修改父节点(恒合法),
        /// 同父前/后=兄弟排序(顺序无变化则非法);跨父嵌入/前/后恒合法;禁止自身/后代(防成环)
        bool    canDropOn(Object target, DropMode mode) const;
        /// 拖拽自动滚动: 光标进入视口上/下边缘区域后周期滚动,越靠边速度越快
        void    updateAutoScroll();
        /// 根据当前光标位置重新计算投放目标/模式/合法性
        void    updateDropState();
        /// 自动滚动定时器触发: 执行滚动并刷新投放判定
        void    onAutoScrollTimeout();
        /// 绘制拖拽视觉: 跟随光标的拖拽图像(图标+名称)、目标行高亮/插入线/顶层指示线
        void    drawDragVisual(QPainter& hDC);
        /// 开始内联重命名(F2 激活,覆盖当前选中行文本区域)
        void    startRename();
        /// 提交重命名: 名称变化时发信号通知主窗口(undo 命令)
        void    commitRename();
        /// 取消重命名: 销毁编辑器并清空状态
        void    cancelRename();
    protected:
        /// 键盘快捷键: Ctrl+C 复制 / Ctrl+X 剪切 / Ctrl+V 粘贴 / Esc 取消 / F2 重命名
        void    keyPressEvent(QKeyEvent *event) override;
        int     getVScroll();
        int     getHScroll();
        void    drawItem(QPainter& hDC, ItemData& item, int x, int y);
        void    paintItem(QPainter& hDC,FEObject& item,int& x,int& y,int parentIndex,int& rowIndex,bool parMatch=false);
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
        /// 拖拽投放完成: node 按投放模式挂接(嵌入/前后插入/顶层),仅本控件内部拖拽
        void    signalNodeDropped(Object node, Object target, DropMode mode);
        /// 复制节点到剪贴板(Ctrl+C,仅 FENode)
        void    signalCopyNode(Object node);
        /// 剪切节点到剪贴板(Ctrl+X,仅 FENode)
        void    signalCutNode(Object node);
        /// 请求粘贴(Ctrl+V): 源节点与粘贴位置由主窗口剪贴板状态决定
        void    signalPasteNode();
        /// Esc 取消剪切: 主窗口清空剪贴板,树恢复节点正常绘制
        void    signalCancelCut();
        /// F2 重命名请求: 参数为节点与新名称,主窗口创建 RenameNodeCmd
        void    signalRenameRequested(Object node, const QString& newName);
        /// 树节点选择变化通知
        void    signalSelect(Object node);
    };
}

using   QtTree  =   FE::QtTree;
