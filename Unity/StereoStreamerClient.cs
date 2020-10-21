using UnityEngine;
using System.Collections.Generic;
using System;
using System.IO;
using System.Text;
using System.Net.Sockets;
using OpenCVForUnity.CoreModule;
using OpenCVForUnity.ImgcodecsModule;
using OpenCVForUnity.ImgprocModule;
using OpenCVForUnity.UtilsModule;
using OpenCVForUnity.UnityUtils;
using System.Linq;

using System.Net;
using System.Threading;
using TMPro;

class StereoStreamer
{
    private const int max_clients_ = 5;
    private const int max_buffer_length_ = 65535;
    private const char eom_token_ = '\n';
    private const int INVALID_CLIENT_ID = -1; //client id used to signify a client id error
    private const int SERVER_CLIENT_ID = -2; //client id held by the server
    private const int SEND_TO_ALL_CLIENTS = -3; //client id used to signify message should be sent to all clients
    private const string SERVER_FULL = "Server full";

    public enum MsgType { MSG_TYPE_STRING, MSG_TYPE_IMAGE, MSG_TYPE_CLIENT_ID, MSG_TYPE_SERVER_MSG, MSG_TYPE_NOT_SET, MSG_TYPE_ERROR };

    public class ServerMsg
    {
        public int senderClientId;
        public int targetClientId;
        public MsgType msgType;
        public int msgEndIndex;
        public int msgIndex;
        public string msg;

        public ServerMsg()
        {
            this.senderClientId = INVALID_CLIENT_ID;
            this.targetClientId = INVALID_CLIENT_ID;
            this.msgType = MsgType.MSG_TYPE_NOT_SET;
            this.msg = "";
            this.msgEndIndex = 1;
            this.msgIndex = 0;
        }

        public ServerMsg(int senderClientId, int targetClientId, MsgType msgType, string msg)
        {
            this.senderClientId = senderClientId;
            this.targetClientId = targetClientId;
            this.msgType = msgType;
            this.msg = msg;
            this.msgEndIndex = 1;
            this.msgIndex = 0;
        }

        public ServerMsg(int senderClientId, int targetClientId, MsgType msgType, string msg, int msgEndIndex, int msgIndex)
        {
            this.senderClientId = senderClientId;
            this.targetClientId = targetClientId;
            this.msgType = msgType;
            this.msg = msg;
            this.msgEndIndex = msgEndIndex;
            this.msgIndex = msgIndex;
        }

        public ServerMsg(string raw_server_msg)
        {
            // convert from server message format
            // comma seperated string
            // SENDER_CLIENT_ID,TARGET_CLIENT_ID,MSG_TYPE,MSG_END_INDEX,MSG_INDEX,MSG
            if (raw_server_msg[raw_server_msg.Length - 1] == '\n')
            {
                raw_server_msg = raw_server_msg.Substring(0, raw_server_msg.Length - 1);
                string[] msg_split = raw_server_msg.Split(',');
                if (msg_split.Length == 6)
                {
                    try
                    {
                        this.senderClientId = Int32.Parse(msg_split[0]); // SENDER_CLIENT_ID
                    }
                    catch (FormatException)
                    {
                        // if no conversion could be performed
                        this.senderClientId = INVALID_CLIENT_ID;
                    }
                    try
                    {
                        this.targetClientId = Int32.Parse(msg_split[1]); //TARGET_CLIENT_ID
                    }
                    catch (FormatException)
                    {
                        // if no conversion could be performed
                        this.targetClientId = INVALID_CLIENT_ID;
                    }
                    try
                    {
                        int msg_type_index = (Int32.Parse(msg_split[2]));
                        this.msgType = (MsgType)msg_type_index; //MSG_TYPE
                    }
                    catch (FormatException)
                    {
                        this.msgType = MsgType.MSG_TYPE_ERROR;
                    }
                    try
                    {
                        int msg_index = (Int32.Parse(msg_split[3]));
                        this.msgEndIndex = msg_index; //MSG_END_INDEX
                    }
                    catch (FormatException)
                    {
                        this.msgEndIndex = -1;
                    }
                    try
                    {
                        int msg_index = (Int32.Parse(msg_split[4]));
                        this.msgIndex = msg_index; //MSG_INDEX
                    }
                    catch (FormatException)
                    {
                        // if no conversion could be performed
                        this.msgIndex = -1;
                    }
                    this.msg = msg_split[5]; //MSG
                }
                else
                {
                    Debug.LogError("Currupt message received. Expects 6 comma delimited string.");
                    invalidMessage();
                }
            } else
            {
                Debug.LogError("Currupt message received. Expects newline at end of string.");
                invalidMessage();
            }
        }

