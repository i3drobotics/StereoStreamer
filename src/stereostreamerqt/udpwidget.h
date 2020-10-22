#ifndef UDPWIDGET_H
#define UDPWIDGET_H

#include <QWidget>
#include <QFile>
#include <QUdpSocket>
#include "image2string.h"

namespace Ui {
class UDPWidget;
}

class UDPWidget: public QWidget
{
    Q_OBJECT

public:
    explicit UDPWidget (QWidget * parent = 0);
    ~ UDPWidget ();

private:
    Ui :: UDPWidget * ui;

    QString statusText;//status information
    QUdpSocket * udpSocket;//Socket
    QHostAddress clientIp;//Client ip
    quint16 clientPort;//Client port

    void SocketSend (QString sendStr, QHostAddress targetIp, quint16 targetPort);//Send data, you can send to the specified target, or broadcast

private slots:
    void ProcessPendingDatagram ();//Respond when data is received
    void on_btnLeftRotate_clicked ();
    void on_btnRightRotate_clicked ();
    void on_btnSendImage_clicked ();
};

#endif //UDPWIDGET_H
