#define _WINSOCKAPI_  // 确保正确包含 Winsock 函数
#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>  // 添加 inet_ntop 函数的声明
#include <windows.h>
#include <string>
#pragma comment(lib, "ws2_32.lib")

#define SERV_UDP_PORT 6666
#define BUFFER_SIZE 1024
#define INET_ADDRSTRLEN 16  // 定义 IPv4 地址字符串的最大长度

// 客户端信息结构
typedef struct {
    SOCKADDR_IN addr;
    char name[50];
    int connected;
} ClientInfo;

ClientInfo client = { 0 };
CRITICAL_SECTION cs;

// UTF-8 转 GBK
std::string Utf8ToGbk(const char* utf8) {
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
    wchar_t* wstr = new wchar_t[len + 1];
    memset(wstr, 0, len + 1);
    MultiByteToWideChar(CP_UTF8, 0, utf8, -1, wstr, len);

    len = WideCharToMultiByte(CP_ACP, 0, wstr, -1, NULL, 0, NULL, NULL);
    char* str = new char[len + 1];
    memset(str, 0, len + 1);
    WideCharToMultiByte(CP_ACP, 0, wstr, -1, str, len, NULL, NULL);

    std::string result = str;
    delete[] wstr;
    delete[] str;
    return result;
}

// 发送消息给客户端
void sendToClient(SOCKET sock, char* message) {
    if (client.connected) {
        std::string gbkMsg = Utf8ToGbk(message);
        const char* msg = gbkMsg.c_str();

        char serverMsg[BUFFER_SIZE] = { 0 };
        sprintf_s(serverMsg, BUFFER_SIZE, "服务器: %s", msg);

        sendto(sock, serverMsg, strlen(serverMsg), 0,
            (LPSOCKADDR)&client.addr, sizeof(client.addr));
        printf("发送给客户端: %s\n", message);
    }
}

// 接收客户端消息的线程
DWORD WINAPI ReceiveThread(LPVOID lpParam) {
    SOCKET theSocket = (SOCKET)lpParam;
    char szBuf[BUFFER_SIZE] = { 0 };

    while (1) {
        SOCKADDR_IN saClient;
        int nLen = sizeof(saClient);

        // 接收前清空缓冲区
        ZeroMemory(szBuf, BUFFER_SIZE);

        int nRet = recvfrom(theSocket, szBuf, BUFFER_SIZE, 0,
            (LPSOCKADDR)&saClient, &nLen);

        if (nRet > 0) {
            EnterCriticalSection(&cs);

            // 检查是否是新客户端连接
            if (!client.connected ||
                client.addr.sin_addr.s_addr != saClient.sin_addr.s_addr ||
                client.addr.sin_port != saClient.sin_port) {

                client.addr = saClient;
                client.connected = 1;
                sprintf_s(client.name, sizeof(client.name), "客户端");

                char welcomeMsg[BUFFER_SIZE] = { 0 };
                sprintf_s(welcomeMsg, BUFFER_SIZE, "欢迎连接到服务器");
                sendToClient(theSocket, welcomeMsg);

                // 使用inet_ntop替代inet_ntoa
                char ipStr[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &client.addr.sin_addr, ipStr, INET_ADDRSTRLEN);
                printf("客户端已连接: %s:%d\n", ipStr, ntohs(client.addr.sin_port));
            }

            // 显示客户端消息
            printf("客户端: %s\n", szBuf);

            LeaveCriticalSection(&cs);
        }
    }
    return 0;
}

// 主函数
void DatagramServer(short nPort) {
    SOCKET theSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    SOCKADDR_IN saServer;
    saServer.sin_family = AF_INET;
    saServer.sin_addr.s_addr = INADDR_ANY;
    saServer.sin_port = htons(nPort);
    bind(theSocket, (LPSOCKADDR)&saServer, sizeof(saServer));

    printf("服务器已启动，监听端口 %d\n", nPort);
    printf("输入消息发送给客户端，输入exit退出\n");

    HANDLE hThread = CreateThread(NULL, 0, ReceiveThread, (LPVOID)theSocket, 0, NULL);

    char szBuf[BUFFER_SIZE] = { 0 };

    while (1) {
        fgets(szBuf, BUFFER_SIZE - 1, stdin);

        if (strcmp(szBuf, "exit\n") == 0) break;

        // 移除换行符
        szBuf[strcspn(szBuf, "\n")] = 0;

        sendToClient(theSocket, szBuf);

        // 清空输入缓冲区
        ZeroMemory(szBuf, BUFFER_SIZE);
    }

    CloseHandle(hThread);
    closesocket(theSocket);
}

int main() {
    InitializeCriticalSection(&cs);
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    DatagramServer(SERV_UDP_PORT);
    WSACleanup();
    DeleteCriticalSection(&cs);
    return 0;
}
