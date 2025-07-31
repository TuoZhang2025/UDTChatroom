#define _WINSOCKAPI_  // ȷ����ȷ���� Winsock ����
#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>  // ��� inet_ntop ����������
#include <windows.h>
#include <string>
#pragma comment(lib, "ws2_32.lib")

#define SERV_UDP_PORT 6666
#define BUFFER_SIZE 1024
#define INET_ADDRSTRLEN 16  // ���� IPv4 ��ַ�ַ�������󳤶�

// �ͻ�����Ϣ�ṹ
typedef struct {
    SOCKADDR_IN addr;
    char name[50];
    int connected;
} ClientInfo;

ClientInfo client = { 0 };
CRITICAL_SECTION cs;

// UTF-8 ת GBK
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

// ������Ϣ���ͻ���
void sendToClient(SOCKET sock, char* message) {
    if (client.connected) {
        std::string gbkMsg = Utf8ToGbk(message);
        const char* msg = gbkMsg.c_str();

        char serverMsg[BUFFER_SIZE] = { 0 };
        sprintf_s(serverMsg, BUFFER_SIZE, "������: %s", msg);

        sendto(sock, serverMsg, strlen(serverMsg), 0,
            (LPSOCKADDR)&client.addr, sizeof(client.addr));
        printf("���͸��ͻ���: %s\n", message);
    }
}

// ���տͻ�����Ϣ���߳�
DWORD WINAPI ReceiveThread(LPVOID lpParam) {
    SOCKET theSocket = (SOCKET)lpParam;
    char szBuf[BUFFER_SIZE] = { 0 };

    while (1) {
        SOCKADDR_IN saClient;
        int nLen = sizeof(saClient);

        // ����ǰ��ջ�����
        ZeroMemory(szBuf, BUFFER_SIZE);

        int nRet = recvfrom(theSocket, szBuf, BUFFER_SIZE, 0,
            (LPSOCKADDR)&saClient, &nLen);

        if (nRet > 0) {
            EnterCriticalSection(&cs);

            // ����Ƿ����¿ͻ�������
            if (!client.connected ||
                client.addr.sin_addr.s_addr != saClient.sin_addr.s_addr ||
                client.addr.sin_port != saClient.sin_port) {

                client.addr = saClient;
                client.connected = 1;
                sprintf_s(client.name, sizeof(client.name), "�ͻ���");

                char welcomeMsg[BUFFER_SIZE] = { 0 };
                sprintf_s(welcomeMsg, BUFFER_SIZE, "��ӭ���ӵ�������");
                sendToClient(theSocket, welcomeMsg);

                // ʹ��inet_ntop���inet_ntoa
                char ipStr[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &client.addr.sin_addr, ipStr, INET_ADDRSTRLEN);
                printf("�ͻ���������: %s:%d\n", ipStr, ntohs(client.addr.sin_port));
            }

            // ��ʾ�ͻ�����Ϣ
            printf("�ͻ���: %s\n", szBuf);

            LeaveCriticalSection(&cs);
        }
    }
    return 0;
}

// ������
void DatagramServer(short nPort) {
    SOCKET theSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    SOCKADDR_IN saServer;
    saServer.sin_family = AF_INET;
    saServer.sin_addr.s_addr = INADDR_ANY;
    saServer.sin_port = htons(nPort);
    bind(theSocket, (LPSOCKADDR)&saServer, sizeof(saServer));

    printf("�������������������˿� %d\n", nPort);
    printf("������Ϣ���͸��ͻ��ˣ�����exit�˳�\n");

    HANDLE hThread = CreateThread(NULL, 0, ReceiveThread, (LPVOID)theSocket, 0, NULL);

    char szBuf[BUFFER_SIZE] = { 0 };

    while (1) {
        fgets(szBuf, BUFFER_SIZE - 1, stdin);

        if (strcmp(szBuf, "exit\n") == 0) break;

        // �Ƴ����з�
        szBuf[strcspn(szBuf, "\n")] = 0;

        sendToClient(theSocket, szBuf);

        // ������뻺����
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
