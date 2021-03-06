#ifndef STEREO_STREAMER2_H
#define STEREO_STREAMER2_H

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

namespace StereoStreamer2 {

    static const int max_clients_ = 5;
    //static const size_t max_buffer_length_ = 65535;
    static const size_t max_buffer_length_ = 1024;
    static const size_t header_buffer_ = 64;
    static const int client_timeout_ = 5000; // will close client if no message received after n ms
    static const char eom_token_ = '\n';
    static const char padding_char_ = '\r';
    static const char option_value_ = 1;
    static const int message_sleep_time_ = 0;
    static const int INVALID_CLIENT_ID = -1; //client id used to signify a client id error
    static const int SERVER_CLIENT_ID = -2; //client id held by the server
    static const int SEND_TO_ALL_CLIENTS = -3; //client id used to signify message should be sent to all clients
    static const std::string SERVER_FULL = "Server full";

    static const bool pad_packets_ = false;

    enum MsgType {MSG_TYPE_STRING, MSG_TYPE_IMAGE, MSG_TYPE_CLIENT_ID, MSG_TYPE_SERVER_MSG, MSG_TYPE_NOT_SET, MSG_TYPE_ERROR};

    struct ClientInfo
    {
        int id;
        SOCKET socket;
        std::chrono::steady_clock::time_point lastValidMsgTime;

        ClientInfo(){
            this->id = INVALID_CLIENT_ID;
            this->socket = INVALID_SOCKET;
            this->lastValidMsgTime = std::chrono::steady_clock::now();
        }

        ClientInfo(int id, SOCKET socket){
            this->id = id;
            this->socket = socket;
            this->lastValidMsgTime = std::chrono::steady_clock::now();
        }

        ClientInfo(int id, SOCKET socket, std::chrono::steady_clock::time_point lastValidMsgTime){
            this->id = id;
            this->socket = socket;
            this->lastValidMsgTime = lastValidMsgTime;
        }
    };

    class ServerMsg
    {
    public:
        int senderClientId;
        int targetClientId;
        MsgType msgType;
        int msgEndIndex;
        int msgIndex;
        std::string msg;

        ServerMsg();

        ServerMsg(int senderClientId, int targetClientId, MsgType msgType, std::string msg);

        ServerMsg(int senderClientId, int targetClientId, MsgType msgType, std::string msg, int msgEndIndex, int msgIndex);

        ServerMsg(std::string raw_server_message);

        std::string get_raw_server_message();

        std::vector<ServerMsg> splitPackets(size_t max_buffer_length, int header_space);

    private:
        void invalidMessage();
    };

    static bool sendServerMessage(SOCKET socket, ServerMsg msg);
    static bool sendMessagePacket(SOCKET socket, std::string msg);

    class Server  : public QObject {
        Q_OBJECT
    public:
        explicit Server(std::string ip="127.0.0.1", std::string port="8000",QObject *parent = nullptr);
        void assignThread(QThread* thread);
        void setIP(std::string ip){ip_ = ip;};
        void setPort(std::string port){port_ = port;};

        bool testConnected(int targetClientId);

        ~Server();

    public slots:
        void startServer();
        void stopServer();

        bool sendStringMessage(std::string msg, int targetClientId = SEND_TO_ALL_CLIENTS);
        bool sendImage(cv::Mat image, int targetClientId = SEND_TO_ALL_CLIENTS);
        bool sendImageThreaded(cv::Mat image, int targetClientId = SEND_TO_ALL_CLIENTS);

    private slots:
        void serverCycleThreaded();
        void serverCycle();

    private:
        std::string ip_;
        std::string port_;
        int num_clients_ = 0;

        QFuture<void> *qfuture_serverCycle = nullptr;

        SOCKET server_socket_;

        std::vector<StereoStreamer2::ClientInfo> clientInfos_;
        std::thread clientThreads_[max_clients_];

        QThread* thread_;

        QFuture<bool>* qfuture_sendThreaded;

        int process_client(StereoStreamer2::ClientInfo &new_client, std::vector<StereoStreamer2::ClientInfo> &client_array, std::thread &thread);
        void clientDisconnected(StereoStreamer2::ClientInfo &new_client, std::vector<StereoStreamer2::ClientInfo> &client_array);
        bool sendServerMessage(ServerMsg msg);

    signals:
        void finished();
        void serverCycleComplete();
    };

    class Client : public QObject {
        Q_OBJECT
    public:
        explicit Client(std::string ip="127.0.0.1", std::string port="8000" ,QObject *parent = nullptr);
        void assignThread(QThread* thread);
        void setIP(std::string ip){ip_ = ip;};
        void setPort(std::string port){port_ = port;};
        ClientInfo getClientInfo(){return clientInfo_;};

        bool testConnected();

        ~Client();

    public slots:
        bool startClient();
        bool stopClient();

        bool sendStringMessage(std::string msg, int targetClientId = SEND_TO_ALL_CLIENTS);
        bool sendImage(cv::Mat image, int targetClientId = SEND_TO_ALL_CLIENTS);
        bool sendImageThreaded(cv::Mat image, int targetClientId = SEND_TO_ALL_CLIENTS);

    private:
        std::string ip_;
        std::string port_;
        bool isClientRunning = false;

        StereoStreamer2::ClientInfo clientInfo_ = {INVALID_CLIENT_ID, INVALID_SOCKET};
        std::string collectivemsgs[max_clients_] = {"","","","",""};
        std::thread clientThread_;

        QFuture<bool>* qfuture_sendThreaded;

        QThread* thread_;

        int process_client(StereoStreamer2::ClientInfo &new_client);
        bool sendServerMessage(ServerMsg msg);

    private slots:

         void messageReceived(StereoStreamer2::ServerMsg msg);

    signals:
        void finished();
        void clientMessage(StereoStreamer2::ServerMsg);
    };
}

Q_DECLARE_METATYPE (StereoStreamer2::ServerMsg);

#endif // STEREO_STREAMER2_H
