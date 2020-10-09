#include "stereostreamer.h"
#include <QtCore/QCoreApplication>
#include <signal.h>
 
int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    StereoStreamer::Server* streamer;
    streamer = new StereoStreamer::Server("127.0.0.1","8000");

    streamer->startServer();

    signal(SIGINT, SIG_DFL);
    while(1){
        QCoreApplication::processEvents();
    }
 
    streamer->stopServer();
    std::cout << "Program has ended successfully" << std::endl;
 
    system("pause");
    
    return 0;
}
