#include "udpwidget.h"
#include "ui_udpwidget.h"

UDPWidget :: UDPWidget (QWidget * parent):
    QWidget (parent),
    ui (new Ui :: UDPWidget)
{
    ui->setupUi (this);
   //Initialize udp
    udpSocket = new QUdpSocket (this);
    udpSocket->bind (QHostAddress :: Any, 8000);//Bind ip and port, can be Any, LocalHost
   //label display status
    statusText = statusText + "wait for connecting ..." + "\n";
    ui->lblStatus->setText (statusText);
   //Binding signal slot, react when data is received
    connect (udpSocket, SIGNAL (readyRead ()), this, SLOT (ProcessPendingDatagram ()));
}

void UDPWidget :: ProcessPendingDatagram ()
{
   //Wait for the data to be received before processing
    while (udpSocket-> hasPendingDatagrams ())
    {
        QByteArray recvData;
        recvData.resize (udpSocket-> pendingDatagramSize ());
        udpSocket-> readDatagram (recvData.data (), recvData.size (), & clientIp, & clientPort);//Read data and ip and port from the sender's package and assign it to the class variable
        statusText += "connet from" + clientIp.toString () + ":" + QString :: number (clientPort) + "";
        statusText += recvData + "\n";
       //Show to status label
        ui-> lblStatus-> setText (statusText);
       //forward it back
        //SocketSend ("from server:" + recvData, clientIp, clientPort);
    }
}

void UDPWidget :: SocketSend (QString sendStr, QHostAddress targetIp, quint16 targetPort)
{
    udpSocket-> writeDatagram (sendStr.toStdString (). c_str (), sendStr.length (), targetIp, targetPort);
}

UDPWidget :: ~ UDPWidget ()
{
    delete ui;
}

//unity object rotates left
void UDPWidget :: on_btnLeftRotate_clicked ()
{
    SocketSend ("leftrotate\n", clientIp, clientPort);
}

//unity object rotates right
void UDPWidget :: on_btnRightRotate_clicked ()
{
    SocketSend ("rightrotate\n", clientIp, clientPort);
}

//send image to unity
void UDPWidget :: on_btnSendImage_clicked ()
{
    cv::Mat test_img(1000, 1000, CV_8UC3);
    cv::randu(test_img, cv::Scalar(0, 0, 0), cv::Scalar(255, 255, 255));
    std::string message = Image2String::ucharMat2str(test_img,100);
    QString qMessage = QString(message.c_str());
    QString qPath = qApp->applicationDirPath() + "/test.txt";
    QFile qFile(qPath);
    if (qFile.open(QIODevice::WriteOnly)) {
        QTextStream out(&qFile);
        out << qMessage;
        qFile.close();
    }
    message += '\n';
    SocketSend (message.c_str(), clientIp, clientPort);
}