        private void invalidMessage()
        {
            this.senderClientId = INVALID_CLIENT_ID;
            this.targetClientId = INVALID_CLIENT_ID;
            this.msgType = MsgType.MSG_TYPE_ERROR;
            this.msgEndIndex = -1;
            this.msgIndex = -1;
            this.msg = "";
        }

        public string get_raw_server_message()
        {
            // convert to server message format
            // comma seperated string
            // SENDER_CLIENT_ID,TARGET_CLIENT_ID,MSG_TYPE,MSG_END_INDEX,MSG_INDEX,MSG
            string raw_msg = this.senderClientId.ToString();
            raw_msg += ",";
            raw_msg += this.targetClientId.ToString();
            raw_msg += ",";
            raw_msg += this.msgType.ToString();
            raw_msg += ",";
            raw_msg += this.msgEndIndex.ToString();
            raw_msg += ",";
            raw_msg += this.msgIndex.ToString();
            raw_msg += ",";
            raw_msg += this.msg;
            raw_msg += eom_token_;
            return raw_msg;
        }
    };

    public class Client
    {
        private Socket _clientSocket = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
        private byte[] _recieveBuffer = new byte[65535];
        private IPAddress ip_;
        private string ip_str_;
        private int port_;
        private Texture2D ImageTexture_;
        private bool runAsync_;
        private bool enableCallbacks;

        private List<ServerMsg> validSrvMsgs;
        private List<ServerMsg>[] clientSrvMsgLists; //List for each possible client
        private List<ServerMsg> serverSrvMsgs; //List for messages from the server
        private String full_message_buffer;

        public Client(string ip, int port, Texture2D ImageTexture)
        {
            ip_str_ = ip;
            ip_ = IPAddress.Parse(ip_str_);
            port_ = port;
            ImageTexture_ = ImageTexture;
            runAsync_ = false; // Force non async run untill working
            enableCallbacks = false;
            validSrvMsgs = new List<ServerMsg>();
            clientSrvMsgLists = new List<ServerMsg>[max_clients_];
            for (int i = 0; i < max_clients_; i++)
            {
                clientSrvMsgLists[i] = new List<ServerMsg>();
            }
            serverSrvMsgs = new List<ServerMsg>();
        }

        ~Client()
        {
            Disconnect();
        }

        public void Disconnect()
        {
            enableCallbacks = false;
            if (runAsync_)
            {
                _clientSocket.Close();
            }
        }

        public void Connect()
        {
            bool isConnected = false;

            try
            {
                _clientSocket.Connect(new IPEndPoint(ip_, port_));
                // If we got here without an exception we should be connected to the server
                isConnected = true;
                Debug.Log("Connected to server");
            }
            catch (SocketException ex)
            {
                Debug.Log(ex.Message);
            }

            if (isConnected)
            {
                if (runAsync_)
                {
                    enableCallbacks = true;
                    // We are now connected, start to receive
                    _clientSocket.BeginReceive(_recieveBuffer, 0, _recieveBuffer.Length, SocketFlags.None, new AsyncCallback(ReceiveCallback), null);
                }
            }
        }

        private void ReceiveCallback(IAsyncResult AR)
        {
            if (enableCallbacks)
            {
                //Check how much bytes are recieved and call EndRecieve to finalize handshake
                try
                {
                    int recieved = _clientSocket.EndReceive(AR);

                    if (recieved <= 0)
                        return;

                    ProcessData(_recieveBuffer, recieved);
                    //Start receiving again
                    _clientSocket.BeginReceive(_recieveBuffer, 0, _recieveBuffer.Length, SocketFlags.None, new AsyncCallback(ReceiveCallback), null);
                }
                catch (SocketException ex)
                {
                    Debug.Log(ex.Message);

                    // If the socket connection was lost, we need to reconnect
                    if (!_clientSocket.Connected)
                    {
                        Connect();
                    }
                    else
                    {
                        //Just a read error, we are still connected
                        _clientSocket.BeginReceive(_recieveBuffer, 0, _recieveBuffer.Length, SocketFlags.None, new AsyncCallback(ReceiveCallback), null);
                    }
                }
            }
        }

