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

public class StereoStreamerClient : MonoBehaviour
{
    internal Boolean socketReady = false;
    TcpClient mySocket;
    NetworkStream theStream;
    StreamWriter theWriter;
    StreamReader theReader;
    String Host = "127.0.0.1";
    Int32 Port = 8000;
    const int datasize = 2444;
    char[] buffer = new char[datasize];

    int image_width = 640;
    int image_height = 480;

    /// <summary>
    /// The right image texture.
    /// </summary>
    [SerializeField, TooltipAttribute("Material to render image")]
    public Material ImageMaterial;

    /// <summary>
    /// The left image texture.
    /// </summary>
    private Texture2D ImageTexture;

    // Start is called before the first frame update
    void Start()
    {
        setupSocket();

        if (ImageTexture == null || ImageTexture.width != image_width || ImageTexture.height != image_height)
            ImageTexture = new Texture2D(image_width, image_height, TextureFormat.RGBA32, false);

        ImageMaterial.mainTexture = ImageTexture;
    }

    // Update is called once per frame
    void Update()
    {
        String msg = readSocket();
        //base64_decode(msg);
        Mat image = str2mat(msg);
        if (image.width() > 0 && image.height() > 0)
        {
            Core.flip(image, image, 0);
            Core.flip(image, image, 1);
            Imgproc.cvtColor(image, image, Imgproc.COLOR_BGR2RGB);
            Utils.matToTexture2D(image, ImageTexture);
        }
    }

    // **********************************************
    public void setupSocket()
    {
        try
        {
            mySocket = new TcpClient(Host, Port);
            theStream = mySocket.GetStream();
            theWriter = new StreamWriter(theStream);
            theReader = new StreamReader(theStream);
            socketReady = true;
        }
        catch (Exception e)
        {
            Debug.Log("Socket error: " + e);
        }
    }
    public void writeSocket(string theLine)
    {
        if (!socketReady)
            return;
        String foo = theLine + "\r\n";
        theWriter.Write(foo);
        theWriter.Flush();
    }
    public String readSocket()
    {
        if (!socketReady)
            return "";
        if (theStream.DataAvailable)
        {
            /*
            for (int i = 0; i < datasize + 1; i++)
            {
                char ch = (char)theReader.Read();
                if (i > 0)
                {
                    buffer[i-1] = ch;
                }
            }
            */
            //Debug.Log(theReader.Read());
            //theReader.ReadBlock(buffer, 0, datasize);
            //String myString = new String(buffer);
            String myString = theReader.ReadLine();
            //String myString = theReader.ReadToEnd();
            return myString;
        }
        return "";
    }
    public void closeSocket()
    {
        if (!socketReady)
            return;
        theWriter.Close();
        theReader.Close();
        mySocket.Close();
        socketReady = false;
    }

    String base64_decode(String encoded_string)
    {
        string base64Decoded;
        byte[] data = System.Convert.FromBase64String(encoded_string);
        base64Decoded = System.Text.ASCIIEncoding.ASCII.GetString(data);
        return base64Decoded;
    }

    Mat str2mat(String s)
    {
        // Decode data
        //Debug.Log(s);
        byte[] byteArr = System.Convert.FromBase64String(s);
        List<byte> byteList = new List<Byte>(byteArr);

        // Decode data
        //string decoded_string = base64_decode(s);
        //List<byte> byteList = new List<Byte>(Encoding.ASCII.GetBytes(decoded_string));
        Mat data = Converters.vector_char_to_Mat(byteList);
        //Debug.Log("Converted to vector mat");
        //System.Collections.Generic.List<char> data(decoded_string.begin(), decoded_string.end());
        //List<uchar> data(decoded_string.begin(), decoded_string.end());
        //Debug.Log("Converted to image");
        Mat img = Imgcodecs.imdecode(data, Imgcodecs.IMREAD_UNCHANGED);
	    return img;
    }

    private void OnDestroy()
    {
        closeSocket();
    }

} // end class s_TCP
