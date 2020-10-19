#include "stereostreamer.h"

namespace StereoStreamer {

StereoStreamer::ServerMsg translateFromServerMessage(std::string msg){
    // convert from server message format
    // comma seperated string
    // CLIENT_ID,MSG_TYPE,MSG
    StereoStreamer::ServerMsg serverMsg;
    std::vector<std::string> msg_split;
    std::stringstream s_stream(msg); //create string stream from the string
    while(s_stream.good()) {
        std::string substr;
        getline(s_stream, substr, ','); //get first string delimited by comma
        msg_split.push_back(substr);
    }
    if (msg_split.size() == 3){
        int msg_type_index;
        try {
            serverMsg.clientId = std::stoi(msg_split[0]);
        }
        catch(std::invalid_argument& e){
            // if no conversion could be performed
            serverMsg.clientId = -1;
            return serverMsg;
        }
        catch(std::out_of_range& e){
            // if the converted value would fall out of the range of the result type
            // or if the underlying function (std::strtol or std::strtoull) sets errno
            // to ERANGE.
            serverMsg.clientId = -1;
            return serverMsg;
        }
        try {
            msg_type_index = (StereoStreamer::MsgType)std::stoi(msg_split[1]);
        }
        catch(std::invalid_argument& e){
            // if no conversion could be performed
            serverMsg.msgType = MSG_TYPE_ERROR;
            return serverMsg;
        }
        catch(std::out_of_range& e){
            // if the converted value would fall out of the range of the result type
            // or if the underlying function (std::strtol or std::strtoull) sets errno
            // to ERANGE.
            serverMsg.msgType = MSG_TYPE_ERROR;
            return serverMsg;
        }
        serverMsg.msg = msg_split[2];
    }
    return serverMsg;
}

std::string translateToServerMessage(StereoStreamer::ServerMsg serverMsg){
    // convert to server message format
    // comma seperated string
    // CLIENT_ID,MSG_TYPE,MSG
    std::string msg;
    msg += std::to_string(serverMsg.clientId);
    msg += ",";
    msg += std::to_string(serverMsg.msgType);
    msg += ",";
    msg += serverMsg.msg;
    return msg;
}

Server::Server(std::string ip, std::string port, QObject *parent) : QObject(parent), ip_(ip), port_(port)
{
    clientInfos_ = std::vector<ClientInfo>(max_clients_);
    for (int i = 0; i < max_clients_; i++)
    {
        clientThreads_[i] = std::thread();
    }
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

    //Initialize the client list
    for (int i = 0; i < max_clients_; i++)
    {
        // initalise clients with msg type of string
        clientInfos_[i] = { -1, INVALID_SOCKET, std::chrono::steady_clock::now()};
    }

    connect(this, SIGNAL(serverCycleComplete()), this, SLOT(serverCycleThreaded()));
    //serverCycle(); //do one non threaded cycle to inialise the server
    serverCycleThreaded();
}

void Server::serverCycleThreaded(){
    qfuture_serverCycle = new QFuture<void>(QtConcurrent::run(this, &Server::serverCycle));
}

int Server::process_client(ClientInfo &new_client, std::vector<ClientInfo> &client_array, std::thread &thread)
{
    std::string msg = "";
    char tempmsg[max_buffer_length_] = "";

    client_array[new_client.id].lastValidMsgTime = std::chrono::steady_clock::now();

    //Session
    while (1)
    {
        memset(tempmsg, 0, max_buffer_length_);

        unsigned long l;
        ioctlsocket(new_client.socket, FIONREAD, &l);
        if (l > 0){
            int iResult = recv(new_client.socket, tempmsg, max_buffer_length_, 0);

            //std::string endlch = "\n";

            if (iResult != SOCKET_ERROR)
            {
                client_array[new_client.id].lastValidMsgTime = std::chrono::steady_clock::now();
                //if (strcmp("", tempmsg))
                    //msg = "Client #" + std::to_string(new_client.id) + ": " + tempmsg;
                    //msg = tempmsg + endlch;
                    //msg = tempmsg;
                msg = tempmsg;

                //qDebug() << msg.c_str();
                //std::cout << msg.c_str() << std::endl;

                //Broadcast that message to the other clients
                for (int i = 0; i < max_clients_; i++)
                {
                    if (client_array[i].socket != INVALID_SOCKET){
                        if (new_client.id != i){
                            send(client_array[i].socket, msg.c_str(), (int)strlen(msg.c_str()), 0);
                            //TODO check result of send to make sure it was successful
                            //iResult = send(client_array[i].socket, msg.c_str(), (int)strlen(msg.c_str()), 0);
                        }
                    }
                }
            }
            else
            {
                clientDisconnected(new_client,client_array);
                break;
            }
        } else {
            // get time idle and disconnect after x time
            std::chrono::steady_clock::time_point time = std::chrono::steady_clock::now();
            int dur_us = std::chrono::duration_cast<std::chrono::microseconds>(time - client_array[new_client.id].lastValidMsgTime).count();
            int dur_ms = dur_us/1000;
            if (dur_ms > client_timeout_){
                std::cerr << "Will disconnect client # " << new_client.id << " due to inactivity" << std::endl;
                clientDisconnected(new_client,client_array);
                break;
            }
        }
    }

    thread.detach();

    return 0;
}

void Server::clientDisconnected(ClientInfo &new_client, std::vector<ClientInfo> &client_array){
    std::string msg = "";
    msg = "Client # " + std::to_string(new_client.id) + " Disconnected";

    qDebug() << msg.c_str();
    //std::cout << msg << std::endl;

    closesocket(new_client.socket);
    closesocket(client_array[new_client.id].socket);
    client_array[new_client.id].socket = INVALID_SOCKET;

    //Broadcast the disconnection message to the other clients
    for (int i = 0; i < max_clients_; i++)
    {
        if (client_array[i].socket != INVALID_SOCKET){
            send(client_array[i].socket, msg.c_str(), (int)strlen(msg.c_str()), 0);
            //TODO check result of send to make sure it was successful
            //iResult = send(client_array[i].socket, msg.c_str(), (int)strlen(msg.c_str()), 0);
        }
    }
}

void Server::serverCycle(){
    std::string msg = "";
    SOCKET incoming = INVALID_SOCKET;
    incoming = accept(server_socket_, NULL, NULL);

    if (incoming != INVALID_SOCKET){
        //Reset the number of clients
        num_clients_ = -1;

        //Create a temporary id for the next client
        int temp_id_ = -1;
        for (int i = 0; i < max_clients_; i++)
        {
            if (clientInfos_[i].socket == INVALID_SOCKET && temp_id_ == -1)
            {
                clientInfos_[i].socket = incoming;
                clientInfos_[i].id = i;
                temp_id_ = i;
            }

            if (clientInfos_[i].socket != INVALID_SOCKET)
                num_clients_++;

            //std::cout << client[i].socket << std::endl;
        }

        if (temp_id_ != -1)
        {
            //Send the id to that client
            qDebug() << "Client #" << clientInfos_[temp_id_].id << "Accepted";
            //std::cout << "Client #" << serverClient_[temp_id_].id << " Accepted" << std::endl;
            msg = std::to_string(clientInfos_[temp_id_].id);
            send(clientInfos_[temp_id_].socket, msg.c_str(), (int)strlen(msg.c_str()), 0);

            //Create a thread process for that client
            clientThreads_[temp_id_] = std::thread(&Server::process_client, this, std::ref(clientInfos_[temp_id_]), std::ref(clientInfos_), std::ref(clientThreads_[temp_id_]));
        }
        else
        {
            msg = "Server is full";
            send(incoming, msg.c_str(), (int)strlen(msg.c_str()), 0);
            qDebug() << msg.c_str();
            //std::cout << msg << std::endl;
        }
    }

    emit serverCycleComplete();
}

void Server::stopServer(){

    disconnect(this, SIGNAL(serverCycleComplete()), this, SLOT(serverCycleThreaded()));

    //Close listening socket
    closesocket(server_socket_);

    //Close client socket
    for (int i = 0; i < max_clients_; i++)
    {
        closesocket(clientInfos_[i].socket);
    }

    //Clean up Winsock
    WSACleanup();
}

Server::~Server(){
    emit finished();
}

Client::Client(std::string ip, std::string port,bool enable_reader, QObject *parent) : QObject(parent), ip_(ip), port_(port)
{
    qRegisterMetaType<StereoStreamer::ServerMsg>();
    clientThread_ = std::thread();
    qfuture_clientSendThreaded = nullptr;
    isReader_ = enable_reader;
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
    ClientInfo client = { -1, INVALID_SOCKET, std::chrono::steady_clock::now()};
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
        client.socket = socket(ptr->ai_family, ptr->ai_socktype,
            ptr->ai_protocol);
        if (client.socket == INVALID_SOCKET) {
            qDebug() << "socket() failed with error: " << WSAGetLastError();
            //std::cout << "socket() failed with error: " << WSAGetLastError() << std::endl;
            return false;
        }

        // Connect to server.
        iResult = ::connect(client.socket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(client.socket);
            client.socket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (client.socket == INVALID_SOCKET) {
        qDebug() << "Unable to connect to server!";
        //std::cout << "Unable to connect to server!" << std::endl;
        return false;
    }

    qDebug() << "Successfully Connected";
    //std::cout << "Successfully Connected" << std::endl;

    //Obtain id from server for this client;
    char received_message[max_buffer_length_];
    recv(client.socket, received_message, max_buffer_length_, 0);
    message = received_message;

    if (message != "Server is full")
    {
        qDebug() << "Connected with ID:" << received_message;
        client.id = atoi(received_message);
        clientInfo_ = client;
        if (isReader_){
            connect(this, SIGNAL(clientMessage(StereoStreamer::ServerMsg)), this, SLOT(messageReceived(StereoStreamer::ServerMsg)));
            clientThread_ = std::thread(&Client::process_client, this, std::ref(clientInfo_));
        }
        isClientRunning = true;
        return true;
    } else {
        qDebug() << "Server is full";
        return false;
    }
}

void Client::enableReader(bool enable){
    if (isReader_ != enable){
        isReader_ = enable;
        if (isClientRunning){
            if (enable){
                connect(this, SIGNAL(clientMessage(StereoStreamer::ServerMsg)), this, SLOT(messageReceived(StereoStreamer::ServerMsg)));
                clientThread_ = std::thread(&Client::process_client, this, std::ref(clientInfo_));
            } else {
                disconnect(this, SIGNAL(clientMessage(StereoStreamer::ServerMsg)), this, SLOT(messageReceived(StereoStreamer::ServerMsg)));
                if (clientThread_.joinable()){
                    clientThread_.detach();
                }
            }
        }
    }
};

void Client::messageReceived(StereoStreamer::ServerMsg server_msg){
    //std::cout << "Message from # " << msg.clientId <<  " : " << msg.msg << std::endl;
    std::string msg = server_msg.msg;
    int sender_id = server_msg.clientId;
    std::cout << "Message received of size: " << msg.size() << std::endl;
    switch(server_msg.msgType){
    case MSG_TYPE_STRING:
        std::cout << "Message from # " << sender_id <<  " : " << msg << std::endl;
        break;
    case MSG_TYPE_IMAGE:
        cv::Mat image = Image2String::str2mat(msg);
        cv::resize(image, image, cv::Size(), 0.25, 0.25);
        cv::Mat display_image;
        if (image.type() == CV_32FC1){
            display_image = image;
            //CVSupport::disparity2colormap(image,display_image);
        } else {
            display_image = image;
        }
        /*
        std::chrono::steady_clock::time_point time = std::chrono::steady_clock::now();
        int dur_ms = std::chrono::duration_cast<std::chrono::microseconds>(time - prevtime).count();
        float dur_s = dur_ms/1000000.0;
        std::cout << "Time difference: " << dur_ms << "[us]" << std::endl;
        float fps = 1.0/dur_s;
        std::cout << "FPS: " << fps << std::endl;
        */
        std::string window_name = "Client #" + std::to_string(clientInfo_.id);
        cv::imshow(window_name, display_image);
        cv::waitKey(1);
        break;
    }
}

bool Client::clientSendUCharImageThreaded(cv::Mat image){
    if (qfuture_clientSendThreaded != nullptr){
        if (qfuture_clientSendThreaded->isFinished()){
            cv::Mat send_image = image.clone();
            qfuture_clientSendThreaded = new QFuture<bool>(QtConcurrent::run(this, &Client::clientSendUCharImage, send_image));
            return true;
        } else {
            qDebug() << "client thread is busy";
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
            qDebug() << "client thread is busy";
            return false;
        }
    } else {
        cv::Mat send_image = image.clone();
        qfuture_clientSendThreaded = new QFuture<bool>(QtConcurrent::run(this, &Client::clientSendFloatImage, send_image));
        return true;
    }
}

bool Client::isConnected(){
    std::string testStr = ".";
    int iResult = send(clientInfo_.socket, testStr.c_str(), (int)strlen(testStr.c_str()), 0);
    if (iResult == SOCKET_ERROR)
    {
        return false;
    }
    return true;
}

bool Client::clientSendMessagePacket(std::string message){
    if (message.size() > max_buffer_length_){
        qDebug() << "Unable to send message as it will overflow the stream buffer";
        return false;
    } else {
        int iResult = send(clientInfo_.socket, message.c_str(), (int)strlen(message.c_str()), 0);
        if (iResult <= 0)
        {
            int error = WSAGetLastError();
            qDebug() << "send() failed: " << error;
            return false;
        } else {
            return true;
        }
    }
}

bool Client::clientSendMessage(ServerMsg message){
    std::string msg = StereoStreamer::translateToServerMessage(message);
    // add end of message token to end of string
    msg+= eom_token_;
    size_t total_size = msg.size();
    if (total_size < max_buffer_length_){
        return clientSendMessagePacket(msg);
    } else {
        bool success = true;
        //split message into packets
        float num_of_packets_f = (float)total_size / (float)max_buffer_length_;
        size_t num_of_packets_i = ceil(num_of_packets_f);
        for (size_t i = 0; i < num_of_packets_i; i++){
            size_t start_index = i*max_buffer_length_;
            size_t packet_length = max_buffer_length_;
            if ((start_index + max_buffer_length_) > msg.size()){
                packet_length = msg.size() - start_index;
            }
            std::string msg_packet = msg.substr(start_index, packet_length);
            success &= clientSendMessagePacket(msg_packet);
        }
        return success;
    }
}

bool Client::clientSendStringMessage(std::string message){
    //TODO check message does not contain OEM as this will cause issues
    ServerMsg srvMsg {clientInfo_.id, MSG_TYPE_STRING, message};
    return clientSendMessage(srvMsg);
}

bool Client::clientSendUCharImage(cv::Mat image){
    std::string message = Image2String::ucharMat2str(image,100);
    qDebug() << "Sending image message of size: " << message.size();
    ServerMsg srvMsg {clientInfo_.id, MSG_TYPE_IMAGE, message};
    return clientSendMessage(srvMsg);
}

bool Client::clientSendFloatImage(cv::Mat image){
    std::string message = Image2String::floatMat2str(image);
    qDebug() << "Sending image message of size: " << message.size();
    ServerMsg srvMsg {clientInfo_.id, MSG_TYPE_IMAGE, message};
    return clientSendMessage(srvMsg);
}

int Client::process_client(ClientInfo &new_client){
    char received_message[max_buffer_length_];
    while (1)
    {
        memset(received_message, 0, max_buffer_length_);

        unsigned long l;
        ioctlsocket(new_client.socket, FIONREAD, &l);
        if (l > 0){
            int iResult = recv(new_client.socket, received_message, max_buffer_length_, 0);

            if (iResult != SOCKET_ERROR){
                std::string msg = received_message;
                collectivemsgs[new_client.id] += msg;
                if (!collectivemsgs[new_client.id].empty() && msg[msg.length()-1] == '\n') {
                    // remove new line character from end of string
                    collectivemsgs[new_client.id].erase(collectivemsgs[new_client.id].length()-1);
                    // check string is not empty
                    if (!collectivemsgs[new_client.id].empty()){
                        StereoStreamer::ServerMsg serverMsg = StereoStreamer::translateFromServerMessage(collectivemsgs[new_client.id]);
                        if (serverMsg.msgType == MSG_TYPE_ERROR || serverMsg.clientId == -1){
                            std::cerr << "Invalid server message" << std::endl;
                        } else {
                            //std::cout << "Message from # " << serverMsg.clientId <<  " : " << serverMsg.msg << std::endl;
                            emit clientMessage(serverMsg);
                        }
                        collectivemsgs[new_client.id] = "";
                    } else {
                        std::cout << "Message is empty after removing newline" << std::endl;
                    }
                } else if (collectivemsgs[new_client.id].size() >= collectivemsgs[new_client.id].max_size()){
                    // reset string if exeedingly large
                    collectivemsgs[new_client.id] = "";
                } else {
                    //std::cout << "Message segment: " << msg << std::endl;
                }
            }
            else
            {
                std::cout << "recv() failed: " << WSAGetLastError() << std::endl;
                break;
            }
        }
    }

    if (WSAGetLastError() == WSAECONNRESET)
        std::cout << "The server has disconnected" << std::endl;

    return 0;
}

bool Client::stopClient(){
    isClientRunning = false;
    disconnect(this, SIGNAL(clientMessage(StereoStreamer::ServerMsg)), this, SLOT(messageReceived(StereoStreamer::ServerMsg)));
    qDebug() << "Shutting down socket...";
    int iResult = shutdown(clientInfo_.socket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        qDebug() << "shutdown() failed with error: " << WSAGetLastError();
        closesocket(clientInfo_.socket);
    }
    closesocket(clientInfo_.socket);
    clientInfo_ = {-1, INVALID_SOCKET};
    return true;
}

Client::~Client(){
    emit finished();
}

}
