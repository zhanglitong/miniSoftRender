#pragma     once
#include    <QWidget>
#include    <unordered_map>
#include    "PropsWidget.hpp"
#include    "WizQPropGroup.h"
#include    <QLabel>

class NodePropsWidget : public PropsWidget
{
    Q_OBJECT
public:
    NodePropsWidget(QWidget* parent = nullptr);
public:
    virtual void    setObeject(FE::FEObject* object) override;

    virtual void    updateUi() override;

    /// 阻止所有属性控件的实时更新(undo/redo 回调执行期间使用)
    void    blockRealtimeUpdate(bool block);
private:
    WizQPropGroup*                              _transGroup;
    WizQPropGroup*                              _rotGroup;
    WizQPropGroup*                              _scaleGroup;
    WizQPropGroup*                              _showGroup;

    std::unordered_map<WizQPropGroup*, std::vector<WizQProperty*>>   _props;
};
