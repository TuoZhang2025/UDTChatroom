#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <limits>  // 添加 numeric_limits 模板类的声明
using namespace std;

#pragma comment(lib, "ws2_32.lib")

#define BUFFER_SIZE 1024

// 接收线程函数
DWORD WINAPI ReceiveThread(LPVOID lpParam) {
    SOCKET sock = *(SOCKET*)lpParam;
    char recvBuf[BUFFER_SIZE] = { 0 };

    while (true) {
        SOCKADDR_IN serverAddr;
        int addrLen = sizeof(serverAddr);

        ZeroMemory(recvBuf, sizeof(recvBuf));

        int recvLen = recvfrom(sock, recvBuf, BUFFER_SIZE, 0,
            (SOCKADDR*)&serverAddr, &addrLen);
        if (recvLen > 0) {
            recvBuf[recvLen] = '\0';
            cout << recvBuf << endl;
        }
    }
    return 0;
}

int main() {
    SetConsoleOutputCP(936);
    SetConsoleCP(936);

    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET clientSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    SOCKADDR_IN serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(6666);

    // 提示用户输入服务器IP地址
    string serverIP;
    cout << "请输入服务器IP地址: ";
    cin >> serverIP;
    if (inet_pton(AF_INET, serverIP.c_str(), &serverAddr.sin_addr) != 1) {
        cerr << "无效的 IP 地址格式" << endl;
        // 处理错误（如退出程序或重新输入）
        return 1;
    }

    // 清空输入缓冲区
    cin.ignore((numeric_limits<streamsize>::max)(), '\n');

    cout << "客户端已启动，输入消息发送给服务器，exit退出" << endl;

    HANDLE hThread = CreateThread(NULL, 0, ReceiveThread, &clientSock, 0, NULL);

    char msg[BUFFER_SIZE] = { 0 };
    while (true) {
        cout << "你: ";
        cin.getline(msg, sizeof(msg));
        if (strcmp(msg, "exit") == 0) break;

        sendto(clientSock, msg, strlen(msg), 0,
            (SOCKADDR*)&serverAddr, sizeof(serverAddr));

        ZeroMemory(msg, sizeof(msg));
    }

    CloseHandle(hThread);
    closesocket(clientSock);
    WSACleanup();
    return 0;
}
