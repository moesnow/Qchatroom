#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QString>

#include <WinSock2.h>
#include <algorithm>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#pragma comment(lib, "ws2_32.lib")

SOCKET serverSocket;
std::vector<SOCKET> clientSockets;
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

void handleClient(SOCKET clientSocket, std::string username) {
    char buffer[1024];  // 创建一个用于存储接收数据的缓冲区，大小为1024字节
    int bytesReceived;  // 用于存储接收的字节数

    // 创建一条消息，表示用户进入聊天室，欢迎用户
    std::string msg = username + " 进入聊天室，大家欢迎！";
    item->listWidget->addItem(msg.c_str());

    // 将消息广播给所有其他客户端
    for (SOCKET socket : clientSockets) {
        send(socket, msg.c_str(), msg.length(), 0);
    }

    try {
        while (true) {
            // 从客户端套接字(clientSocket)接收数据，将数据存储在buffer中
            bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);

            // 如果接收到的字节数小于或等于0，表示连接已关闭或出现错误，退出循环
            if (bytesReceived <= 0) {
                break;
            }

            buffer[bytesReceived] = '\0';  // 将接收到的数据标记为字符串的结尾

            // 创建消息，将用户名和接收到的数据拼接在一起，形成聊天消息
            std::string message = username + "：" + buffer;

            // 将消息添加到UI中的列表控件
            item->listWidget->addItem(message.c_str());

            // 将消息广播给所有其他客户端
            for (SOCKET socket : clientSockets) {
                send(socket, message.c_str(), message.length(), 0);
            }
        }
    } catch (...) {
        // 处理连接中断的异常情况
    }

    // 关闭当前客户端套接字
    closesocket(clientSocket);

    // 从clientSockets容器中移除当前客户端套接字
    clientSockets.erase(std::remove(clientSockets.begin(), clientSockets.end(), clientSocket), clientSockets.end());

    // 创建一条消息，表示该用户离开了聊天室
    msg = username + " 离开了聊天室！";

    // 将离开消息添加到UI中的列表控件
    item->listWidget->addItem(msg.c_str());

    // 将消息广播给所有其他客户端
    for (SOCKET socket : clientSockets) {
        send(socket, msg.c_str(), msg.length(), 0);
    }
}


void handleAccept() {
    // 向UI添加一条消息，表示聊天室服务端已成功启动
    item->listWidget->addItem("聊天室服务端启动成功！");

    // 禁用UI中的按钮（item->pushButton），防止重复启动服务器
    item->pushButton->setDisabled(true);

    while (true) {
        SOCKADDR_IN clientAddress; // 存储客户端地址信息的结构体
        int clientAddressSize = sizeof(clientAddress);

        // 等待来自客户端的连接请求，并接受连接
        SOCKET clientSocket = accept(serverSocket, (SOCKADDR*)&clientAddress, &clientAddressSize);

        // 创建一条消息，指示已接收到连接请求，并包含客户端的IP地址和端口信息
        std::string msg = "接收到连接来自：";
        msg += inet_ntoa(clientAddress.sin_addr); // 将IP地址转换为字符串形式
        msg += ":";
        msg += std::to_string(ntohs(clientAddress.sin_port)); // 获取端口号并转换为字符串
        item->listWidget->addItem(msg.c_str());

        char usernameBuffer[1024];
        // 从客户端接收用户名信息
        int usernameBytesReceived = recv(clientSocket, usernameBuffer, sizeof(usernameBuffer), 0);
        usernameBuffer[usernameBytesReceived] = '\0';
        std::string username = usernameBuffer; // 将接收到的用户名存储为字符串

        // 将客户端套接字(clientSocket)添加到服务器的客户端套接字列表(clientSockets)
        clientSockets.push_back(clientSocket);

        // 创建一个新线程，用于处理与客户端的通信，传递客户端套接字和用户名
        std::thread clientThread(handleClient, clientSocket, username);

        // 分离线程，使其在后台运行，无需等待线程结束
        clientThread.detach();
    }

    // 关闭服务器套接字
    closesocket(serverSocket);

    // 清理Winsock库的资源
    WSACleanup();

    // 启用UI中的按钮，以便可以重新启动服务器
    item->pushButton->setDisabled(false);
}


void server(const char *addr, unsigned short port) {
    // 初始化 Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        // 初始化Winsock库，如果失败则打印错误信息并返回
        std::cerr << "Failed to initialize Winsock" << std::endl;
        return;
    }

    // 创建服务器套接字
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        // 创建套接字失败时，打印错误信息、关闭Winsock库并返回
        std::cerr << "Failed to create server socket" << std::endl;
        WSACleanup(); // 清理Winsock资源
        return;
    }

    SOCKADDR_IN serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = inet_addr(addr);

    // 绑定服务器套接字到指定地址和端口
    if (bind(serverSocket, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
        // 绑定失败时，打印错误信息、关闭服务器套接字、关闭Winsock库并返回
        std::cerr << "Failed to bind server socket" << std::endl;
        closesocket(serverSocket); // 关闭服务器套接字
        WSACleanup(); // 清理Winsock资源
        return;
    }

    // 开始监听客户端连接请求
    if (listen(serverSocket, 5) == SOCKET_ERROR) {
        // 启动监听失败时，打印错误信息、关闭服务器套接字、关闭Winsock库并返回
        std::cerr << "Failed to listen on server socket" << std::endl;
        closesocket(serverSocket); // 关闭服务器套接字
        WSACleanup(); // 清理Winsock资源
        return;
    }

    // 创建一条消息，表示服务器已经开始监听指定地址和端口
    std::string msg = "服务端监听在：";
    msg += inet_ntoa(serverAddress.sin_addr); // 获取IP地址并转换为字符串
    msg += ":";
    msg += std::to_string(ntohs(serverAddress.sin_port)); // 获取端口号并转换为字符串
    item->listWidget->addItem(msg.c_str());

    // 创建一个新线程，用于处理接受连接的功能
    std::thread serverThread(handleAccept);

    // 分离线程，使其在后台运行
    serverThread.detach();
}


void MainWindow::on_pushButton_clicked()
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

    // 打印连接信息
    ui->listWidget->addItem("IP地址："+addr+" "+"端口号："+port);

    // 调用server函数，传递IP地址和端口号作为参数，启动服务器
    server(addr.toStdString().c_str(), port.toUShort());
}

