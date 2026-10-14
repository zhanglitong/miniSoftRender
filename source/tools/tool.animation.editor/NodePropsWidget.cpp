#include    "NodePropsWidget.h"

NodePropsWidget::NodePropsWidget(QWidget* parent)
    : PropsWidget(parent)
{
    _transGroup         =   new WizQPropGroup(u8"位置", this);
    auto    groupLayout =   new QVBoxLayout(_transGroup->propsWidget());

    auto    doubleProp  =   new WizDoubleQProperty("X", FE::PROP_TRANSFORM_X, _transGroup);
    doubleProp->valueBox()->setSingleStep(0.01);
    groupLayout->addWidget(doubleProp);
    _props[_transGroup].push_back(doubleProp);

    doubleProp  =   new WizDoubleQProperty("Y", FE::PROP_TRANSFORM_Y, _transGroup);
    doubleProp->valueBox()->setSingleStep(0.01);
    groupLayout->addWidget(doubleProp);
    _props[_transGroup].push_back(doubleProp);

    doubleProp  =   new WizDoubleQProperty("Z", FE::PROP_TRANSFORM_Z, _transGroup);
    doubleProp->valueBox()->setSingleStep(0.01);
    groupLayout->addWidget(doubleProp);
    _props[_transGroup].push_back(doubleProp);

    _rotGroup   =   new WizQPropGroup(u8"旋转", this);
    groupLayout =   new QVBoxLayout(_rotGroup->propsWidget());

    doubleProp  =   new WizDoubleQProperty("X", FE::PROP_ROTATE_X, _rotGroup);
    doubleProp->valueBox()->setSingleStep(1.0);
    groupLayout->addWidget(doubleProp);
    _props[_rotGroup].push_back(doubleProp);

    doubleProp  =   new WizDoubleQProperty("Y", FE::PROP_ROTATE_Y, _rotGroup);
    doubleProp->valueBox()->setSingleStep(1.0);
    groupLayout->addWidget(doubleProp);
    _props[_rotGroup].push_back(doubleProp);

    doubleProp  =   new WizDoubleQProperty("Z", FE::PROP_ROTATE_Z, _rotGroup);
    doubleProp->valueBox()->setSingleStep(1.0);
    groupLayout->addWidget(doubleProp);
    _props[_rotGroup].push_back(doubleProp);

    _scaleGroup =   new WizQPropGroup(u8"缩放", this);
    groupLayout =   new QVBoxLayout(_scaleGroup->propsWidget());

    doubleProp  =   new WizDoubleQProperty("X", FE::PROP_SCALE_X, _scaleGroup);
    doubleProp->valueBox()->setSingleStep(0.01);
    groupLayout->addWidget(doubleProp);
    _props[_scaleGroup].push_back(doubleProp);

    doubleProp  =   new WizDoubleQProperty("Y", FE::PROP_SCALE_Y, _scaleGroup);
    doubleProp->valueBox()->setSingleStep(0.01);
    groupLayout->addWidget(doubleProp);
    _props[_scaleGroup].push_back(doubleProp);

    doubleProp  =   new WizDoubleQProperty("Z", FE::PROP_SCALE_Z, _scaleGroup);
    doubleProp->valueBox()->setSingleStep(0.01);
    groupLayout->addWidget(doubleProp);
    _props[_scaleGroup].push_back(doubleProp);

    _showGroup  =   new WizQPropGroup(u8"显示", this);
    groupLayout =   new QVBoxLayout(_showGroup->propsWidget());
    auto    boolProp   =   new WizBoolQProperty(u8"显隐", FE::PROP_VISIBLE, _showGroup);
    groupLayout->addWidget(boolProp);
    _props[_showGroup].push_back(boolProp);

    auto    layout  =   new QVBoxLayout(this);

    layout->addWidget(_label);
    layout->addWidget(_transGroup);
    layout->addWidget(_rotGroup);
    layout->addWidget(_scaleGroup);
    layout->addWidget(_showGroup);
    layout->addItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));
}

void    NodePropsWidget::setObeject(FE::FEObject* object)
{
    _object =   object;

    /// FE 动画对象作为首选动画传给属性行,行内自行解析 owner 节点
    auto    anim    =   object ? object->cast<FE::FEAnimation>() : nullptr;
    for (auto& group : {_transGroup, _rotGroup, _scaleGroup, _showGroup})
    {
        for (auto& prop : _props[group])
            prop->setObject(object, anim);
    }

    updateUi();
}

void    NodePropsWidget::updateUi()
{
    for (auto& group : {_transGroup, _rotGroup, _scaleGroup, _showGroup})
    {
        for (auto& prop : _props[group])
            prop->updateFromObject();
    }
}

void    NodePropsWidget::blockRealtimeUpdate(bool block)
{
    for (auto& group : {_transGroup, _rotGroup, _scaleGroup, _showGroup})
    {
        for (auto& prop : _props[group])
            prop->blockRealtimeUpdate(block);
    }
}
