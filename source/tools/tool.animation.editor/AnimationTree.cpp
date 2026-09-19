#include    "AnimationTree.h"
#include    <QHeaderView>
#include    <QMouseEvent>
#include    <QContextMenuEvent>
#include    <QWheelEvent>
#include    "UiTickMgr.h"
#include    "AnimationItem.h"
#include    "MainWindow.h"
#include    "mesh/FEMesh.hpp"

using    namespace    FE;

inline  QIcon   iconOfObject(FEObject* object)
{
    if (object == nullptr)                      return  OBJECT_ICON(ID_LAYER);
    else if (object->cast<FENode>())            return  OBJECT_ICON(ID_NODE);
    else if (object->cast<FEAnimation>())       return  OBJECT_ICON(ID_ANIMATION);
    else if (object->cast<FEKeyFrameTrack>())   return  OBJECT_ICON(ID_ATTRIBUTE);
    else if (object->cast<FEMaterial>())        return  OBJECT_ICON(ID_MATERIAL);
    else if (object->cast<FEMesh>())            return  OBJECT_ICON(ID_GEO);
    else                                        return  OBJECT_ICON(ID_OBJECT);
}
inline  auto    nameOfObject(FEObject& item)
{
    auto    node    =   item.cast<FENode>();
    if (node)
        return  node->name();
    else
        return  item.className(); 
}
/// REGIST_SHORTCUT 在 UiTickMgr.cpp 中定义
QShortcut*  REGIST_SHORTCUT(const std::string& str, QObject* parent);

static  std::string   ObjText(FEObject* obj)
{
    assert(obj->className());
    return  obj->className();
}

static  bool    IsExpand(FEObject& item)
{
    auto    node    =   item.cast<FENode>();
    if (node)
        return  node->flags().hasFlag(FE::FLAG_EXPAND);
    else
        return  item.flags().hasFlag(FE::FLAG_EXPAND);
}

AnimationTree::AnimationTree(QWidget* parent)
    : QTreeView(parent)
{
    _model      =   new QStandardItemModel(this);
    _model->setHorizontalHeaderLabels(QStringList()<<u8"动画");
    auto    invisibleItem  =   _model->invisibleRootItem();
    _rootItem   =   new AnimationItem(nullptr,u8"总纲", AnimationItem::IT_Par, nullptr, iconOfObject(nullptr));
    invisibleItem->appendRow(_rootItem);

    this->setStyleSheet(R"(QTreeView::item {height: 40px;})");

    setModel(_model);

    connect(this,   SIGNAL(sigPaintEvent()),                _tickMgr,   SLOT(slotDoPaint()));
    connect(this,   SIGNAL(expanded(const QModelIndex &)),  this,       SLOT(slotItemExpanded(const QModelIndex &)));
    connect(this,   SIGNAL(collapsed(const QModelIndex &)), this,       SLOT(slotItemCollapsed(const QModelIndex &)));
    connect(selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(slotSelectItemChanged(const QItemSelection&, const QItemSelection&)));

    /// 添加右键菜单
    _menu   =   new QMenu(this);
    QAction*    deleteTrack = new QAction("删除轨道", this);
    _menu->addAction(deleteTrack);
    connect(deleteTrack, SIGNAL(triggered()), this, SLOT(slotDeleteTrack()));
    _shortcutDelete     =   REGIST_SHORTCUT("Delete", this);
    connect(_shortcutDelete, &QShortcut::activated, [&]()
    {
        auto    globaPos    =   QCursor::pos();
        auto    localPos    =   mapFromGlobal(globaPos);

        if (rect().contains(localPos))
        {
            slotDeleteTrack();
        }
    });
}
Animation   AnimationTree::curAnimation()
{
    if (_curItem == nullptr)
        return  nullptr;
    else if(_curItem->object() == nullptr)
        return  nullptr;
    else
        return  _curItem->object()->cast<FEAnimation>();

}

bool    AnimationTree::isExpand(AnimationItem* item)
{
    QModelIndex index = item->index();
    if (index.isValid() && QTreeView::isExpanded(index)) 
        return  true;
    else
        return  false;
}

AnimationItem*  AnimationTree::rootItem() const
{
    return  _rootItem;
}

void    AnimationTree::reset()
{
    _objects    =   {};
    updateUi();
}

