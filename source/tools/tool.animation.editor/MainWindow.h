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
    /// <summary>
    /// 设置标题栏
    /// </summary>
    /// <param name="fileName"></param>
    void    setTitile(QString fileName)
    {
        if (fileName.isEmpty())
            setWindowTitle("FEEditor - unnamed.fepj*");
        else
            setWindowTitle("FEEditor - " + fileName);
        
    }
public slots:
    void    slotImportModel();
    void    slotOpenProject();
    void    slotSaveProject();

    void    slotReset();
    void    slotRedo();
    void    slotUndo();
public:
    void    closeEvent(QCloseEvent *event)  override ;
protected:
    QString         _projectName;
    Ui::MainWindow  ui;
};