#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <winsock2.h>
#include <iostream>
#include <thread>
#include <string>

SOCKET client_socket;
Ui::MainWindow *item;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void receive_messages(SOCKET client_socket) {
    // 禁用UI中的连接按钮，使其在接收消息期间不可用
    item->connect->setDisabled(true);

    // 启用UI中的发送按钮，以便可以发送消息
    item->send->setDisabled(false);

    while (true) {
        char buffer[1024]; // 创建一个缓冲区，用于接收消息，大小为1024字节
        int bytesReceived = recv(client_socket, buffer, sizeof(buffer), 0); // 从服务器接收消息

        // 如果接收到的字节数小于或等于0，表示连接已关闭或出现错误，退出循环
        if (bytesReceived <= 0) {
            std::cerr << "Connection closed by the server." << std::endl;
            break;
        }

        // 将接收到的数据标记为字符串的结尾
        buffer[bytesReceived] = '\0';

        // 将接收到的消息添加到UI中的列表控件
        item->listWidget->addItem(buffer);
    }

    // 清理并关闭客户端套接字
    closesocket(client_socket);

    // 清理Winsock库的资源
    WSACleanup();

    // 启用UI中的连接按钮，以便可以重新连接服务器
    item->connect->setDisabled(false);

    // 禁用UI中的发送按钮，因为连接已经关闭
    item->send->setDisabled(true);
}


void client(const char *addr, unsigned short port, const std::string username) {
    // 初始化 Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        // 初始化Winsock库，如果失败则打印错误信息并返回
        std::cerr << "Failed to initialize Winsock." << std::endl;
        return;
    }

    // 创建一个TCP套接字
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == INVALID_SOCKET) {
        // 创建套接字失败时，打印错误信息、关闭Winsock库并返回
        std::cerr << "Socket creation failed." << std::endl;
        WSACleanup(); // 清理Winsock资源
        return;
    }

    // 服务器地址和端口信息
    sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr(addr); // 服务器IP地址
    server_address.sin_port = htons(port); // 服务器端口

    // 连接到服务器
    if (connect(client_socket, (struct sockaddr*)&server_address, sizeof(server_address)) == SOCKET_ERROR) {
        // 连接服务器失败时，打印错误信息、关闭客户端套接字、关闭Winsock库并返回
        std::cerr << "Connection to the server failed." << std::endl;
        closesocket(client_socket); // 关闭客户端套接字
        WSACleanup(); // 清理Winsock资源
        return;
    }

    // 发送用户名到服务器
    send(client_socket, username.c_str(), username.length(), 0);

    // 创建一个新线程，用于接收从服务器发来的消息
    std::thread receive_thread(receive_messages, client_socket);

    // 分离线程，使其在后台运行
    receive_thread.detach();
}

void MainWindow::on_connect_clicked()
{
    item = ui;

    // 获取IP地址
    QString addr = ui->addr->toPlainText();
    if (addr.isEmpty()){
        addr = ui->addr->placeholderText();
    }

    // 获取端口号
    QString port = ui->port->toPlainText();
    if (port.isEmpty()){
        port = ui->port->placeholderText();
    }

    // 获取用户名
    QString name = ui->name->toPlainText();
    if (name.isEmpty()){
        name = ui->name->placeholderText();
    }

    // 打印连接信息
    ui->listWidget->addItem("IP地址："+addr+" "+"端口号："+port+" "+"用户名："+name);

    // 调用client函数，传递IP地址、端口号和用户名作为参数，启动消息接受
    client(addr.toStdString().c_str(), port.toUShort(),name.toStdString());
}


void MainWindow::on_send_clicked()
{
    // 获取消息内容
    QString message = ui->text->toPlainText();
    if (message.isEmpty()){
        message = ui->text->placeholderText();
    }

    // 发送消息内容
    if (send(client_socket, message.toStdString().c_str(), message.toStdString().length(), 0) == SOCKET_ERROR) {
        std::cerr << "Send failed." << std::endl;
        return;
    }

    // 重置消息框
    ui->text->clear();
}

