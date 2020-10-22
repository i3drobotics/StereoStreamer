#include "udpwindow.h"
#include "ui_udpwindow.h"

UDPWindow::UDPWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::UDPWindow)
{
    ui->setupUi(this);
}

UDPWindow::~UDPWindow()
{
    delete ui;
}
