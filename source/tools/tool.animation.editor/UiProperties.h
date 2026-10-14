#pragma     once
#include    <QWidget>
#include    "ui_Properties.h"
#include    "FEObject.h"
#include    "NodePropsWidget.h"


class   UiProperties : public QWidget
{
    Q_OBJECT
public:
    UiProperties(QWidget* parent = nullptr);
    ~UiProperties();
public:
    /// 设置当前显示对象(节点或动画,动画解析 owner 节点),非节点/动画对象忽略
    void    setObject(FE::FEObject* pObject);
    void    updateUi();
    /// <summary>
    /// 当前属性面板显示的对象(用于判断是否需切换)
    /// </summary>
    inline  FE::FEObject*  currentObject() const
    {
        return  _pCurrentObject;
    }

    /// 阻止所有属性控件的实时更新(undo/redo 回调执行期间使用)
    void    blockRealtimeUpdate(bool block);
public slots:
    void    slotSelectObject(FE::Object object);
private:
    Ui::widget_properties   ui;

    FE::FEObject*           _pCurrentObject =   nullptr;

    NodePropsWidget*        _nodeWidget     =   nullptr;
};
