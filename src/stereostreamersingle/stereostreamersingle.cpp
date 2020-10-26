#include "stereostreamersingle.h"

namespace StereoStreamerSingle {

std::vector<std::string> splitPackets(std::string message, size_t max_buffer_length, int header_space){
    std::string srvMsg_msg = message;
    std::vector<std::string> srvMsg_msg_packets;
    //split message into packets
    size_t max_packet_length = max_buffer_length - header_space; //leave room for header in each packet
    int i = 0;
    while (true){
        int start_index = i*max_packet_length;
        if ((start_index + max_packet_length) < srvMsg_msg.size()){
            std::string srvMsg_msg_packet = srvMsg_msg.substr(start_index, max_packet_length);
            srvMsg_msg_packets.push_back(srvMsg_msg_packet);
            i++;
        } else {
            int end_index = srvMsg_msg.size();
            int length = end_index - start_index;
            std::string srvMsg_msg_packet = srvMsg_msg.substr(start_index, length);
            srvMsg_msg_packets.push_back(srvMsg_msg_packet);
            break;
        }
    }
    return srvMsg_msg_packets;
}

bool sendMessage(SOCKET socket, std::string msg){
    // add end of message token to end of string
    //msg += eom_token_;
    size_t total_size = msg.size();
    if (total_size < max_buffer_length_){
        return sendMessagePacket(socket, msg);
    } else {
        bool success = true;
        std::vector<std::string> srvMsg_packets = splitPackets(msg,max_buffer_length_,header_buffer_);
        for (std::vector<std::string>::iterator it = srvMsg_packets.begin() ; it != srvMsg_packets.end(); ++it){
            //raw_msg += eom_token_; // add end of message token to end of string
            qDebug() << "Packet size:" << it->size();
            success &= sendMessagePacket(socket,*it);
            if (!success) break;
            Sleep(message_sleep_time_); // sleep between sends
        }
        return success;
    }
}

bool sendMessagePacket(SOCKET socket, std::string message){
    if (message.size() > max_buffer_length_){
        qDebug() << "Unable to send message as it will overflow the stream buffer";
        return false;
    } else {
        if (socket != INVALID_SOCKET){
            std::string send_message = message;
            if (pad_packets_){
                // always send same size packet
                if (message.size() < max_buffer_length_){
                    std::string padded_message = send_message.append(std::string( (max_buffer_length_ - message.size() ), padding_char_));
                    send_message = padded_message;
                }
            }
            //send_message += eom_token_; // add end of message token to end of string
            int iResult = send(socket, send_message.c_str(), (int)strlen(send_message.c_str()), 0);
            if (iResult <= 0)
            {
                int error = WSAGetLastError();
                qDebug() << "send() failed: " << error;
                return false;
            } else {
                return true;
            }
        } else {
            qDebug() << "Invalid socket";
            return false;
        }
    }
}

Server::Server(std::string ip, std::string port, QObject *parent) : QObject(parent), ip_(ip), port_(port)
{
    clientThread_ = std::thread();
}

void Server::assignThread(QThread *thread){
    qDebug() << "Moving stereo streamer server to thread";
    thread_ = thread;
    this->moveToThread(thread_);
    connect(this, SIGNAL(finished()), thread_, SLOT(quit()));
    //connect(this, SIGNAL(finished()), this, SLOT(deleteLater()));
    connect(thread_, SIGNAL(finished()), thread_, SLOT(deleteLater()));
    thread_->start();
    thread_->setPriority(QThread::LowestPriority);
}

void Server::startServer(){
    WSADATA wsaData;
    struct addrinfo hints;
    struct addrinfo *server = NULL;
    server_socket_ = INVALID_SOCKET;

    //Initialize Winsock
    qDebug() << "Intializing Winsock...";
    //std::cout << "Intializing Winsock..." << std::endl;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    //Setup hints
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    //Setup Server
    qDebug() << "Setting up server on" << ip_.c_str() << ":" << port_.c_str();
    //std::cout << "Setting up server..." << std::endl;
    getaddrinfo(ip_.c_str(), port_.c_str(), &hints, &server);

    //Create a listening socket for connecting to server
    qDebug() << "Creating server socket...";
    //std::cout << "Creating server socket..." << std::endl;
    server_socket_ = socket(server->ai_family, server->ai_socktype, server->ai_protocol);

    //Setup socket options
    setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, &option_value_, sizeof(int)); //Make it possible to re-bind to a port that was used within the last 2 minutes
    setsockopt(server_socket_, IPPROTO_TCP, TCP_NODELAY, &option_value_, sizeof(int)); //Used for interactive programs

    //int ReceiveTimeout = 3000;
    //int e = setsockopt(server_socket_, SOL_SOCKET, SO_RCVTIMEO, (char*)&ReceiveTimeout, sizeof(int));

