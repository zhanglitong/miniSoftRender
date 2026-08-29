#pragma     once

#include    <QtWidgets/QMainWindow>
#include    <QUndoStack>
#include    <QShortcut>
#include    <QMessageBox>
#include    <string>
#include    <map>
#include    "ui_MainWindow.h"


class   MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow();
    ~MainWindow();
protected:
    Ui::MainWindow  ui;
};