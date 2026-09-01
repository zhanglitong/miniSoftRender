
#include    "MainWindow.h"


MainWindow::MainWindow()
{
    ui.setupUi(this);

    ui.widget_keyframe->linkScrollBar(ui.horizontalScrollBar);
}

MainWindow::~MainWindow()
{
}