void    AnimationTree::linkToTickMgr(UiTickMgr* mgr)
{
    _tickMgr            =   mgr;
    int keyRowHeight    =   visualRect(_rootItem->index()).height();
    int timeRowHeight   =   header()->height();

    _tickMgr->setKeyRowHeight(keyRowHeight);
    _tickMgr->setTimeRowHeight(timeRowHeight);
    _tickMgr->setAniTree(this);
    connect(this, SIGNAL(sigPaintEvent()), _tickMgr, SLOT(slotDoPaint()));
}

void    AnimationTree::updateUi()
{
    _rootItem->removeRows(0, _rootItem->rowCount());
    /// 更新树
    AnimationItems  needExpand;
    for (auto obj : _objects)
    {
        if (obj->cast<FENode>())
        {
            auto    pItem   =   createItemForNode(obj->cast<FENode>(),needExpand);
            _rootItem->appendRow(pItem);
        }
        else
        {
            auto    pItem   =   createItemForObj(obj,needExpand);
            _rootItem->appendRow(pItem);
        }
    }
    for (auto& var : needExpand)
    {
        expand(_model->indexFromItem(var));
    }
}
void    AnimationTree::selectObject(Object item,bool multiSelect)
{
    _objects    =   {item};
    updateUi();
}


void    AnimationTree::slotItemExpanded(const QModelIndex& index)
{
    auto item = dynamic_cast<AnimationItem*>(_model->itemFromIndex(index));
    item->setIsExpanded(true);
}

void    AnimationTree::slotItemCollapsed(const QModelIndex& index)
{
    auto item = dynamic_cast<AnimationItem*>(_model->itemFromIndex(index));
    item->setIsExpanded(false);
}

void    AnimationTree::slotSelectItemChanged(const QItemSelection& cur, const QItemSelection& pre)
{
    _curItem    =   nullptr;
    for (auto& index : cur.indexes())
    {
        auto    item    =   dynamic_cast<AnimationItem*>(_model->itemFromIndex(index));
        item->setIsSelected(true);
        _curItem = item;
    }
    for (auto& index : pre.indexes())
    {
        auto    item    =   dynamic_cast<AnimationItem*>(_model->itemFromIndex(index));
        item->setIsSelected(false);
    }
}

void    AnimationTree::slotDeleteTrack()
{
    if (!_curItem)
    {
        return;
    }

    /// 删除当前所有轨道
    std::vector<FE::FEKeyFrameTrack*> tracks;
    collectAllTrackItemChildren(_curItem, tracks);

    if (tracks.empty())
    {
        return;
    }

    /// TODO: UNDO_STACK 待实现
    /// UNDO_STACK->beginMacro(u8"删除轨道");
    /// for (auto& pTrack : tracks)
    /// {
    ///     auto pAnim = pTrack->getAnimObject();
    ///     UNDO_STACK->push(new RemoveKeyframeTrackCmd(pAnim, CUR_ANIMMIXER, CUR_ANIMCLIP, pTrack));
    ///     if (pAnim->mixerGroupCnt() == 0)
    ///     {
    ///         UNDO_STACK->push(new SetAnimObjectCmd(pAnim->getObject(), nullptr));
    ///     }
    /// }
    /// UNDO_STACK->endMacro();
}

void    AnimationTree::slotDoubleClikced(const QModelIndex& index)
{
    auto item = dynamic_cast<AnimationItem*>(_model->itemFromIndex(index));

    while (item->parent() != _rootItem)
    {
        item = dynamic_cast<AnimationItem*>(item->parent());
    }

    auto    pObj    =   item->object();
    auto    pNode   =   pObj ? pObj->cast<FENode>() : nullptr;
    if (pNode)
    {
        if (_isSelectWhenMoveTo)
        {
            /// TODO: UNDO_STACK 待实现
            /// UNDO_STACK->beginMacro(u8"动画树定位");
            /// UNDO_STACK->push(new SelectCmd({ pNode }, true, SELECTEDOBJS, SELECT_FLAG_RECURSION));
            /// UNDO_STACK->endMacro();
        }

        /// TODO: CELLSceneBrowser 待移植
        /// CELLSceneBrowser& sceneBrowser = CELLSceneBrowser::Get(ENGINE->ctx());
        /// auto    box =   pNode->globalAabb();
        /// sceneBrowser.moveTo(box, 1.0, CELLSceneBrowser::AabbFaceIndex::None);
    }
}

