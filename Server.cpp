#undef UNICODE
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <iostream>
#include <thread>
#include <string>
#include <cstring>
#include <mutex>

// Khắc phục lỗi linker
#pragma comment(lib, "Ws2_32.lib")

// Port cho server
#define PORT "27015"

// Chiều dài buffer
#define BUFLEN 512

int balance = 1886;

const char success[] = "Deposit successfully!\n";
const char failure[] = "Not enough balance!\n";
const char loginSuccess[] = "Login successfully!\n";
const char loginFail[] = "Failed to login: incorrect account or password\n";
const char account[] = "myaccount";
const char password[] = "123456789";

using namespace std;

typedef int sem;
mutex _mutex;
sem s = 1;

void down() {
	unique_lock<mutex> lock(_mutex);
	while (s <= 0);
	s--;
}

void up() {
	unique_lock<mutex> lock(_mutex);
	s++;
}

void criticalSection(int depositAmount) {
	down();
	balance -= depositAmount;
	cout << "A client has just made a deposit, balance left: " << balance << '\n';
	up();
}

void handleClient(SOCKET clientSocket) {
	int iResult;
	int iSendResult;

	int recvBufLen = BUFLEN;

	char* recvAccount = new char[BUFLEN];
	char* recvPassword = new char[BUFLEN];

	iResult = recv(clientSocket, recvAccount, recvBufLen, 0);
	if (iResult < 0) {
		cout << "recv failed: " << WSAGetLastError() << '\n';
		closesocket(clientSocket);
		delete[] recvAccount;
		recvAccount = nullptr;
		delete[] recvPassword;
		recvPassword = nullptr;
		return;
	}

	iResult = recv(clientSocket, recvPassword, recvBufLen, 0);
	if (iResult < 0) {
		cout << "recv failed: " << WSAGetLastError() << '\n';
		closesocket(clientSocket);
		delete[] recvAccount;
		recvAccount = nullptr;
		delete[] recvPassword;
		recvPassword = nullptr;
		return;
	}

	if (strcmp(recvAccount, account) != 0 || strcmp(recvPassword, password) != 0) {
		iSendResult = send(clientSocket, loginFail, (int)(strlen(loginFail) + 1), 0);

		if (iSendResult == SOCKET_ERROR) {
			cout << "send failed: " << WSAGetLastError() << '\n';
			closesocket(clientSocket);
			delete[] recvAccount;
			recvAccount = nullptr;
			delete[] recvPassword;
			recvPassword = nullptr;
			return;
		}

		delete[] recvAccount;
		recvAccount = nullptr;
		delete[] recvPassword;
		recvPassword = nullptr;

		goto SHUT_DOWN;
	}

	delete[] recvAccount;
	recvAccount = nullptr;
	delete[] recvPassword;
	recvPassword = nullptr;

	iSendResult = send(clientSocket, loginSuccess, (int)(strlen(loginSuccess) + 1), 0);
	if (iSendResult == SOCKET_ERROR) {
		cout << "send failed: " << WSAGetLastError() << '\n';
		closesocket(clientSocket);
		return;
	}

	char recvBuf[BUFLEN];

	do {
		iResult = recv(clientSocket, recvBuf, recvBufLen, 0);
		if (iResult > 0) {
			int depositAmount = atoi(recvBuf);

			for (int i = 0; i < recvBufLen; i++) {
				recvBuf[i] = '\0';
			}

			if (depositAmount == 0) {
				string bal = to_string(balance);
				iSendResult = send(clientSocket, bal.c_str(), (int)(bal.size() + 1), 0);
			}
			else if (depositAmount > balance) {
				iSendResult = send(clientSocket, failure, (int)(strlen(failure) + 1), 0);
			}
			else {
				criticalSection(depositAmount);
				iSendResult = send(clientSocket, success, (int)(strlen(success) + 1), 0);
			}

			if (iSendResult == SOCKET_ERROR) {
				cout << "send failed: " << WSAGetLastError() << '\n';
				closesocket(clientSocket);
				return;
			}

		}
		else if (iResult == 0) {
			closesocket(clientSocket);
			cout << "A client has disconnected\n";
			return;
		}
		else if (iResult < 0) {
			cout << "recv failed: " << WSAGetLastError() << '\n';
			closesocket(clientSocket);
			return;
		}

	} while (iResult > 0);

SHUT_DOWN:
	iResult = shutdown(clientSocket, SD_SEND);

	if (iResult == SOCKET_ERROR) {
		cout << "shutdown failed: " << WSAGetLastError() << '\n';
	}

	closesocket(clientSocket);
	cout << "A client has disconnected\n";
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
	iResult = getaddrinfo(NULL, PORT, &hints, &result);
	if (iResult != 0) {
		cout << "getaddrinfo failed: " << iResult << '\n';
		WSACleanup();
		return -1;
	}

	//
	SOCKET listenSocket = INVALID_SOCKET;

	// Tạo một socket để nghe các client
	listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

	// 
	if (listenSocket == INVALID_SOCKET) {
		cout << "Error at socket(): " << WSAGetLastError() << '\n';
		freeaddrinfo(result);
		WSACleanup();
		return -1;
	}

	// 
	iResult = bind(listenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		cout << "bind failed with error: " << WSAGetLastError() << '\n';
		freeaddrinfo(result);
		closesocket(listenSocket);
		WSACleanup();
		return -1;
	}

	// 
	freeaddrinfo(result);

	//
	if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
		cout << "Listen failed with error: " << WSAGetLastError() << '\n';
		closesocket(listenSocket);
		WSACleanup();
		return -1;
	}

	//
	SOCKET clientSocket = INVALID_SOCKET;

	cout << "Balance left: " << balance << '\n';
	while (true) {
		clientSocket = accept(listenSocket, NULL, NULL);

		if (clientSocket == INVALID_SOCKET) {
			cout << "accept failed: " << WSAGetLastError() << '\n';
			closesocket(listenSocket);
			WSACleanup();
			return -1;
		}

		thread clientThread(handleClient, clientSocket);
		clientThread.detach();
		cout << "A client has connected\n";
	}

	closesocket(listenSocket);
	WSACleanup();

	return 0;
}
