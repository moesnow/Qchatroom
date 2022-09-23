#include "serverwindow.h"
#include "ui_serverwindow.h"

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>

ServerWindow::ServerWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::ServerWindow)
{
    ui->setupUi(this);
}

ServerWindow::~ServerWindow()
{
    delete ui;
}

Ui::ServerWindow *item;

#define NAME_LEN 128
#define MAX_CLIENTS 256

struct Client {
    int fd;
    char name[NAME_LEN];
};

struct Client clts[MAX_CLIENTS];
size_t size = 0;

void errExit(const char *desc) {
    perror(desc);
    exit(-1);
}

void broadcast(const char *msg, int fd) {
    int i;
    for (i = 0; i < size; i++) {
        if (clts[i].fd != fd) {
            int ret = send(clts[i].fd, msg, strlen(msg) + 1, 0);
            if (ret <= 0) {
            }
        }
    }
}

void *run(void *arg) {
    int listenFd = (long)arg;
    fd_set fdsets;
    int i, ret;
    int maxfd;
    for (;;) {
        maxfd = listenFd;
        FD_ZERO(&fdsets);
        FD_SET(listenFd, &fdsets);
        for (i = 0; i < size; i++) {
            FD_SET(clts[i].fd, &fdsets);
            if (clts[i].fd > maxfd) {
                maxfd = clts[i].fd;
            }
        }
        ret = select(maxfd + 1, &fdsets, NULL, NULL, NULL);
        while (ret > 0) {
            if (FD_ISSET(listenFd, &fdsets)) {
                struct sockaddr_in caddr;
                socklen_t addrlen = sizeof(caddr);
                int cfd = accept(listenFd, (struct sockaddr *)&caddr, &addrlen);
                printf("%s %hu\n", inet_ntoa(caddr.sin_addr), ntohs(caddr.sin_port));
                clts[size].fd = cfd;
                strcpy(clts[size].name, "");
                ret--;
                size++;
            }
            for (i = 0; ret > 0 && i < size; i++) {
                int n = 1;
                if (FD_ISSET(clts[i].fd, &fdsets)) {
                    if (strcmp(clts[i].name, "") == 0) {
                        recv(clts[i].fd, clts[i].name, NAME_LEN, 0);
                        char msg[1024] = {};
                        char buffer[10];
                        sprintf(buffer, "%ld", size);
                        strcpy(msg, "当前在线人数:");
                        strcat(msg, buffer);
                        send(clts[i].fd, msg, strlen(msg) + 1, 0);
                        strcpy(msg, clts[i].name);
                        strcat(msg, "进入聊天室，大家欢迎!");
                        item->listWidget->addItem(msg);
                        broadcast(msg, clts[i].fd);
                    } else {
                        char msg[1024] = {};
                        strcpy(msg, clts[i].name);
                        strcat(msg, ":");
                        int len = strlen(msg);
                        n = recv(clts[i].fd, msg + len, 1024 - len, 0);
                        if (n <= 0) {
                            strcpy(msg, clts[i].name);
                            strcat(msg, "离开了聊天室!");
                        }
                        item->listWidget->addItem(msg);
                        broadcast(msg, clts[i].fd);
                    }
                    ret--;
                }
                if (n <= 0) {
                    close(clts[i].fd);
                    clts[i] = clts[size - 1];
                    size--;
                    i--;
                }
            }
        }
    }
}

void server(const char *ip, unsigned short port) {
    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd == -1) {
        errExit("socket");
    }
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);
    int ret;
    ret = bind(sfd, (const struct sockaddr *)&addr, sizeof(addr));
    if (ret != 0) {
        errExit("bind");
    }
    ret = listen(sfd, 5);
    if (ret != 0) {
        errExit("listen");
    }
    pthread_t id;
    pthread_create(&id, NULL, run, (void *)(long)sfd);
}

void ServerWindow::on_pushButton_clicked(){
    item = ui;

    QString ip=ui->addr->toPlainText();
    QString port=ui->port->toPlainText();
    ui->listWidget->addItem("IP地址："+ip+" 端口号："+port);
    ui->listWidget->addItem("聊天室服务端启动成功！");

    server(ip.toStdString().c_str(), port.toUShort());

    ui->pushButton->setDisabled(true);
}

