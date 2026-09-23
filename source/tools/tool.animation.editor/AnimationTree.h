#pragma     once
#include    <QTreeView>
#include    <QShortcut>
#include    <QStandardItemModel>
#include    <QUndoStack>
#include    "node/FENode.hpp"
#include    "animation/FEKeyFrameTrack.hpp"
#include    "AnimationItem.h"

class   UiTickMgr;
class   AnimationItem;

class   AnimationTree : public QTreeView
{
    Q_OBJECT
public:
    AnimationTree(QWidget* parent = nullptr);
public:

    void    linkToTickMgr(UiTickMgr* mgr);
    void    updateUi();
    void    setUndoStack(QUndoStack* stack) { _undoStack = stack; }
    QUndoStack* undoStack() const { return _undoStack; }
    /// 供 undo 命令直接操作 _objects 列表(不触发 updateUi)
    void    addObjectToList(Object item);
    void    removeObjectFromList(Object item);
    void    clearObjectList();
    void    setObjectList(const FE::Objects& objs);
    FE::Objects snapshotObjectList() const;
    void    setSelectWhenMoveTo(bool enable)
    {
        _isSelectWhenMoveTo = enable;
    }
    Animation   curAnimation();
    /// <summary>
    /// 判断是否是展开状态
    /// </summary>
    /// <param name="item"></param>
    /// <returns></returns>
    bool        isExpand(AnimationItem* item);
    /// <summary>
    /// 获取根节点
    /// </summary>
    /// <returns></returns>
    AnimationItem*  rootItem() const;
    /// <summary>
    /// 获取当前选中项
    /// </summary>
    /// <returns></returns>
    AnimationItem*  curItem() const
    {
        return  _curItem;
    }
    /// <summary>
    /// 重置
    /// </summary>
    void            reset();
    /// <summary>
    /// 递归收集所有 IT_Track 子项的关键帧轨道
    /// </summary>
    /// <param name="item"></param>
    /// <param name="results"></param>
    void    collectAllTrackItemChildren(AnimationItem* item, std::vector<FE::FEKeyFrameTrack*>& results);
public:
    /// <summary>
    /// 模型树选择对象同通知,
    /// </summary>
    /// <param name="item">被选择对象</param>
    /// <param name="multiSelect">是否是多选</param>
    void        selectObject(Object item,bool multiSelect);
    /// <summary>
    /// 添加对象到动画树(如果已存在则移除)
    /// </summary>
    /// <param name="item">目标对象</param>
    /// <returns>true=已添加, false=已移除</returns>
    bool        toggleObject(Object item, double curTime = 0);
    /// <summary>
    /// 判断对象是否已在动画树中
    /// </summary>
    bool        containsObject(Object item) const;
signals:
    void        sigPaintEvent();
public slots:
    void        slotItemExpanded(const QModelIndex & index);
    void        slotItemCollapsed(const QModelIndex & index);
    void        slotSelectItemChanged(const QItemSelection&, const QItemSelection&);
    void        slotDeleteTrack();
    void        slotCreateAnimation();
    void        slotDeleteAnimation();
    void        slotToggleEnable();
    void        slotClearAllAnimations();
    void        slotRemoveNode();
    void        slotDeleteTrackNode();
    void        slotToggleTrackEnable();
    void        slotDoubleClikced(const QModelIndex&);
public:
    virtual void    mousePressEvent(QMouseEvent* evt)           override;
    virtual void    paintEvent(QPaintEvent* evt)                override;
    virtual void    contextMenuEvent(QContextMenuEvent* event)  override;
    virtual void    wheelEvent(QWheelEvent* event)              override;
    virtual void    mouseDoubleClickEvent(QMouseEvent *event)   override ;
private:
    AnimationItem*  createItemForNode(FE::FENode* object,  AnimationItems& needExpand);
    AnimationItem*  createItemForObj(FE::FEObject* object, AnimationItems& needExpand);
private:
    FE::Objects             _objects;
    QStandardItemModel*     _model              =   nullptr;
    QMenu*                  _menu               =   nullptr;
    UiTickMgr*              _tickMgr            =   nullptr;
    QUndoStack*             _undoStack          =   nullptr;
    AnimationItem*          _rootItem           =   nullptr;
    AnimationItem*          _curItem            =   nullptr;
    QShortcut*              _shortcutDelete     =   nullptr;
    bool                    _isSelectWhenMoveTo =   false;
};
