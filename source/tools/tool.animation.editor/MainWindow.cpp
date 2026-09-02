
#include    "MainWindow.h"


MainWindow::MainWindow()
{
    ui.setupUi(this);

    ui.widget_keyframe->linkScrollBar(ui.horizontalScrollBar);
    ui.modelTree->setApp(ui.playerWidget->scene());
}

MainWindow::~MainWindow()
{
}
