
#include    "MainWindow.h"
#include    "FEFileFormatHelper.hpp"
#include    "fileFormat/fepk/FEFormatFepj.hpp"

MainWindow::MainWindow()
{
    ui.setupUi(this);

    ui.widget_keyframe->linkScrollBar(ui.horizontalScrollBar);
    ui.modelTree->setApp(ui.sceneViewer->scene());

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
    auto    fileNames   =   QFileDialog::getOpenFileNames(this
        , C2Q("打开工程")
        , qApp->applicationDirPath()
        , C2Q("工程文件(*.fepj)"));

    FEFileFormat    fmtText     =   FEFileFormat(".fepj","1.0.0.0","FE Buildin Format!");

    auto            reader  =   FEFileFormatHelper::queryReader(scene()->ctx(),fmtText);
    if (reader == nullptr)
    {
        QMessageBox::information(this, C2Q("提示"), C2Q("没有找到文件解析器读取文件!"), QMessageBox::Ok);
        return;
    }
    Strings     files;
    for (auto var : fileNames)
    {
        String  fileName    =   var.toLocal8Bit().data();
        files.push_back(fileName);
    }
    auto    objects =   reader->readFiles(files);
    Nodes   nodes;
    for (auto var : objects)
    {   
        Node    node    =   var->cast<FENode>();
        if (node == nullptr)
            continue;
        else
            nodes.push_back(node);
    }
    scene()->dispatchNodesToSystem(nodes);
    scene()->addNodesToTree(nodes);

    QMessageBox::information(this, C2Q("提示"), C2Q("打开工程文件成功!"), QMessageBox::Ok);
}
void    MainWindow::slotSaveProject()
{
    if (_projectName.isEmpty())
    {
        auto    fileName    =   QFileDialog::getSaveFileName( this, C2Q("另存为.."), "D:/", C2Q("工程文件(*.fepj)"));
        if (fileName.isEmpty())
            return;
        _projectName    =   fileName;
    }
    FEFileFormat    fmt(".fepj","1.0.0.0","FE Buildin Format!");
    fmt._type       =   FEFileFormat::DT_Model;
    fmt._mode       =   FEFileFormat::SM_FILE|FEFileFormat::SM_MEMORY;
    auto    writer  =   FEFileFormatHelper::queryWriter(scene()->ctx(),fmt);
    if (writer == nullptr)
    {
        QMessageBox::information(this, C2Q("提示"), C2Q("没有找到文件存储器写文件!"), QMessageBox::Ok);
        return;
    }
    auto    nodes   =   scene()->nodeTree().topLevelNodes();
    if (nodes.empty())
    {
        QMessageBox::information(this, C2Q("提示"), C2Q("没有数据需要!"), QMessageBox::Ok);
        return;
    }
    Objects objects(nodes.size());
    for (size_t i = 0; i < nodes.size(); i++)
    {
        objects[i]  =   nodes[i].get();
    }
    String  fileName    =   _projectName.toLocal8Bit().data();
    if(writer->writeFile(objects,fileName))
        QMessageBox::information(this, C2Q("提示"), C2Q("保存成功!"), QMessageBox::Ok);
    else
        QMessageBox::information(this, C2Q("提示"), C2Q("保存失败!"), QMessageBox::Ok);
}

void    MainWindow::slotRedo()
{
}
void    MainWindow::slotUndo()
{
}