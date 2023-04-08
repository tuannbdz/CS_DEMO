#undef UNICODE
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <iostream>
#include <thread>
#include <cstring>
#include <mutex>
// Khắc phục lỗi linker
#pragma comment(lib, "Ws2_32.lib")
// Port cho server
#define DEFAULT_PORT "27015"

// Chiều dài buffer
#define DEFAULT_BUFLEN 512

typedef int sem;
std::mutex _mutex;
sem s = 1;
void down() {
	std::unique_lock<std::mutex> lock(_mutex);
	while (s <= 0);
	s--;
}

void up() {
	std::unique_lock<std::mutex> lock(_mutex);
	s++;
}

int balance = 1886;
//
void CS(int depositAmount) {
	down();
	balance -= depositAmount;
	std::cout << "Balance left: " << balance << '\n';
	up();
}
const char success[] = "Deposit successfully!\n";
const char failure[] = "Not enough balance!\n";

using namespace std;

void handleClient(SOCKET ClientSocket) {
	int iResult;
	int iSendResult;

	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN;

	do {
		iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
		if (iResult > 0) {
			int depositAmount = atoi(recvbuf);
			for (int i = 0; i < DEFAULT_BUFLEN; ++i) {
				recvbuf[i] = '\0';
			}
			if (depositAmount < 0) {
				break;
			}

			if (depositAmount > balance) {
				iSendResult = send(ClientSocket, failure, (int)strlen(failure), 0);
			}
			else {
				CS(depositAmount);
				iSendResult = send(ClientSocket, success, (int)strlen(success), 0);
			}
			if (iSendResult == SOCKET_ERROR) {
				cout << "send failed: " << WSAGetLastError() << '\n';
				closesocket(ClientSocket);
				return;
			}

			if (balance <= 0) {
				break;
			}
		}
		else if (iResult < 0) {
			cout << "recv failed: " << WSAGetLastError() << '\n';
			closesocket(ClientSocket);
			return;
		}
	} while (iResult > 0);

	iResult = shutdown(ClientSocket, SD_SEND);

	if (iResult == SOCKET_ERROR) {
		cout << "shutdown failed: " << WSAGetLastError() << '\n';
	}

	closesocket(ClientSocket);
}

int main() {
	// Các biến để thiết lập socket
	WSADATA wsaData;
	addrinfo* result = nullptr, * ptr = nullptr, hints;
	int iResult;

	// Khởi tạo Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		cout << "WSAStartup failed: " << iResult << '\n';
		return -1;
	}

	// Thiết lập các thông tin của Socket
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// 
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		cout << "getaddrinfo failed: " << iResult << '\n';
		WSACleanup();
		return -1;
	}

	//
	SOCKET ListenSocket = INVALID_SOCKET;

	// Tạo một socket để nghe các client
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

	// 
	if (ListenSocket == INVALID_SOCKET) {
		cout << "Error at socket(): " << WSAGetLastError() << '\n';
		freeaddrinfo(result);
		WSACleanup();
		return -1;
	}

	// 
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		cout << "bind failed with error: " << WSAGetLastError() << '\n';
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return -1;
	}

	// 
	freeaddrinfo(result);

	//
	if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR) {
		cout << "Listen failed with error: " << WSAGetLastError() << '\n';
		closesocket(ListenSocket);
		WSACleanup();
		return -1;
	}

	//
	SOCKET clientSocket = INVALID_SOCKET;

	while (balance > 0) {
		clientSocket = accept(ListenSocket, NULL, NULL);

		if (clientSocket == INVALID_SOCKET) {
			cout << "accept failed: " << WSAGetLastError() << '\n';
			closesocket(ListenSocket);
			WSACleanup();
			return -1;
		}

		thread clientThread(handleClient, clientSocket);
		clientThread.detach();
	}

	WSACleanup();

	return 0;
}