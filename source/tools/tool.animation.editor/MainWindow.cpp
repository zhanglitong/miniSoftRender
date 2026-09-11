
#include    "MainWindow.h"
#include    "FEFileFormatHelper.hpp"
#include    "fileFormat/fepk/FEFormatFepj.hpp"

MainWindow::MainWindow()
{
    ui.setupUi(this);

    ui.action_savePrj->setShortcut(QKeySequence::Save);

    ui.widget_keyframe->linkScrollBar(ui.horizontalScrollBar);
    ui.modelTree->setup(ui.sceneViewer->scene());
    setTitile("");

    connect(ui.action_importModel,  SIGNAL(triggered()),    this,   SLOT(slotImportModel()));
    connect(ui.action_openPrj,      SIGNAL(triggered()),    this,   SLOT(slotOpenProject()));
    connect(ui.action_savePrj,      SIGNAL(triggered()),    this,   SLOT(slotSaveProject()));
    connect(ui.actionReset,         SIGNAL(triggered()),    this,   SLOT(slotReset()));
    connect(ui.action_redo,         SIGNAL(triggered()),    this,   SLOT(slotRedo()));
    connect(ui.action_undo,         SIGNAL(triggered()),    this,   SLOT(slotUndo()));
}
MainWindow::~MainWindow()
{
}
Scene   MainWindow::scene()
{
    return  ui.sceneViewer->scene();
}

void    MainWindow::slotImportModel()
{
}
void    MainWindow::slotOpenProject()
{
    auto    fileName    =   QFileDialog::getOpenFileName(this
        , C2Q("打开工程")
        , qApp->applicationDirPath()
        , C2Q("工程文件(*.fepj)"));

    String  gbkName    =   fileName.toLocal8Bit().data();
    if (scene()->open(gbkName.c_str()))
    {
        _projectName    =   fileName;
        setTitile(_projectName);
        QMessageBox::information(this, C2Q("提示"), C2Q("打开工程文件成功!"), QMessageBox::Ok);
    }  
    else
    {
        QMessageBox::information(this, C2Q("提示"), C2Q("打开工程文件失败!"), QMessageBox::Ok);
    }
}
void    MainWindow::slotSaveProject()
{
    if (scene()->nodeTree().topLevelNodes().empty())
        return;
    if (_projectName.isEmpty())
    {
        auto    fileName    =   QFileDialog::getSaveFileName( this, C2Q("另存为.."), "D:/", C2Q("工程文件(*.fepj)"));
        if (fileName.isEmpty())
            return;
        _projectName    =   fileName;
    }
    String  fileName    =   _projectName.toLocal8Bit().data();
    if(!scene()->save(fileName.c_str()))
        QMessageBox::information(this, C2Q("提示"), C2Q("保存失败!"), QMessageBox::Ok);
    else
        setTitile(_projectName);
}

void    MainWindow::slotReset()
{
    if (scene())
    {
        scene()->clear();
        _projectName.clear();
        setTitile("");
    }
}
void    MainWindow::slotRedo()
{
}
void    MainWindow::slotUndo()
{
}

void    MainWindow::closeEvent(QCloseEvent*event)
{
    ui.modelTree->close();
    ui.sceneViewer->close();
}