    //Assign an address to the server socket.
    qDebug() << "Binding socket...";
    //std::cout << "Binding socket..." << std::endl;
    bind(server_socket_, server->ai_addr, (int)server->ai_addrlen);

    //Listen for incoming connections.
    qDebug() << "Listening...";
    //std::cout << "Listening..." << std::endl;
    listen(server_socket_, SOMAXCONN);

    connect(this, SIGNAL(serverCycleComplete()), this, SLOT(serverCycleThreaded()));
    //serverCycle(); //do one non threaded cycle to inialise the server
    serverCycleThreaded();
}

void Server::serverCycleThreaded(){
    qfuture_serverCycle = new QFuture<void>(QtConcurrent::run(this, &Server::serverCycle));
}

void Server::process_client(SOCKET socket)
{
    std::string msg = "";
    char tempmsg[max_buffer_length_] = "";
    memset(tempmsg, 0, max_buffer_length_);

    unsigned long l;
    ioctlsocket(socket, FIONREAD, &l);
    if (l > 0){
        int iResult = recv(socket, tempmsg, max_buffer_length_, 0);

        if (iResult != SOCKET_ERROR)
        {
            msg = tempmsg;
            qDebug() << "Message from client: " << msg.c_str();

            //send(socket, msg.c_str(), (int)strlen(msg.c_str()), 0);
        }
        else
        {
            disconnectClient(socket);
        }
    } else {
    }
}

void Server::disconnectClient(SOCKET socket){
    std::string msg = "";
    msg = "Client Disconnected";

    qDebug() << msg.c_str();
    //std::cout << msg << std::endl;

    closesocket(socket);
    client_socket_ = INVALID_SOCKET;
}

void Server::serverCycle(){
    std::string msg = "";
    SOCKET incoming = INVALID_SOCKET;
    incoming = accept(server_socket_, NULL, NULL);

    if (incoming != INVALID_SOCKET){ //new client trying to connect
        if (client_socket_ == INVALID_SOCKET)
        {
            client_socket_ = incoming;
            clientThread_ = std::thread(&Server::process_client, this, client_socket_);
        } else {
            qDebug() << "Client already connected. Only 1 client allowed at a time";
        }
    }

    emit serverCycleComplete();
}

bool Server::testConnected(SOCKET socket){
    std::string testStr = ".";
    int iResult = send(socket, testStr.c_str(), (int)strlen(testStr.c_str()), 0);
    if (iResult == SOCKET_ERROR)
    {
        return false;
    }
    return true;
}

void Server::stopServer(){

    disconnect(this, SIGNAL(serverCycleComplete()), this, SLOT(serverCycleThreaded()));

    //Close listening socket
    closesocket(server_socket_);

    if (testConnected(client_socket_)){
        closesocket(client_socket_);
        if (clientThread_.joinable()){
            clientThread_.detach();
        }
        //disconnectClient(client_socket_);
    }

    //Clean up Winsock
    WSACleanup();
}

Server::~Server(){
    emit finished();
}

Client::Client(std::string ip, std::string port, QObject *parent) : QObject(parent), ip_(ip), port_(port)
{
    clientThread_ = std::thread();
    qfuture_clientSendThreaded = nullptr;
}

void Client::assignThread(QThread *thread){
    qDebug() << "Moving stereo streamer client to thread";
    thread_ = thread;
    this->moveToThread(thread_);
    connect(this, SIGNAL(finished()), thread_, SLOT(quit()));
    //connect(this, SIGNAL(finished()), this, SLOT(deleteLater()));
    connect(thread_, SIGNAL(finished()), thread_, SLOT(deleteLater()));
    thread_->start();
    thread_->setPriority(QThread::LowestPriority);
}

