#undef UNICODE
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <iostream>
#include <string>

// Khắc phục lỗi linker
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

//
#define PORT "27015"
#define ADDR "127.0.0.1"

//
#define BUFLEN 512

//
char recvBuf[BUFLEN];
int recvBufLen = BUFLEN;

const char loginFail[] = "Failed to login: incorrect account or password\n";

using namespace std;

int main() {
    WSADATA wsaData;
    addrinfo* result = nullptr, * ptr = nullptr, hints;
    int iResult, iSendResult;

    //
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        cout << "WSAStartup failed with error: " << iResult << '\n';
        return -1;
    }

    //
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    //
    iResult = getaddrinfo(ADDR, PORT, &hints, &result);
    if (iResult != 0) {
        cout << "getaddrinfo failed with error: " << iResult << '\n';
        WSACleanup();
        return -1;
    }

    SOCKET connectSocket = INVALID_SOCKET;
    //
    for (ptr = result; ptr != nullptr; ptr = ptr->ai_next) {
        //
        connectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (connectSocket == INVALID_SOCKET) {
            cout << "socket failed with error: " << WSAGetLastError() << '\n';
            WSACleanup();
            return -1;
        }

        //
        iResult = connect(connectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(connectSocket);
            connectSocket = INVALID_SOCKET;
            continue;
        }

        break;
    }

    freeaddrinfo(result);

    if (connectSocket == INVALID_SOCKET) {
        cout << "Unable to connect to the server!\n";
        WSACleanup();
        return -1;
    }

    string account, password;
    cout << "Enter account: ";
    getline(cin, account);
    cout << "Enter password: ";
    getline(cin, password);

    iSendResult = send(connectSocket, account.c_str(), (int)(account.size() + 1), 0);
    if (iSendResult == SOCKET_ERROR) {
        cout << "send failed with error: " << WSAGetLastError() << '\n';
        closesocket(connectSocket);
        WSACleanup();
        return -1;
    }

    iSendResult = send(connectSocket, password.c_str(), (int)(password.size() + 1), 0);
    if (iSendResult == SOCKET_ERROR) {
        cout << "send failed with error: " << WSAGetLastError() << '\n';
        closesocket(connectSocket);
        WSACleanup();
        return -1;
    }

    iResult = recv(connectSocket, recvBuf, recvBufLen, 0);
    if (iResult == SOCKET_ERROR) {
        cout << "recv failed with error: " << WSAGetLastError() << '\n';
        closesocket(connectSocket);
        WSACleanup();
        return -1;
    }

    if (strcmp(recvBuf, loginFail) == 0) {
        cout << recvBuf << '\n';
        goto SHUT_DOWN;
    }

    int amount;
    do {
        cout << "Enter the amount of money you want to deposit (Enter 0 to check the balance, enter a negative number to exit): ";
        cin >> amount;
        if (amount < 0) {
            break;
        }

        string sendBuf = to_string(amount);
        iSendResult = send(connectSocket, sendBuf.c_str(), (int)(sendBuf.size() + 1), 0);
        if (iSendResult == SOCKET_ERROR) {
            cout << "send failed with error: " << WSAGetLastError() << '\n';
            closesocket(connectSocket);
            WSACleanup();
            return -1;
        }

        iResult = recv(connectSocket, recvBuf, recvBufLen, 0);
        if (iResult > 0) {
            if (amount == 0) {
                cout << "Your balance: ";
            }
            cout << recvBuf << '\n';
        }
        else if (iResult == 0) {
            cout << "Connection closed\n";
            closesocket(connectSocket);
            WSACleanup();
            return 0;
        }
        else {
            cout << "recv failed with error: " << WSAGetLastError() << '\n';
            closesocket(connectSocket);
            WSACleanup();
            return -1;
        }
    } while (iResult > 0);

SHUT_DOWN:
    //
    iResult = shutdown(connectSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        cout << "shutdown failed with error: " << WSAGetLastError() << '\n';
        closesocket(connectSocket);
        WSACleanup();
        return -1;
    }

    //
    do {
        iResult = recv(connectSocket, recvBuf, recvBufLen, 0);
        if (iResult > 0) {
            cout << recvBuf << '\n';
        }
        else if (iResult == 0) {
            cout << "Connection closed\n";
        }
        else {
            cout << "recv failed with error: " << WSAGetLastError() << '\n';
        }
    } while (iResult > 0);

    closesocket(connectSocket);
    WSACleanup();

    system("pause");

    return 0;
}
