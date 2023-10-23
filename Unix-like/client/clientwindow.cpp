#include "clientwindow.h"
#include "ui_clientwindow.h"

#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>


Ui::ClientWindow *item;
int ret;
int sockfd;

ClientWindow::ClientWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::ClientWindow)
{
    ui->setupUi(this);
}

ClientWindow::~ClientWindow()
{
    delete ui;
}


#define LOG(fmt, args...) \
    printf("[%s %d]" fmt, __FILE__, __LINE__, ##args)

void errExit(const char *desc) {
    perror(desc);
    exit(-1);
}

void *recvMsg(void *arg) {
    long fd = (long)arg;
    for (;;) {
        char msg[1024] = {};
        if (recv(fd, msg, sizeof(msg), 0) <= 0) {
            return NULL;
        }
        item->listWidget->addItem(msg);
    }
}
void connectServer(const char *ip, unsigned short port) {
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        errExit("socket");
    }
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);
    ret = connect(sockfd, (const struct sockaddr *)&addr, sizeof(addr));
    if (ret != 0) {
        errExit("connect");
    }
    QString name=item->name->toPlainText();
    send(sockfd, name.toStdString().c_str(), strlen(name.toStdString().c_str()) + 1, 0);
    pthread_t id;
    pthread_create(&id, NULL, recvMsg, (void *)(long)sockfd);
}

void ClientWindow::on_connect_clicked()
{
    item = ui;
    QString ip=ui->addr->toPlainText();
    QString port=ui->port->toPlainText();
    ui->listWidget->addItem("IP地址："+ip+" 端口号："+port);

    connectServer(ip.toStdString().c_str(), port.toUShort());

    ui->connect->setDisabled(true);
    ui->send->setDisabled(false);
}


void ClientWindow::on_send_clicked()
{
    QString ip=item->text->toPlainText();
    ret = send(sockfd, ip.toStdString().c_str(), strlen(ip.toStdString().c_str()) + 1, 0);
    item->text->setPlainText("");
}

