#ifndef UDP_WINDOW_H
#define UDP_WINDOW_H

#include <QMainWindow>
#include "udpwidget.h"

namespace Ui {
class UDPWindow;
}

class UDPWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit UDPWindow(QWidget *parent = nullptr);
    ~UDPWindow();

private:
    Ui::UDPWindow *ui;
};

#endif // UDP_WINDOW_H
