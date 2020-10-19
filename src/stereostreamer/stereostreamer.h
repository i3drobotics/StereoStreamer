#ifndef STEREO_STREAMER_H
#define STEREO_STREAMER_H

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

namespace StereoStreamer {

    static const int max_clients_ = 5;
    static const size_t max_buffer_length_ = 65535;
    static const int client_timeout_ = 5000; // will close client if no message received after n ms
    static const char eom_token_ = '\n';
    static const char option_value_ = 1;

    enum MsgType {MSG_TYPE_STRING, MSG_TYPE_IMAGE, MSG_TYPE_ERROR};

    struct ClientInfo
    {
        int id;
        SOCKET socket;
        std::chrono::steady_clock::time_point lastValidMsgTime;
    };

    struct ServerMsg
    {
        int clientId;
        MsgType msgType;
        std::string msg;
    };

    class Server  : public QObject {
        Q_OBJECT
    public:
        explicit Server(std::string ip="127.0.0.1", std::string port="8000",QObject *parent = nullptr);
        void assignThread(QThread* thread);
        void setIP(std::string ip){ip_ = ip;};
        void setPort(std::string port){port_ = port;};

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
        int num_clients_ = 0;

        QFuture<void> *qfuture_serverCycle = nullptr;

        SOCKET server_socket_;

        std::vector<StereoStreamer::ClientInfo> clientInfos_;
        std::thread clientThreads_[max_clients_];

        QThread* thread_;

        int process_client(StereoStreamer::ClientInfo &new_client, std::vector<StereoStreamer::ClientInfo> &client_array, std::thread &thread);
        void clientDisconnected(StereoStreamer::ClientInfo &new_client, std::vector<StereoStreamer::ClientInfo> &client_array);

    signals:
        void finished();
        void serverCycleComplete();
    };

    class Client : public QObject {
        Q_OBJECT
    public:
        explicit Client(std::string ip="127.0.0.1", std::string port="8000",bool enable_reader = false,QObject *parent = nullptr);
        void assignThread(QThread* thread);
        void setIP(std::string ip){ip_ = ip;};
        void setPort(std::string port){port_ = port;};
        ClientInfo getClientInfo(){return clientInfo_;};
        void enableReader(bool enable);

        bool isConnected();

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
        bool isReader_ = true;
        bool isClientRunning = false;

        StereoStreamer::ClientInfo clientInfo_ = {-1, INVALID_SOCKET};
        std::string collectivemsgs[max_clients_] = {"","","","",""};
        std::thread clientThread_;

        QFuture<bool>* qfuture_clientSendThreaded;

        QThread* thread_;

        int process_client(StereoStreamer::ClientInfo &new_client);
        bool clientSendMessage(ServerMsg msg);
        bool clientSendMessagePacket(std::string msg);

    private slots:

         void messageReceived(StereoStreamer::ServerMsg msg);

    signals:
        void finished();
        void clientMessage(StereoStreamer::ServerMsg);
    };
}

Q_DECLARE_METATYPE (StereoStreamer::ServerMsg);

#endif // STEREO_STREAMER_H
