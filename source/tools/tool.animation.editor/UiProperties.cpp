#include    "UiProperties.h"
#include    "MainWindow.h"
#include    "node/FENode.hpp"
#include    "animation/FEAnimation.hpp"

UiProperties::UiProperties(QWidget* parent)
    : QWidget(parent)
{
    ui.setupUi(this);
    _pCurrentObject =   nullptr;

    _nodeWidget     =   new NodePropsWidget(this);
    ui.tabWidget->addTab(_nodeWidget, u8"节点");
    ui.tabWidget->setCurrentIndex(0);
}

UiProperties::~UiProperties()
{
}

void    UiProperties::setObject(FE::FEObject* pObject)
{
    if  (!pObject)
        return;
    /// 只支持节点或动画对象(动画解析其 owner 节点)
    auto    node    =   pObject->cast<FE::FENode>();
    auto    anim    =   node ? nullptr : pObject->cast<FE::FEAnimation>();
    if  (node == nullptr && anim == nullptr)
        return;
    /// 动画对象必须关联节点
    if  (anim)
    {
        auto    owner   =   anim->owner();
        if  (!owner || !owner->cast<FE::FENode>())
            return;
    }

    _pCurrentObject =   pObject;
    _nodeWidget->setObeject(_pCurrentObject);

    QString title   =   node
        ? QString::fromUtf8(node->name())
        : QString::fromUtf8(anim->name().c_str());
    _nodeWidget->setTitle(title);
}

void    UiProperties::updateUi()
{
    if  (!_pCurrentObject)
        return;
    _nodeWidget->updateUi();

    /// 节点/动画名称可能已变化(重命名 undo/redo),同步刷新标题
    auto    node    =   _pCurrentObject->cast<FE::FENode>();
    auto    anim    =   node ? nullptr : _pCurrentObject->cast<FE::FEAnimation>();
    QString title   =   node
        ? QString::fromUtf8(node->name())
        : (anim ? QString::fromUtf8(anim->name().c_str()) : QString());
    _nodeWidget->setTitle(title);
}

void    UiProperties::slotSelectObject(FE::Object object)
{
    if  (!object)
        return;
    if  (object->cast<FE::FENode>() || object->cast<FE::FEAnimation>())
        setObject(object.get());
}

void    UiProperties::blockRealtimeUpdate(bool block)
{
    if  (_nodeWidget)
        _nodeWidget->blockRealtimeUpdate(block);
}
