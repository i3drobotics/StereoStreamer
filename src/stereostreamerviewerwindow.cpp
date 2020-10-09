#include "stereostreamerviewerwindow.h"
#include "ui_stereostreamerviewerwindow.h"

StereoStreamerViewerWindow::StereoStreamerViewerWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::StereoStreamerViewerWindow)
{
    ui->setupUi(this);
}

StereoStreamerViewerWindow::~StereoStreamerViewerWindow()
{
    delete ui;
}
