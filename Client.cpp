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
#define DEFAULT_PORT "27015"
#define ADDR "127.0.0.1"

//
#define DEFAULT_BUFLEN 512

//
char recvbuf[DEFAULT_BUFLEN];
int recvbuflen = DEFAULT_BUFLEN;

using namespace std;

int main() {
    WSADATA wsaData;
    addrinfo* result = NULL, * ptr = NULL, hints;
    int iResult;

    //
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    //
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    //
    iResult = getaddrinfo(ADDR, DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return -1;
    }

    SOCKET connectSocket = INVALID_SOCKET;
    // Attempt to connect to an address until one succeeds
    for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {
        // Create a SOCKET for connecting to server
        connectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (connectSocket == INVALID_SOCKET) {
            printf("socket failed with error: %ld\n", WSAGetLastError());
            WSACleanup();
            return -1;
        }

        // Connect to server.
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
        printf("Unable to connect to server!\n");
        WSACleanup();
        return -1;
    }

    do {
        cout << "Enter the amount of money you want to deposit (Enter a non-positive number to exit): ";
        int amount;
        cin >> amount;
        if (amount <= 0) {
            break;
        }

        string sendbuf = to_string(amount);
        cout << sendbuf << '\n';
        iResult = send(connectSocket, sendbuf.c_str(), (int)sendbuf.size(), 0);
        if (iResult == SOCKET_ERROR) {
            printf("send failed with error: %d\n", WSAGetLastError());
            closesocket(connectSocket);
            WSACleanup();
            return -1;
        }

        iResult = recv(connectSocket, recvbuf, recvbuflen, 0);
        if (iResult > 0) {
            cout << recvbuf << '\n';
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

    //
    iResult = shutdown(connectSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(connectSocket);
        WSACleanup();
        return -1;
    }

    //
    do {
        iResult = recv(connectSocket, recvbuf, recvbuflen, 0);
        if (iResult > 0)
            printf("Bytes received: %d\n", iResult);
        else if (iResult == 0)
            printf("Connection closed\n");
        else
            printf("recv failed with error: %d\n", WSAGetLastError());
    } while (iResult > 0);

    // cleanup
    closesocket(connectSocket);
    WSACleanup();

    return 0;
}