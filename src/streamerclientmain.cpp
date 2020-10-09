#include "stereostreamer.h"
#include <signal.h>
#include <QtCore/QCoreApplication>
 
int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    StereoStreamer::Client* streamer_client;
    streamer_client = new StereoStreamer::Client("127.0.0.1","8000");
    if (!streamer_client->startClient()){
        std::cout << "Failed to connect client";
    } else {
        // even clients are writers only (disable read)
        //bool isReader = (streamer_client->getClientId() + 1) % 2 == 0;
        //streamer_client->enableReader(isReader);
        signal(SIGINT, SIG_DFL);
        std::string test_msg = "Hello world!";
        cv::Mat test_img(1000, 1000, CV_8UC3);
        while (1) {
            Sleep(100);
            streamer_client->clientSendStringMessage(test_msg);
            QCoreApplication::processEvents();
            /*
            Sleep(100);
            cv::randu(test_img, cv::Scalar(0, 0, 0), cv::Scalar(255, 255, 255));
            streamer_client->clientSendUCharImage(test_img);
            */
            QCoreApplication::processEvents();
        }

        streamer_client->stopClient();
        std::cout << "Program has ended successfully" << std::endl;
    }

    system("pause");

    return 0;
}
