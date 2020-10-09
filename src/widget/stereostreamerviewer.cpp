#include "stereostreamerviewer.h"
#include "ui_stereostreamerviewer.h"

StereoStreamerViewer::StereoStreamerViewer(QWidget *parent)
    : QWidget(parent), ui(new Ui::StereoStreamerViewer) {
    ui->setupUi(this);
}

StereoStreamerViewer::~StereoStreamerViewer() {
    delete ui;
}