        public void Receive()
        {
            int recieved = _clientSocket.Receive(_recieveBuffer);
            ProcessData(_recieveBuffer, recieved);
        }

        private void ProcessData(byte[] receiveBuffer, int bufferSize)
        {
            //Copy the recieved data into new buffer , to avoid null bytes
            byte[] recData = new byte[bufferSize];
            Buffer.BlockCopy(receiveBuffer, 0, recData, 0, bufferSize);

            full_message_buffer += Encoding.ASCII.GetString(recData);
            if (full_message_buffer.Contains('\n'))
            {
                //Debug.Log("Message buffer contains new line");
                // split buffer into packets
                string[] packets = full_message_buffer.Split('\n');
                int packet_index = 0;
                foreach (string packet in packets)
                {
                    if (packet_index == packets.Length - 1) break; //ignore last packet as will be incomplete
                    ServerMsg srvMsg = new ServerMsg(packet + '\n');
                    if (srvMsg.msgIndex != -1 || srvMsg.msgEndIndex != -1)
                    {
                        validSrvMsgs.Add(srvMsg);
                    }
                    packet_index++;
                }

                full_message_buffer = packets[packets.Length - 1];

                // sort server messages
                foreach (ServerMsg validSrvMsg in validSrvMsgs)
                {
                    if (validSrvMsg.senderClientId == -2)
                    {
                        serverSrvMsgs.Add(validSrvMsg);
                    }
                    else if (validSrvMsg.senderClientId >= 0)
                    {
                        clientSrvMsgLists[validSrvMsg.senderClientId].Add(validSrvMsg);
                    }
                }

                validSrvMsgs.Clear();

                //TODO process direct server messages
                serverSrvMsgs.Clear(); // clear server messages after processed

                // process server messages from clients
                List<bool>[] items_processed = new List<bool>[max_clients_];
                for (int client_num = 0; client_num < max_clients_; client_num++)
                {
                    items_processed[client_num] = new List<bool>();
                    for (int item_index = 0; item_index < clientSrvMsgLists[client_num].Count; item_index++)
                    {
                        items_processed[client_num].Add(false);
                    }
                }

                for (int client_num = 0; client_num < max_clients_; client_num++)
                {
                    List<ServerMsg> clientSrvMsgList = clientSrvMsgLists[client_num];
                    for (int item_index = 0; item_index < clientSrvMsgList.Count; item_index++){
                        ServerMsg clientSrvMsg = clientSrvMsgList[item_index];
                        if (clientSrvMsg.msgEndIndex == 1)
                        {
                            // process single packet message
                            for (int i = 0; i < item_index; i++)
                            {
                                items_processed[client_num][i] = true;
                            }
                            // remove from process list
                            items_processed[client_num][item_index] = true;
                        }
                        else if (clientSrvMsg.msgEndIndex >= 2 && clientSrvMsg.msgIndex == 0 && (clientSrvMsgList.Count - item_index > clientSrvMsg.msgEndIndex))
                        {
                            // mark all preview items as processed
                            for (int i = 0; i < item_index; i++)
                            {
                                items_processed[client_num][i] = true;
                            }
                            // search next n items to build complete packet
                            bool valid_next_items = true;
                            List<ServerMsg> multiPacketServerMsgs = new List<ServerMsg>();
                            int item_loc = item_index;
                            for (int i = 0; i < clientSrvMsg.msgEndIndex; i++)
                            {
                                item_loc = item_index + i;
                                items_processed[client_num][item_loc] = true;
                                //clientSrvMsgListsUnprocesssed[client_num].RemoveAt(item_loc);
                                if (i != clientSrvMsgList[item_loc].msgIndex)
                                {
                                    valid_next_items = false;
                                    break;
                                } else
                                {
                                    multiPacketServerMsgs.Add(clientSrvMsgList[item_loc]);
                                }
                            }
                            item_index = item_loc; //skip processing items that have been process inside here
                            if (valid_next_items)
                            {
                                Debug.Log("found packet list");
                                // process multi packet server message
                                String joint_packet_msg = "";
                                foreach (ServerMsg srvMsgPacket in multiPacketServerMsgs)
                                {
                                    joint_packet_msg += srvMsgPacket.msg;
                                }
                                ServerMsg srvMsg_full = new ServerMsg(
                                    multiPacketServerMsgs[0].senderClientId, multiPacketServerMsgs[0].targetClientId, 
                                    multiPacketServerMsgs[0].msgType, joint_packet_msg
                                );
                                ReadServerMessage(srvMsg_full);
                            }
                        }
                    }
                }

                // re-populate client server message lists with only unprocessed items
                for (int client_num = 0; client_num < max_clients_; client_num++)
                {
                    List<ServerMsg> newClientSrvMsgList = new List<ServerMsg>();
                    for (int item_index = 0; item_index < items_processed[client_num].Count; item_index++)
                    {
                        if (!items_processed[client_num][item_index])
                        {
                            newClientSrvMsgList.Add(clientSrvMsgLists[client_num][item_index]);
                        }
                    }
                    clientSrvMsgLists[client_num] = new List<ServerMsg>(newClientSrvMsgList);
                }
            }
        }

