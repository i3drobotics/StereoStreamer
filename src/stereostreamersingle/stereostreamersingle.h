#ifndef STEREO_STREAMER_SINGLE_H
#define STEREO_STREAMER_SINGLE_H

#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <thread>
#include <vector>
#include <cmath>
#include <chrono>
#include <stdexcept>

#include "image2string.h"

#include <QObject>
#include <QThread>
#include <QDebug>
#include <QCoreApplication>
#include <QtConcurrent>

#pragma comment (lib, "Ws2_32.lib")

namespace StereoStreamerSingle {

    //static const size_t max_buffer_length_ = 65535;
    static const size_t max_buffer_length_ = 1024;
    static const size_t header_buffer_ = 64;
    static const int client_timeout_ = 5000; // will close client if no message received after n ms
    static const char eom_token_ = '\n';
    static const char padding_char_ = '\r';
    static const char option_value_ = 1;
    static const int message_sleep_time_ = 0;

    static const bool pad_packets_ = false;

    std::vector<std::string> splitPackets(std::string message, size_t max_buffer_length, int header_space);
    bool sendMessage(SOCKET socket, std::string message);
    bool sendMessagePacket(SOCKET socket, std::string message);

    class Server  : public QObject {
        Q_OBJECT
    public:
        explicit Server(std::string ip="127.0.0.1", std::string port="8000",QObject *parent = nullptr);
        void assignThread(QThread* thread);
        void setIP(std::string ip){ip_ = ip;};
        void setPort(std::string port){port_ = port;};

        bool testConnected(SOCKET socket);

        ~Server();

    public slots:
        void startServer();
        void stopServer();

    private slots:
        void serverCycleThreaded();
        void serverCycle();

    private:
        std::string ip_;
        std::string port_;

        QFuture<void> *qfuture_serverCycle = nullptr;

        SOCKET server_socket_;
        SOCKET client_socket_;

        std::thread clientThread_;

        QThread* thread_;

        void process_client(SOCKET socket);
        void disconnectClient(SOCKET socket);

    signals:
        void finished();
        void serverCycleComplete();
    };

    class Client : public QObject {
        Q_OBJECT
    public:
        explicit Client(std::string ip="127.0.0.1", std::string port="8000", QObject *parent = nullptr);
        void assignThread(QThread* thread);
        void setIP(std::string ip){ip_ = ip;};
        void setPort(std::string port){port_ = port;};

        bool testConnected();

        ~Client();

    public slots:
        bool startClient();
        bool stopClient();

        bool clientSendStringMessage(std::string msg);
        bool clientSendUCharImage(cv::Mat image);
        bool clientSendFloatImage(cv::Mat image);
        bool clientSendUCharImageThreaded(cv::Mat image);
        bool clientSendFloatImageThreaded(cv::Mat image);

    private:
        std::string ip_;
        std::string port_;
        bool isClientRunning = false;

        SOCKET client_socket_ = INVALID_SOCKET;
        std::string collectivemsgs = "";
        std::thread clientThread_;

        QFuture<bool>* qfuture_clientSendThreaded;

        QThread* thread_;

        void process_client(SOCKET socket);

    private slots:

         void messageReceived(QString msg);

    signals:
        void finished();
        void clientMessage(QString);
    };
}

#endif // STEREO_STREAMER_SINGLE_H