bool Client::startClient(){
    WSAData wsa_data;
    struct addrinfo *result = NULL, *ptr = NULL, hints;
    std::string sent_message = "";
    SOCKET mSocket = INVALID_SOCKET;
    int iResult = 0;
    std::string message;

    qDebug() << "Starting Client...";
    //std::cout << "Starting Client...\n";

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    if (iResult != 0) {
        qDebug() << "WSAStartup() failed with error: " << iResult;
        //std::cout << "WSAStartup() failed with error: " << iResult << std::endl;
        return false;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    qDebug() <<"Connecting..";
    //std::cout << "Connecting...\n";

    // Resolve the server address and port
    iResult = getaddrinfo(ip_.c_str(), port_.c_str(), &hints, &result);
    if (iResult != 0) {
        qDebug() << "getaddrinfo() failed with error: " << iResult;
        //std::cout << "getaddrinfo() failed with error: " << iResult << std::endl;
        return false;
    }

    // Attempt to connect to an address until one succeeds
    for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

        // Create a SOCKET for connecting to server
        mSocket = socket(ptr->ai_family, ptr->ai_socktype,
            ptr->ai_protocol);
        if (mSocket == INVALID_SOCKET) {
            qDebug() << "socket() failed with error: " << WSAGetLastError();
            //std::cout << "socket() failed with error: " << WSAGetLastError() << std::endl;
            return false;
        }

        // Connect to server.
        iResult = ::connect(mSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(mSocket);
            mSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (mSocket == INVALID_SOCKET) {
        qDebug() << "Unable to connect to server!";
        //std::cout << "Unable to connect to server!" << std::endl;
        return false;
    }

    qDebug() << "Successfully Connected";
    //std::cout << "Successfully Connected" << std::endl;

    connect(this, SIGNAL(clientMessage(QString)), this, SLOT(messageReceived(QString)));
    clientThread_ = std::thread(&Client::process_client, this, client_socket_);
    isClientRunning = true;
    return true;
}

void Client::messageReceived(QString server_msg){
    qDebug() << "Message received: " << server_msg;
}

bool Client::clientSendUCharImageThreaded(cv::Mat image){
    if (qfuture_clientSendThreaded != nullptr){
        if (qfuture_clientSendThreaded->isFinished()){
            cv::Mat send_image = image.clone();
            qfuture_clientSendThreaded = new QFuture<bool>(QtConcurrent::run(this, &Client::clientSendUCharImage, send_image));
            return true;
        } else {
            //qDebug() << "client thread is busy";
            return false;
        }
    } else {
        cv::Mat send_image = image.clone();
        qfuture_clientSendThreaded = new QFuture<bool>(QtConcurrent::run(this, &Client::clientSendUCharImage, send_image));
        return true;
    }
}

bool Client::clientSendFloatImageThreaded(cv::Mat image){
    if (qfuture_clientSendThreaded != nullptr){
        if (qfuture_clientSendThreaded->isFinished()){
            cv::Mat send_image = image.clone();
            qfuture_clientSendThreaded = new QFuture<bool>(QtConcurrent::run(this, &Client::clientSendFloatImage, send_image));
            return true;
        } else {
            //qDebug() << "client thread is busy";
            return false;
        }
    } else {
        cv::Mat send_image = image.clone();
        qfuture_clientSendThreaded = new QFuture<bool>(QtConcurrent::run(this, &Client::clientSendFloatImage, send_image));
        return true;
    }
}

bool Client::testConnected(){
    std::string testStr = ".";
    int iResult = send(client_socket_, testStr.c_str(), (int)strlen(testStr.c_str()), 0);
    if (iResult == SOCKET_ERROR)
    {
        return false;
    }
    return true;
}

bool Client::clientSendStringMessage(std::string message){
    return sendMessage(client_socket_,message);
}

bool Client::clientSendUCharImage(cv::Mat image){
    std::string message = Image2String::ucharMat2str(image,100);
    return sendMessage(client_socket_,message);
}

bool Client::clientSendFloatImage(cv::Mat image){
    std::string message = Image2String::floatMat2str(image);
    return sendMessage(client_socket_,message);
}

void Client::process_client(SOCKET socket){

    char received_message[max_buffer_length_];
    memset(received_message, 0, max_buffer_length_);

    unsigned long l;
    ioctlsocket(socket, FIONREAD, &l);
    if (l > 0){
        int iResult = recv(socket, received_message, max_buffer_length_, 0);

        if (iResult != SOCKET_ERROR){
            std::string msg = received_message;
            collectivemsgs += msg;
            if (!collectivemsgs.empty() && msg[msg.length()-1] == '\n') {
                // remove new line character from end of string
                collectivemsgs.erase(collectivemsgs.length()-1);
                // check string is not empty
                if (!collectivemsgs.empty()){
                    emit clientMessage(QString(collectivemsgs.c_str()));
                    collectivemsgs = "";
                } else {
                    std::cout << "Message is empty after removing newline" << std::endl;
                }
            } else if (collectivemsgs.size() >= collectivemsgs.max_size()){
                // reset string if exeedingly large
                collectivemsgs = "";
            } else {
                //std::cout << "Message segment: " << msg << std::endl;
            }
        }
        else
        {
            std::cout << "recv() failed: " << WSAGetLastError() << std::endl;
        }
    }


    //if (WSAGetLastError() == WSAECONNRESET)
    //    std::cout << "The server has disconnected" << std::endl;
}

bool Client::stopClient(){
    isClientRunning = false;
    disconnect(this, SIGNAL(clientMessage(QString)), this, SLOT(messageReceived(QString)));
    qDebug() << "Shutting down socket...";
    int iResult = shutdown(client_socket_, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        qDebug() << "shutdown() failed with error: " << WSAGetLastError();
        closesocket(client_socket_);
    }
    closesocket(client_socket_);
    client_socket_ = INVALID_SOCKET;
    return true;
}

Client::~Client(){
    emit finished();
}

}
