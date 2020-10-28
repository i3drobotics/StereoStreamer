#include "stereostreamer2.h"

namespace StereoStreamer2 {

ServerMsg::ServerMsg(){
    this->senderClientId = INVALID_CLIENT_ID;
    this->targetClientId = INVALID_CLIENT_ID;
    this->msgType = MSG_TYPE_NOT_SET;
    this->msgEndIndex = 1;
    this->msgIndex = 0;
    this->msg = "";
}

ServerMsg::ServerMsg(int senderClientId, int targetClientId, MsgType msgType, std::string msg){
    this->senderClientId = senderClientId;
    this->targetClientId = targetClientId;
    this->msgType = msgType;
    this->msgEndIndex = 1;
    this->msgIndex = 0;
    this->msg = msg;
}

ServerMsg::ServerMsg(int senderClientId, int targetClientId, MsgType msgType, std::string msg, int msgEndIndex, int msgIndex){
    this->senderClientId = senderClientId;
    this->targetClientId = targetClientId;
    this->msgType = msgType;
    this->msgEndIndex = msgEndIndex;
    this->msgIndex = msgIndex;
    this->msg = msg;
}

ServerMsg::ServerMsg(std::string raw_server_msg){
    // convert from server message format
    // comma seperated string
    // SENDER_CLIENT_ID,TARGET_CLIENT_ID,MSG_TYPE,MSG_END_INDEX,MSG_INDEX,MSG
    if (raw_server_msg[raw_server_msg.size()-1] == '\n'){
        raw_server_msg = raw_server_msg.substr(0,raw_server_msg.size()-1);
        std::vector<std::string> msg_split;
        std::stringstream s_stream(raw_server_msg); //create string stream from the string
        while(s_stream.good()) {
            std::string substr;
            getline(s_stream, substr, ','); //get first string delimited by comma
            msg_split.push_back(substr);
        }
        if (msg_split.size() == 6){
            try {
                this->senderClientId = std::stoi(msg_split[0]);
            }
            catch(std::invalid_argument& e){
                // if no conversion could be performed
                this->senderClientId = INVALID_CLIENT_ID;
            }
            catch(std::out_of_range& e){
                // if the converted value would fall out of the range of the result type
                // or if the underlying function (std::strtol or std::strtoull) sets errno
                // to ERANGE.
                this->senderClientId = INVALID_CLIENT_ID;
            }
            try {
                this->targetClientId = std::stoi(msg_split[1]);
            }
            catch(std::invalid_argument& e){
                // if no conversion could be performed
                this->targetClientId = INVALID_CLIENT_ID;
            }
            catch(std::out_of_range& e){
                // if the converted value would fall out of the range of the result type
                // or if the underlying function (std::strtol or std::strtoull) sets errno
                // to ERANGE.
                this->targetClientId = INVALID_CLIENT_ID;
            }
            try {
                int msg_type_index = std::stoi(msg_split[2]);
                this->msgType = (StereoStreamer2::MsgType)msg_type_index;
            }
            catch(std::invalid_argument& e){
                // if no conversion could be performed
                this->msgType = MSG_TYPE_ERROR;
            }
            catch(std::out_of_range& e){
                // if the converted value would fall out of the range of the result type
                // or if the underlying function (std::strtol or std::strtoull) sets errno
                // to ERANGE.
                this->msgType = MSG_TYPE_ERROR;
            }
            try {
                int msg_index = std::stoi(msg_split[3]);
                this->msgEndIndex = msg_index;
            }
            catch(std::invalid_argument& e){
                // if no conversion could be performed
                this->msgEndIndex = -1;
            }
            catch(std::out_of_range& e){
                // if the converted value would fall out of the range of the result type
                // or if the underlying function (std::strtol or std::strtoull) sets errno
                // to ERANGE.
                this->msgEndIndex = -1;
            }
            try {
                int msg_index = std::stoi(msg_split[4]);
                this->msgIndex = msg_index;
            }
            catch(std::invalid_argument& e){
                // if no conversion could be performed
                this->msgIndex = -1;
            }
            catch(std::out_of_range& e){
                // if the converted value would fall out of the range of the result type
                // or if the underlying function (std::strtol or std::strtoull) sets errno
                // to ERANGE.
                this->msgIndex = -1;
            }
            this->msg = msg_split[5];
            if (pad_packets_){
                this->msg.erase(std::find(this->msg.begin(), this->msg.end(), padding_char_), this->msg.end());
            }
        } else {
            qDebug() << "Currupt message received. Expects 6 comma delimited string.";
            invalidMessage();
        }
    } else {
        qDebug() << "Currupt message received. Expects newline at end of string.";
        invalidMessage();
    }
}

void ServerMsg::invalidMessage(){
    this->senderClientId = INVALID_CLIENT_ID;
    this->targetClientId = INVALID_CLIENT_ID;
    this->msgType = MSG_TYPE_ERROR;
    this->msgEndIndex = -1;
    this->msgIndex = -1;
    this->msg = "";
}

std::string ServerMsg::get_raw_server_message(){
    // convert to server message format
    // comma seperated string
    // SENDER_CLIENT_ID,TARGET_CLIENT_ID,MSG_TYPE,MSG_END_INDEX,MSG_INDEX,MSG
    std::string raw_msg;
    raw_msg += std::to_string(this->senderClientId);
    raw_msg += ",";
    raw_msg += std::to_string(this->targetClientId);
    raw_msg += ",";
    raw_msg += std::to_string(this->msgType);
    raw_msg += ",";
    raw_msg += std::to_string(this->msgEndIndex);
    raw_msg += ",";
    raw_msg += std::to_string(this->msgIndex);
    raw_msg += ",";
    raw_msg += this->msg;
    raw_msg += eom_token_;
    return raw_msg;
}

std::vector<ServerMsg> ServerMsg::splitPackets(size_t max_buffer_length, int header_space){
    std::string srvMsg_msg = this->msg;
    std::vector<std::string> srvMsg_msg_packets;
    std::vector<ServerMsg> srvMsg_packets;
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
    int end_index = srvMsg_msg_packets.size();
    for (std::vector<std::string>::iterator it = srvMsg_msg_packets.begin() ; it != srvMsg_msg_packets.end(); ++it){
        int index = it - srvMsg_msg_packets.begin();
        std::string srv_msg = *it;
        ServerMsg newSrvMsg(this->senderClientId,this->targetClientId,this->msgType,srv_msg,end_index,index);
        std::string raw_msg = newSrvMsg.get_raw_server_message();
        if (raw_msg.size() < max_buffer_length){
            srvMsg_packets.push_back(newSrvMsg);
        } else {
            qDebug() << "Invalid packet size:" << raw_msg.size() << "THIS SHOULD NEVER HAPPEN!";
        }
    }
    return srvMsg_packets;
}

bool sendMessagePacket(SOCKET socket, std::string message){
    if (message.size() > max_buffer_length_){
        qDebug() << "Unable to send message as it will overflow the stream buffer";
        return false;
    } else {
        std::string send_message = message;
        if (pad_packets_){
            // always send same size packet
            if (message.size() < max_buffer_length_){
                std::string padded_message = send_message.append(std::string( (max_buffer_length_ - message.size()+1 ), padding_char_));
                send_message = padded_message;
            }
        }
        //send_message += eom_token_;
        if (socket != INVALID_SOCKET){
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

bool sendServerMessage(SOCKET socket, ServerMsg message){
    std::string msg = message.get_raw_server_message();
    // add end of message token to end of string
    //msg += eom_token_;
    size_t total_size = msg.size();
    if (total_size < max_buffer_length_){
        return sendMessagePacket(socket,msg);
    } else {
        bool success = true;
        std::vector<ServerMsg> srvMsg_packets = message.splitPackets(max_buffer_length_,header_buffer_);
        for (std::vector<ServerMsg>::iterator it = srvMsg_packets.begin() ; it != srvMsg_packets.end(); ++it){
            std::string raw_msg = it->get_raw_server_message();
            //raw_msg += eom_token_; // add end of message token to end of string
            qDebug() << "Packet size:" << raw_msg.size()+1;
            success &= sendMessagePacket(socket,raw_msg);
            if (!success) break;
            Sleep(message_sleep_time_); // sleep between sends
        }
        return success;
    }
}

Server::Server(std::string ip, std::string port, QObject *parent) : QObject(parent), ip_(ip), port_(port)
{
    clientInfos_ = std::vector<ClientInfo>(max_clients_);
    for (int i = 0; i < max_clients_; i++)
    {
        clientThreads_[i] = std::thread();
    }
    qfuture_sendThreaded = nullptr;
}

void Server::assignThread(QThread *thread){
    qDebug() << "Moving stereo streamer server to thread";
    thread_ = thread;
    this->moveToThread(thread_);
    connect(this, SIGNAL(finished()), thread_, SLOT(quit()));
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
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    //Setup hints
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    //Setup Server
    qDebug() << "Setting up server on" << ip_.c_str() << ":" << port_.c_str();
    getaddrinfo(ip_.c_str(), port_.c_str(), &hints, &server);

    //Create a listening socket for connecting to server
    qDebug() << "Creating server socket...";
    server_socket_ = socket(server->ai_family, server->ai_socktype, server->ai_protocol);

    //Setup socket options
    setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, &option_value_, sizeof(int)); //Make it possible to re-bind to a port that was used within the last 2 minutes
    setsockopt(server_socket_, IPPROTO_TCP, TCP_NODELAY, &option_value_, sizeof(int)); //Used for interactive programs

    //Assign an address to the server socket.
    qDebug() << "Binding socket...";
    bind(server_socket_, server->ai_addr, (int)server->ai_addrlen);

    //Listen for incoming connections.
    qDebug() << "Listening...";
    listen(server_socket_, SOMAXCONN);

    //Initialize the client list
    for (int i = 0; i < max_clients_; i++)
    {
        // initalise clients with msg type of string
        clientInfos_[i] = ClientInfo();
    }

    connect(this, SIGNAL(serverCycleComplete()), this, SLOT(serverCycleThreaded()));
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

            if (iResult != SOCKET_ERROR)
            {
                client_array[new_client.id].lastValidMsgTime = std::chrono::steady_clock::now();
                msg = tempmsg;

                //qDebug() << "Message from client #" << new_client.id << msg.c_str();

                /*

                //qDebug() << msg.c_str();

                //get target client from message
                ServerMsg srvMsg(msg);
                if (srvMsg.targetClientId != INVALID_CLIENT_ID){
                    if (srvMsg.targetClientId == SEND_TO_ALL_CLIENTS){
                        //Broadcast that message to all the other clients
                        for (int i = 0; i < max_clients_; i++)
                        {
                            if (client_array[i].socket != INVALID_SOCKET){
                                if (new_client.id != i){
                                    iResult = send(client_array[i].socket, msg.c_str(), (int)strlen(msg.c_str()), 0);

                                    //if (iResult != SOCKET_ERROR)
                                    //{
                                    //    qDebug() << "Closing client as unable to send data to socket";
                                    //    clientDisconnected(client_array[i],client_array);
                                    //}

                                }
                            }
                        }
                    } else if (srvMsg.targetClientId == SERVER_CLIENT_ID){

                    } else {
                        //Broadcast message to specific client
                        if (client_array[srvMsg.targetClientId].socket != INVALID_SOCKET){
                            send(client_array[srvMsg.targetClientId].socket, msg.c_str(), (int)strlen(msg.c_str()), 0);
                        }
                    }

                } else {
                    qDebug () << "Invalid target client id";
                }
                */
            }
            else
            {
                clientDisconnected(new_client,client_array);
                break;
            }
        } else {
            /*
            // get time idle and disconnect after x time
            std::chrono::steady_clock::time_point time = std::chrono::steady_clock::now();
            int dur_us = std::chrono::duration_cast<std::chrono::microseconds>(time - client_array[new_client.id].lastValidMsgTime).count();
            int dur_ms = dur_us/1000;
            if (dur_ms > client_timeout_){
                std::cerr << "Will disconnect client # " << new_client.id << " due to inactivity" << std::endl;
                clientDisconnected(new_client,client_array);
                break;
            }
            */
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
    /*
    for (int i = 0; i < max_clients_; i++)
    {
        if (client_array[i].socket != INVALID_SOCKET){
            send(client_array[i].socket, msg.c_str(), (int)strlen(msg.c_str()), 0);
            //TODO check result of send to make sure it was successful
            //iResult = send(client_array[i].socket, msg.c_str(), (int)strlen(msg.c_str()), 0);
        }
    }
    */
}

void Server::serverCycle(){
    std::string msg = "";
    SOCKET incoming = INVALID_SOCKET;
    incoming = accept(server_socket_, NULL, NULL);

    if (incoming != INVALID_SOCKET){
        //Reset the number of clients
        num_clients_ = -1;

        //Create a temporary id for the next client
        int temp_id_ = INVALID_CLIENT_ID;
        for (int i = 0; i < max_clients_; i++)
        {
            if (clientInfos_[i].socket == INVALID_SOCKET && temp_id_ == INVALID_CLIENT_ID)
            {
                clientInfos_[i].socket = incoming;
                clientInfos_[i].id = i;
                temp_id_ = i;
            }

            if (clientInfos_[i].socket != INVALID_SOCKET)
                num_clients_++;
        }

        if (temp_id_ != INVALID_CLIENT_ID)
        {
            //Send the id to that client
            qDebug() << "Client #" << clientInfos_[temp_id_].id << "Accepted";
            msg = std::to_string(clientInfos_[temp_id_].id);
            sendServerMessage(ServerMsg(SERVER_CLIENT_ID,temp_id_,MSG_TYPE_CLIENT_ID,msg));

            //Create a thread process for that client
            clientThreads_[temp_id_] = std::thread(&Server::process_client, this, std::ref(clientInfos_[temp_id_]), std::ref(clientInfos_), std::ref(clientThreads_[temp_id_]));
        }
        else
        {
            msg = SERVER_FULL;
            sendServerMessage(ServerMsg(SERVER_CLIENT_ID,temp_id_,MSG_TYPE_SERVER_MSG,msg));
            qDebug() << msg.c_str();
        }
    }

    emit serverCycleComplete();
}

bool Server::sendServerMessage(ServerMsg message){
    if (message.targetClientId != INVALID_CLIENT_ID){
        if (message.targetClientId == SEND_TO_ALL_CLIENTS){
            //Broadcast that message to all the clients
            bool success = true;
            for (int i = 0; i < max_clients_; i++)
            {
                if (clientInfos_[message.targetClientId].socket != INVALID_SOCKET){ //only send to connected clients
                    success &= ::StereoStreamer2::sendServerMessage(clientInfos_[message.targetClientId].socket,message);
                }
            }
            return success;
        } else if (message.targetClientId == SERVER_CLIENT_ID){
            //Send message to self
            return ::StereoStreamer2::sendServerMessage(clientInfos_[message.targetClientId].socket,message);
        } else {
            //Broadcast message to specific client
            return ::StereoStreamer2::sendServerMessage(clientInfos_[message.targetClientId].socket,message);
        }
    } else {
        qDebug () << "Invalid target client id";
        return false;
    }
}

bool Server::sendStringMessage(std::string message, int targetClientId){
    //TODO check message does not contain OEM as this will cause issues
    ServerMsg srvMsg(SERVER_CLIENT_ID, targetClientId, MSG_TYPE_STRING, message);
    return sendServerMessage(srvMsg);
}

bool Server::sendImage(cv::Mat image, int targetClientId){
    std::string message;
    if (image.type() == CV_32FC3){
        message = Image2String::mat2str(image, true);
    } else if (image.type() == CV_8UC3 || image.type() == CV_8UC1){
        message = Image2String::mat2str(image,false,100);
    } else {
        qDebug() << "Invalid image type";
        return false;
    }
    qDebug() << "Sending image message of size: " << message.size();
    ServerMsg srvMsg(SERVER_CLIENT_ID, targetClientId, MSG_TYPE_IMAGE, message);
    return sendServerMessage(srvMsg);
}

bool Server::sendImageThreaded(cv::Mat image, int targetClientId){
    bool ready = false;
    if (qfuture_sendThreaded != nullptr){
        if (qfuture_sendThreaded->isFinished()){
            ready = true;
        } else {
            //qDebug() << "image send thread is busy";
        }
    } else {
        ready = true;
    }
    if (ready){
        cv::Mat send_image = image.clone();
        qfuture_sendThreaded = new QFuture<bool>(QtConcurrent::run(this, &Server::sendImage, send_image, targetClientId));
        return true;
    } else {
        return false;
    }
}

bool Server::testConnected(int targetClientId){
    std::string testStr = ".";
    int iResult = send(clientInfos_[targetClientId].socket, testStr.c_str(), (int)strlen(testStr.c_str()), 0);
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

    //Close client socket
    for (int i = 0; i < max_clients_; i++)
    {
        if (testConnected(clientInfos_[i].id)){
            closesocket(clientInfos_[i].socket);
            //clientDisconnected(clientInfos_[i],clientInfos_);
            if (clientThreads_[i].joinable()){
                clientThreads_[i].detach();
            }
        }

    }

    //Clean up Winsock
    WSACleanup();
}

Server::~Server(){
    emit finished();
}

Client::Client(std::string ip, std::string port, QObject *parent) : QObject(parent), ip_(ip), port_(port)
{
    qRegisterMetaType<StereoStreamer2::ServerMsg>();
    clientThread_ = std::thread();
    qfuture_sendThreaded = nullptr;
}

void Client::assignThread(QThread *thread){
    qDebug() << "Moving stereo streamer client to thread";
    thread_ = thread;
    this->moveToThread(thread_);
    connect(this, SIGNAL(finished()), thread_, SLOT(quit()));
    connect(thread_, SIGNAL(finished()), thread_, SLOT(deleteLater()));
    thread_->start();
    thread_->setPriority(QThread::LowestPriority);
}

bool Client::startClient(){
    WSAData wsa_data;
    struct addrinfo *result = NULL, *ptr = NULL, hints;
    std::string sent_message = "";
    ClientInfo client;
    int iResult = 0;
    std::string message;

    qDebug() << "Starting Client...";

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    if (iResult != 0) {
        qDebug() << "WSAStartup() failed with error: " << iResult;
        return false;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    qDebug() <<"Connecting..";

    // Resolve the server address and port
    iResult = getaddrinfo(ip_.c_str(), port_.c_str(), &hints, &result);
    if (iResult != 0) {
        qDebug() << "getaddrinfo() failed with error: " << iResult;
        return false;
    }

    // Attempt to connect to an address until one succeeds
    for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

        // Create a SOCKET for connecting to server
        client.socket = socket(ptr->ai_family, ptr->ai_socktype,
                               ptr->ai_protocol);
        if (client.socket == INVALID_SOCKET) {
            qDebug() << "socket() failed with error: " << WSAGetLastError();
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
        return false;
    }

    qDebug() << "Successfully Connected";

    //Obtain id from server for this client;
    char received_message[max_buffer_length_];
    memset(received_message, 0, max_buffer_length_);
    recv(client.socket, received_message, max_buffer_length_, 0);
    message = received_message;
    ServerMsg srvMsg(message);

    if (srvMsg.msg != SERVER_FULL)
    {
        if (srvMsg.msgType == MSG_TYPE_CLIENT_ID){
            qDebug() << "Connected with ID:" << srvMsg.msg.c_str();
            client.id = atoi(srvMsg.msg.c_str());
            clientInfo_ = client;
            connect(this, SIGNAL(clientMessage(StereoStreamer::ServerMsg)), this, SLOT(messageReceived(StereoStreamer::ServerMsg)));
            clientThread_ = std::thread(&Client::process_client, this, std::ref(clientInfo_));
            isClientRunning = true;
            return true;
        } else {
            qDebug() << "Invalid server response. Expected client id message";
            return false;
        }

    } else {
        qDebug() << "Server is full";
        return false;
    }
}

void Client::messageReceived(StereoStreamer2::ServerMsg server_msg){
    //std::cout << "Message from # " << msg.clientId <<  " : " << msg.msg << std::endl;
    std::string msg = server_msg.msg;
    int sender_id = server_msg.senderClientId;
    std::cout << "Message received of size: " << msg.size() << std::endl;
    switch(server_msg.msgType){
        case MSG_TYPE_ERROR: {
            std::cerr << "Error in message" << std::endl;
            break;
        }

        case MSG_TYPE_STRING: {
            std::cout << "Message from # " << sender_id <<  " : " << msg << std::endl;
            break;
        }

        case MSG_TYPE_IMAGE: {
            cv::Mat image = Image2String::str2mat(msg);
            std::cout << "Image message from # " << sender_id <<  " : type=" << image.type() << ", size=(" << image.size().width << "," << image.size().height << ")" << std::endl;
            /*
            cv::resize(image, image, cv::Size(), 0.25, 0.25);
            cv::Mat display_image;
            if (image.type() == CV_32FC1){
                display_image = image;
                //CVSupport::disparity2colormap(image,display_image);
            } else {
                display_image = image;
            }
            std::string window_name = "Client #" + std::to_string(clientInfo_.id);
            cv::imshow(window_name, display_image);
            cv::waitKey(1);
            */
            break;

        }
        default: {
            break;
        }
    }
}

bool Client::testConnected(){
    std::string testStr = ".";
    int iResult = send(clientInfo_.socket, testStr.c_str(), (int)strlen(testStr.c_str()), 0);
    if (iResult == SOCKET_ERROR)
    {
        return false;
    }
    return true;
}

bool Client::sendServerMessage(ServerMsg message){
    return ::StereoStreamer2::sendServerMessage(clientInfo_.socket,message);
}

bool Client::sendStringMessage(std::string message, int targetClientId){
    //TODO check message does not contain OEM as this will cause issues
    ServerMsg srvMsg(SERVER_CLIENT_ID, targetClientId, MSG_TYPE_STRING, message);
    return sendServerMessage(srvMsg);
}

bool Client::sendImage(cv::Mat image, int targetClientId){
    std::string message = Image2String::mat2str(image,false,100);
    qDebug() << "Sending image message of size: " << message.size();
    ServerMsg srvMsg(SERVER_CLIENT_ID, targetClientId, MSG_TYPE_IMAGE, message);
    return sendServerMessage(srvMsg);
}

bool Client::sendImageThreaded(cv::Mat image, int targetClientId){
    bool ready = false;
    if (qfuture_sendThreaded != nullptr){
        if (qfuture_sendThreaded->isFinished()){
            ready = true;
        } else {
            //qDebug() << "image send thread is busy";
        }
    } else {
        ready = true;
    }
    if (ready){
        cv::Mat send_image = image.clone();
        qfuture_sendThreaded = new QFuture<bool>(QtConcurrent::run(this, &Client::sendImage, send_image, targetClientId));
        return true;
    } else {
        return false;
    }
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
                        StereoStreamer2::ServerMsg serverMsg(collectivemsgs[new_client.id]);
                        if (serverMsg.msgType == MSG_TYPE_ERROR || serverMsg.senderClientId == INVALID_CLIENT_ID){
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
    clientInfo_ = ClientInfo();
    return true;
}

Client::~Client(){
    emit finished();
}

}
