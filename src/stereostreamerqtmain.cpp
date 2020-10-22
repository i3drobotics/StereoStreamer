/**
 * @file main.cpp
 * @authors Ben Knight (bknight@i3drobotics.com)
 * @brief
 * @version 0.0.1a
 * @date 2020-10-22
 *
 * @copyright Copyright (c) I3D Robotics Ltd, 2020
 *
 */

#include <QApplication>
#include <QFile>

#include "udpwindow.h"

int main(int argc, char *argv[]) {
  QApplication a(argc, argv);

  UDPWindow w;
  w.show();

  int ret = a.exec();
  return ret;
}