        private void ReadServerMessage(ServerMsg srvMsg) {
            if (srvMsg.msgType == MsgType.MSG_TYPE_IMAGE)
            {
                Debug.Log("Updating texture...");
                String message = srvMsg.msg;
                Mat image = str2mat(message);
                if (image.width() > 0 && image.height() > 0)
                {
                    Core.flip(image, image, 0);
                    Core.flip(image, image, 1);
                    if (image.type() == CvType.CV_8UC3)
                    {
                        Imgproc.cvtColor(image, image, Imgproc.COLOR_BGR2RGB);
                        Utils.matToTexture2D(image, ImageTexture_);
                    } else if (image.type() == CvType.CV_32FC1)
                    {
                        Mat disp_colormap = DisparityController.Disparity2Colormap(image,255,false);
                        Imgproc.cvtColor(disp_colormap, disp_colormap, Imgproc.COLOR_BGR2RGB);
                        Utils.matToTexture2D(disp_colormap, ImageTexture_);
                    } else
                    {
                        Debug.Log("Unsupported CV image type: " + image.type().ToString());
                    }
                }
            }
            else if (srvMsg.msgType == MsgType.MSG_TYPE_STRING)
            {
                Debug.Log(srvMsg.msg);
            }
        }

        private Mat str2mat(String s)
        {
            // Decode data
            byte[] byteArr = Convert.FromBase64String(s);
            List<byte> byteList = new List<Byte>(byteArr);
            Mat data = Converters.vector_char_to_Mat(byteList);
            Mat img = Imgcodecs.imdecode(data, Imgcodecs.IMREAD_UNCHANGED);
            return img;
        }

        private void SendData(byte[] data)
        {
            SocketAsyncEventArgs socketAsyncData = new SocketAsyncEventArgs();
            socketAsyncData.SetBuffer(data, 0, data.Length);
            _clientSocket.SendAsync(socketAsyncData);
        }

    };

}

public class StereoStreamerClient : MonoBehaviour
{
    [SerializeField, TooltipAttribute("Server IP")]
    public String Host = "127.0.0.1";
    [SerializeField, TooltipAttribute("Server port")]
    public Int32 Port = 8000;

    [SerializeField, TooltipAttribute("Image width")]
    public int image_width = 752;
    [SerializeField, TooltipAttribute("Image height")]
    public int image_height = 480;

    /// <summary>
    /// The right image texture.
    /// </summary>
    [SerializeField, TooltipAttribute("Material to render image")]
    public Material ImageMaterial;

    /// <summary>
    /// Stereo matching algorithm to use.
    /// </summary>
    public enum NetworkImageType { Left, Right, Disparity }
    [SerializeField, TooltipAttribute("Network image type")]
    public NetworkImageType networkImageType;

    /// <summary>
    /// The left image texture.
    /// </summary>
    private Texture2D ImageTexture;

    StereoStreamer.Client client;

    void Start()
    {
        if (ImageTexture == null || ImageTexture.width != image_width || ImageTexture.height != image_height)
            ImageTexture = new Texture2D(image_width, image_height, TextureFormat.RGBA32, false);

        ImageMaterial.mainTexture = ImageTexture;

        client = new StereoStreamer.Client(Host, Port, ImageTexture);
        client.Connect();
    }

    void Update()
    {
        client.Receive();
    }

    void OnDestroy()
    {
        client.Disconnect();
    }
}