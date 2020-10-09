#include "stereostreamersettings.h"
#include "ui_stereostreamersettings.h"

StereoStreamerSettings::StereoStreamerSettings(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::StereoStreamerSettings)
{
    ui->setupUi(this);
}

StereoStreamerSettings::~StereoStreamerSettings()
{
    delete ui;
}
