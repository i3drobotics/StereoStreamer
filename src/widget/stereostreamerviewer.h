#ifndef STEREO_STREAMER_VIEWER_H
#define STEREO_STREAMER_VIEWER_H

#include <QDebug>
#include <QWidget>
#include <QCoreApplication>

namespace Ui {
class StereoStreamerViewer;
}

/**
 * @brief QT widget for viewing tpc video stream
 */
class StereoStreamerViewer : public QWidget {
    Q_OBJECT

public:
    /**
    * @brief Construct a new viewer
    *
    * @param parent
    */
    explicit StereoStreamerViewer(QWidget* parent = 0);
    /**
     * @brief Destroy the Main Window object
     *
     */
    ~StereoStreamerViewer();

private:
    Ui::StereoStreamerViewer *ui;
};

#endif  // STEREO_STREAMER_VIEWER_H
