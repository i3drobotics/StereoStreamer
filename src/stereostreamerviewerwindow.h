#ifndef STEREO_STREAMER_VIEWER_WINDOW_H
#define STEREO_STREAMER_VIEWER_WINDOW_H

#include <QMainWindow>
#include "stereostreamerviewer.h"

namespace Ui {
class StereoStreamerViewerWindow;
}

class StereoStreamerViewerWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit StereoStreamerViewerWindow(QWidget *parent = nullptr);
    ~StereoStreamerViewerWindow();

private:
    Ui::StereoStreamerViewerWindow *ui;
};

#endif // STEREO_STREAMER_VIEWER_WINDOW_H
