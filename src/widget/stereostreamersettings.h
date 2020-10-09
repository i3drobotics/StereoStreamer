#ifndef STEREOSTREAMERSETTINGS_H
#define STEREOSTREAMERSETTINGS_H

#include <QWidget>

namespace Ui {
class StereoStreamerSettings;
}

class StereoStreamerSettings : public QWidget
{
    Q_OBJECT

public:
    explicit StereoStreamerSettings(QWidget *parent = nullptr);
    ~StereoStreamerSettings();

private:
    Ui::StereoStreamerSettings *ui;
};

#endif // STEREOSTREAMERSETTINGS_H
