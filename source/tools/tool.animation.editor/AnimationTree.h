#pragma     once
#include    <QTreeView>
#include    <QShortcut>
#include    <QStandardItemModel>
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
public:
    /// <summary>
    /// 模型树选择对象同通知,
    /// </summary>
    /// <param name="item">被选择对象</param>
    /// <param name="multiSelect">是否是多选</param>
    void        selectObject(Object item,bool multiSelect);
signals:
    void        sigPaintEvent();
public slots:
    void        slotItemExpanded(const QModelIndex & index);
    void        slotItemCollapsed(const QModelIndex & index);
    void        slotSelectItemChanged(const QItemSelection&, const QItemSelection&);
    void        slotDeleteTrack();
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
    void            collectAllTrackItemChildren(AnimationItem* item, std::vector<FE::FEKeyFrameTrack*>& results);
private:
    FE::Objects             _objects;
    QStandardItemModel*     _model              =   nullptr;
    QMenu*                  _menu               =   nullptr;
    UiTickMgr*              _tickMgr            =   nullptr;
    AnimationItem*          _rootItem           =   nullptr;
    AnimationItem*          _curItem            =   nullptr;
    QShortcut*              _shortcutDelete     =   nullptr;
    bool                    _isSelectWhenMoveTo =   false;
};
