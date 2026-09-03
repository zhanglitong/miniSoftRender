#pragma     once

#include    <QtWidgets/QMainWindow>
#include    <QUndoStack>
#include    <QShortcut>
#include    <QMessageBox>
#include    <string>
#include    <map>
#include    "ui_MainWindow.h"

#define     Q2C(qstr)     (qstr.toUtf8().data())
///const char* 转换为QString 
#define     C2Q(cstr)     (QString::fromUtf8(cstr))


class   MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow();
    ~MainWindow();
    /// <summary>
    /// 获取场景
    /// </summary>
    /// <returns></returns>
    Scene   scene();
public slots:
    void    slotImportModel();
    void    slotOpenProject();
    void    slotSaveProject();
    void    slotRedo();
    void    slotUndo();

protected:
    QString         _projectName;
    Ui::MainWindow  ui;
};