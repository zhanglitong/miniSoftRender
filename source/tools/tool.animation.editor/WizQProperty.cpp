#include    "WizQProperty.h"
#include    "MainWindow.h"
#include    "UndoCommand.h"
#include    "animation/FEAnimationHelper.hpp"
#include    <QPointer>
#include    <algorithm>

WizQProperty::WizQProperty(const QString& text, const FE::PropIndex& propIndex, QWidget* vWidget, QWidget* parent)
    : QWidget(parent)
{
    _object         =   nullptr;
    _preferredAnim  =   nullptr;
    _propIndex      =   propIndex;

    /// 属性名称
    _label      =   new     QLabel(this);
    _label->setText(text);
    _label->setAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);

    /// 动画轨道勾选框
    _trackBox   =   new     QCheckBox(this);
    _trackBox->setChecked(false);
    _trackBox->setFocusPolicy(Qt::ClickFocus);
    _trackBox->setToolTip(u8"在当前时间为该属性添加/删除关键帧");
    connect(_trackBox, &QCheckBox::toggled, this, &WizQProperty::slotTrackBoxToggled);

    _layout     =   new     QHBoxLayout(this);

    QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    _label->setSizePolicy(sizePolicy);
    vWidget->setSizePolicy(sizePolicy);

    sizePolicy  =   QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    _trackBox->setSizePolicy(sizePolicy);

    _layout->addWidget(_label);
    _layout->addWidget(vWidget);
    _layout->addWidget(_trackBox);

    _layout->setContentsMargins({0, 0, 0, 0});

    _layout->setStretch(0, 1);
    _layout->setStretch(1, 2);
}

void    WizQProperty::setObject(FE::FEObject* object, FE::FEAnimation* preferredAnim)
{
    _object         =   object;
    _preferredAnim  =   preferredAnim;
}

FE::FENode*     WizQProperty::node() const
{
    if  (!_object)
        return  nullptr;
    if  (auto n = _object->cast<FE::FENode>())
        return  n;
    if  (auto a = _object->cast<FE::FEAnimation>())
    {
        auto    owner   =   a->owner();
        return  owner ? owner->cast<FE::FENode>() : nullptr;
    }
    return  nullptr;
}

FE::Animation   WizQProperty::animation() const
{
    if  (_preferredAnim)
        return  FE::Animation(_preferredAnim);
    if  (!_object)
        return  nullptr;
    if  (auto a = _object->cast<FE::FEAnimation>())
        return  FE::Animation(a);
    auto    n   =   node();
    if  (!n)
        return  nullptr;
    auto    anims   =   n->objects<FE::FEAnimation>();
    return  anims.empty() ? nullptr : FE::Animation(anims.front());
}

std::function<void()>   WizQProperty::makeRefreshCb()
{
    return  [guard = QPointer<MainWindow>(_mainApp)]()
    {
        if  (!guard)
            return;
        guard->noticeUpdatePropsUi();
        guard->refreshAnimTree();
        guard->refreshTimeline();
    };
}

FE::Animation   WizQProperty::ensureAnimCreated(FE::Node node, FE::Animation anim)
{
    if  (anim)
        return  anim;
    /// 节点没有动画:按当前属性所在轨道组创建(空 clip 自带该组轨道)
    auto    created =   FEAnimationHelper::createNodeAnimtion(node->ctx()
        , FE::NodeProperyBits(FE::PropertyKeyCmd::groupBit(_propIndex)));
    UNDO_STACK->push(new FE::CreateAnimCmd(node, created, makeRefreshCb()));
    return  created;
}

void    WizQProperty::updateFromObject()
{
    auto    n   =   node();

    /// 更新属性值
    updateUiOfPropertyValue();

    /// 更新轨道勾选框:当前时间目标轨道是否存在关键帧
    _trackBox->blockSignals(true);
    bool    checked =   false;
    if  (n)
    {
        auto    anim    =   animation();
        if  (anim)
            checked =   FE::PropertyKeyCmd::hasKeyAt(anim, _propIndex, _mainApp->curTime());
    }
    _trackBox->setChecked(checked);
    _trackBox->setEnabled(n != nullptr);
    _trackBox->blockSignals(false);
}

void    WizQProperty::slotTrackBoxToggled(bool toggled)
{
    auto    n   =   node();
    if  (!n || !UNDO_STACK)
        return;

    auto    cb  =   makeRefreshCb();
    if  (toggled)
    {
        /// 勾选:当前时间为该属性加关键帧,值取节点当前属性;没有动画则先创建
        UNDO_STACK->beginMacro(u8"添加属性关键帧");
        auto    anim    =   animation();
        anim    =   ensureAnimCreated(n, anim);
        UNDO_STACK->push(new FE::PropertyKeyCmd(n, anim, _propIndex, _mainApp->curTime(), true, cb));
        UNDO_STACK->endMacro();
    }
    else
    {
        /// 取消勾选:删除当前时间该属性的关键帧(带快照,可 undo)
        auto    anim    =   animation();
        if  (!anim)
            return;
        UNDO_STACK->beginMacro(u8"删除属性关键帧");
        UNDO_STACK->push(new FE::PropertyKeyCmd(n, anim, _propIndex, _mainApp->curTime(), false, cb));
        UNDO_STACK->endMacro();
    }
}

