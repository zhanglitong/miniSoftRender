#include "ComboBox.h"
#include <QKeyEvent>
#include <QLineEdit>

ComboBox::ComboBox(QWidget* parent)
    : QComboBox(parent)
{
}

void ComboBox::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return)
    {
        int     index   =   currentIndex();
        auto    text    =   lineEdit()->text();

        emit    sigCurIndexTextChanged(index, text);

        blockSignals(true);
        clearFocus();
        blockSignals(false);
    }
    else
    {
        QComboBox::keyPressEvent(event);
    }
}
