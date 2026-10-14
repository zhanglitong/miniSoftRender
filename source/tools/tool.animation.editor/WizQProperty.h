#pragma     once
#include    "PropsUi.h"
#include    "FEObject.h"
#include    "node/FENode.hpp"
#include    "animation/FEAnimation.hpp"
#include    <QCheckBox>
#include    <QLabel>
#include    <QHBoxLayout>
#include    <functional>

class WizQProperty : public QWidget
{
public:
    Q_OBJECT
public:
    WizQProperty(const QString& text, const FE::PropIndex& propIndex, QWidget* valueWidget, QWidget* parent);
public:
    /// <summary>
    /// 绑定对象,object 可以是 FENode 或 FEAnimation(动画时解析 owner 节点)
    /// preferredAnim 为动画对象时作为首选动画,否则取节点上的第一个动画
    /// </summary>
    void    setObject(FE::FEObject* object, FE::FEAnimation* preferredAnim = nullptr);

    void    updateFromObject();

    /// 阻止实时更新关键帧(undo/redo 回调执行期间使用)
    void    blockRealtimeUpdate(bool block) { _blockRealtimeUpdate = block; }
protected:
    virtual void    updateUiOfPropertyValue() = 0;

    /// 当前绑定的节点(动画对象时解析其 owner)
    FE::FENode*         node() const;
    /// 当前生效的动画(首选动画 -> 对象自身 -> 节点第一个动画)
    FE::Animation       animation() const;
    /// 实时编辑:写回节点属性,命中当前时间关键帧时同步更新关键帧值
    void                applyRealtime(double value);
    /// 编辑结束:推 NodePropsCmd,按 AUTO 开关自动插帧
    void                finishEdit(double beginValue, double endValue);
    /// 统一的刷新回调(属性面板/动画树/时间线)
    std::function<void()> makeRefreshCb();
    /// 节点没有动画时,在已开启的 undo macro 内创建动画(轨道组按当前属性补建)
    FE::Animation       ensureAnimCreated(FE::Node node, FE::Animation anim);
public slots:
    virtual void    slotTrackBoxToggled(bool toggled);
protected:
    QCheckBox*          _trackBox;
    QLabel*             _label;
    QHBoxLayout*        _layout;
    FE::FEObject*       _object         =   nullptr;
    FE::FEAnimation*    _preferredAnim  =   nullptr;
    FE::PropIndex       _propIndex;
    bool                _blockRealtimeUpdate = false;
};

class   WizDoubleQProperty : public WizQProperty
{
public:
    Q_OBJECT
public:
    WizDoubleQProperty(const QString& text, const FE::PropIndex& propIndex, QWidget* parent);
public:
    DoubleEditingSpinBox*   valueBox() const
    {
        return  _valueBox;
    }
protected:
    virtual void    updateUiOfPropertyValue() override;
public slots:
    void    slotValueChanged(double value);
    void    slotEditFinished(double bV, double eV);
private:
    DoubleEditingSpinBox*   _valueBox;
};

class   WizBoolQProperty : public WizQProperty
{
public:
    Q_OBJECT
public:
    WizBoolQProperty(const QString& text, const FE::PropIndex& propIndex, QWidget* parent);
protected:
    virtual void    updateUiOfPropertyValue() override;
public slots:
    void    slotValueChanged(bool value);
private:
    QCheckBox*      _valueBox;
};