void    WizQProperty::applyRealtime(double value)
{
    auto    n   =   node();
    if  (!n)
        return;

    /// 实时修改节点属性
    FE::NodePropsCmd::writeProp(n, _propIndex, value);

    /// undo/redo 回调期间跳过关键帧实时更新,避免覆盖恢复值
    if  (_blockRealtimeUpdate)
        return;

    auto    anim    =   animation();
    /// 动画覆盖本地变换后,编辑值先落到采样变换,再据此覆盖关键帧,否则写入的仍是旧采样值
    if  (anim && FE::PropertyKeyCmd::isDriven(anim, _propIndex))
        FE::PropertyKeyCmd::syncFromNode(anim, n, _propIndex);

    if  (anim && FE::PropertyKeyCmd::hasKeyAt(anim, _propIndex, _mainApp->curTime()))
    {
        /// 当前时间在该属性关键帧上:实时覆盖关键帧值(不记录 undo)
        FEAnimationHelper::updateNodeKeyFrame(anim, _mainApp->curTime(), n
            , FE::NodeProperyBits(FE::PropertyKeyCmd::helperBit(anim, _propIndex)));
    }
}

void    WizQProperty::finishEdit(double bV, double eV)
{
    auto    n   =   node();
    if  (!n || !UNDO_STACK)
        return;
    if  (bV == eV)
        return;

    auto    cb      =   makeRefreshCb();
    auto    anim    =   animation();
    double  tm      =   _mainApp->curTime();
    bool    onKey   =   anim && FE::PropertyKeyCmd::hasKeyAt(anim, _propIndex, tm);

    UNDO_STACK->beginMacro(u8"修改属性值");
    /// 节点属性:bV -> eV(redo 写入 eV,undo 恢复 bV;实时阶段节点已是 eV,redo 等效重写)
    UNDO_STACK->push(new FE::NodePropsCmd(n, FE::NodePropsCmd::PropChanges{{_propIndex, bV, eV}}, cb));

    if  (onKey)
    {
        /// 当前时间在该属性关键帧上:关键帧值同步改为 eV,快照旧值支持 undo
        /// 只保留目标轨道快照,redo 时按节点当前值覆盖该时间关键帧
        auto    targets =   FE::PropertyKeyCmd::targetTracks(anim, _propIndex);
        auto    snaps   =   FE::UpdateKeyframeCmd::snapshotTracks(anim, tm);
        FE::UpdateKeyframeCmd::TrackSnapshots  filtered;
        for (auto& s : snaps)
        {
            if  (std::find(targets.begin(), targets.end(), s.track) != targets.end())
                filtered.push_back(std::move(s));
        }
        if  (!filtered.empty())
            UNDO_STACK->push(new FE::UpdateKeyframeCmd(n, anim, tm, std::move(filtered), cb));
    }
    else if (_mainApp->autoKeyframe())
    {
        /// AUTO 自动插帧:当前时间无关键帧时补一个
        anim    =   ensureAnimCreated(n, anim);
        UNDO_STACK->push(new FE::PropertyKeyCmd(n, anim, _propIndex, tm, true, cb));
    }
    UNDO_STACK->endMacro();
}

WizDoubleQProperty::WizDoubleQProperty(const QString& text, const FE::PropIndex& propIndex, QWidget* parent)
    : WizQProperty(text, propIndex, _valueBox = new DoubleEditingSpinBox(), parent)
{
    _valueBox->setParent(this);
    _valueBox->setAlignment(Qt::AlignCenter);
    _valueBox->setDecimals(5);
    _valueBox->setMinimum(-10000000);
    _valueBox->setMaximum( 10000000);
    _valueBox->setSingleStep(0.01);

    connect(_valueBox,   SIGNAL(valueChanged(double)),             this, SLOT(slotValueChanged(double)));
    connect(_valueBox,   SIGNAL(sigEditFinished(double, double)),  this, SLOT(slotEditFinished(double, double)));
}

void    WizDoubleQProperty::updateUiOfPropertyValue()
{
    auto    n   =   node();
    if  (!n)
        return;
    /// 被动画轨道接管时显示采样值,与视口保持一致
    real    value   =   FE::NodePropsCmd::readProp(n, _propIndex);
    bool    driven  =   FE::PropertyKeyCmd::readDriven(animation(), _propIndex, value);
    _valueBox->blockSignals(true);
    _valueBox->setValue(value);
    _valueBox->blockSignals(false);
    _valueBox->setToolTip(driven ? C2Q(u8"该通道由动画驱动,显示的是当前采样值") : QString());
    if  (_label)
        _label->setStyleSheet(driven ? QStringLiteral("color:#3aa0ff;") : QString());
}

void    WizDoubleQProperty::slotValueChanged(double value)
{
    applyRealtime(value);
}

void    WizDoubleQProperty::slotEditFinished(double bV, double eV)
{
    finishEdit(bV, eV);
}

WizBoolQProperty::WizBoolQProperty(const QString& text, const FE::PropIndex& propIndex, QWidget* parent)
    : WizQProperty(text, propIndex, _valueBox = new QCheckBox(), parent)
{
    _valueBox->setParent(this);

    connect(_valueBox, &QCheckBox::toggled, this, &WizBoolQProperty::slotValueChanged);
}

void    WizBoolQProperty::updateUiOfPropertyValue()
{
    auto    n   =   node();
    if  (!n)
        return;
    _valueBox->blockSignals(true);
    _valueBox->setChecked(FE::NodePropsCmd::readProp(n, _propIndex) > 0.5);
    _valueBox->blockSignals(false);
}

void    WizBoolQProperty::slotValueChanged(bool toggle)
{
    auto    n   =   node();
    if  (!n)
        return;

    real    bV  =   toggle ? 0.0 : 1.0;
    real    eV  =   toggle ? 1.0 : 0.0;

    /// 实时改节点,编辑提交逻辑(命令/关键帧/AUTO)与数值行一致
    FE::NodePropsCmd::writeProp(n, _propIndex, eV);
    finishEdit(bV, eV);
}