void    AnimationTree::mousePressEvent(QMouseEvent* evt)
{
    QModelIndex index = indexAt(evt->pos());
    if (!index.isValid())
    {
        this->clearSelection();
    }
    QTreeView::mousePressEvent(evt);
}

void    AnimationTree::paintEvent(QPaintEvent* evt)
{
    QTreeView::paintEvent(evt);
    /// 通知tickMgr更新
    emit    sigPaintEvent();
}

void    AnimationTree::contextMenuEvent(QContextMenuEvent* event)
{
    if (!_curItem)
        return;
    _menu->exec(event->globalPos());

    QWidget::contextMenuEvent(event);
}

void    AnimationTree::wheelEvent(QWheelEvent* event)
{
    auto    scrollBar   =   verticalScrollBar();
    int     delta       =   event->angleDelta().y();
    int     val         =   scrollBar->value();
    if (delta > 0)
        scrollBar->setValue(val - 1);
    else if (delta < 0)
        scrollBar->setValue(val + 1);
}

void    AnimationTree::mouseDoubleClickEvent(QMouseEvent* event)
{
    auto    pos     =   event->pos();
    auto    index   =   indexAt(pos);
    if (index.isValid() && _model->itemFromIndex(index) != _rootItem)
    {
        slotDoubleClikced(index);
    }
}

AnimationItem*  AnimationTree::createItemForNode(FE::FENode* object, AnimationItems& needExpand)
{
    /// 建立树形结构
    /// node
    ///  - animation0
    ///    - track0
    ///    - track1
    ///  - animation1
    ///    - track0
    ///    - track1
    auto    rootItem    =   new AnimationItem(nullptr,nameOfObject(*object), AnimationItem::IT_Par, object, iconOfObject(object));
    /// 获取当前节点的所有动画数据
    auto    anims       =   object->objects<FEAnimation>();
    /// 遍历所有动画
    for (auto anim : anims)
    {
        auto    animItem    =   new AnimationItem(anim,anim->name().c_str(), AnimationItem::IT_Par, anim,iconOfObject(anim));
        rootItem->appendRow(animItem);
        auto    clip    =   anim->clip();
        assert(clip != nullptr);
        if (clip == nullptr)
            continue;
        auto    tracks  =   clip->tracks();
        for (auto track : tracks)
        {
            animItem->appendRow(new AnimationItem(anim,track->name().c_str(), AnimationItem::IT_Track, track,iconOfObject(track)));
        }
    }
    return  rootItem;
}

AnimationItem*  AnimationTree::createItemForObj(FE::FEObject* pObject, AnimationItems& needExpand)
{
    /// 建立树形结构
    /// Object(mat,geo...)
    ///  - animation0
    ///    - track0
    ///    - track1
    ///  - animation1
    ///    - track0
    ///    - track1
    auto        rootItem    =   new AnimationItem(nullptr,nameOfObject(*pObject), AnimationItem::IT_Par, pObject,iconOfObject(pObject));
    Animations  anims;
    pObject->traverseObject([&](const FEObject& object, const FEObject&, const FEObject::FETrvsCtx&, uint)->bool
    {
        auto    pAnim   =   dynamic_cast<const FEAnimation*>(&object);
        if (pAnim)
        {
            Animation   tmp =   (FEAnimation*)pAnim;
            anims.push_back(tmp);
        }
        return true;
    });
    /// 遍历所有动画
    for (auto anim : anims)
    {
        auto    animItem    =   new AnimationItem(anim,anim->name().c_str(), AnimationItem::IT_Par, anim,iconOfObject(anim));
        rootItem->appendRow(animItem);
        auto    clip    =   anim->clip();
        assert(clip != nullptr);
        if (clip == nullptr)
            continue;
        auto    tracks  =   clip->tracks();
        for (auto track : tracks)
        {
            animItem->appendRow(new AnimationItem(anim,track->name().c_str(), AnimationItem::IT_Track, track,iconOfObject(track)));
        }
    }
    return  rootItem;
}

void    AnimationTree::collectAllTrackItemChildren(AnimationItem* item, std::vector<FE::FEKeyFrameTrack*>& results)
{
    auto pTrack = item->animKeyframeTrack();
    if (pTrack)
    {
        results.push_back(pTrack);
    }
    for (int i = 0; i < item->rowCount(); ++i)
    {
        collectAllTrackItemChildren((AnimationItem*)item->child(i), results);
    }
}
