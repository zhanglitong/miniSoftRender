#pragma once
#include    <QComboBox>

class ComboBox : public QComboBox
{
    Q_OBJECT
public:
    ComboBox(QWidget* parent = nullptr);
    ~ComboBox()
    {

    }
public:
    virtual void keyPressEvent(QKeyEvent* event) override;
signals:
    void    sigCurIndexTextChanged(int index, QString);
};
