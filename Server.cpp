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
#include <csignal>

#pragma comment(lib, "Ws2_32.lib") // Link thư viện

#define PORT "27015" // Port cho server
#define BUFLEN 512 // Chiều dài buffer

SOCKET listenSocket = INVALID_SOCKET; // Socket chính của server, dùng để nhận các client

int balance = 1886; // Số tiền trong tài khoản, dùng để mô phỏng các giao dịch ngân hàng

// Các thông báo của server đến với client
const char withdrawSuccess[] = "Withdraw successfully!";
const char depositSuccess[] = "Deposit successfully!";
const char failure[] = "Not enough balance!";
const char loginSuccess[] = "Login successfully!";
const char loginFail[] = "Failed to login: incorrect account or password";

// Tài khoản và mật khẩu, dùng để mô phỏng đăng nhập ngân hàng
const char account[] = "myaccount";
const char password[] = "123456789";

using namespace std;

//
typedef int sem;
mutex _mutex;
sem s = 1;

//
void down() {
	unique_lock<mutex> lock(_mutex);
	while (s <= 0);
	s--;
}

//
void up() {
	unique_lock<mutex> lock(_mutex);
	s++;
}

//
void criticalSection(int amount) {
	down();
	balance += amount;
	if (amount > 0) {
		cout << "A client has just made a deposit, new account balance: " << balance << '\n';
	}
	else {
		cout << "A client has just made a withdrawal, balance left: " << balance << '\n';
	}
	up();
}

// Hàm để server quản lý một client
// Hàm này sẽ được server tạo một thread riêng để chạy và dùng để quản lý riêng một client nào đó
void handleClient(SOCKET clientSocket) {
	int iResult;
	int iSendResult;

	int recvBufLen = BUFLEN;

	char* recvAccount = new char[BUFLEN];
	char* recvPassword = new char[BUFLEN];

	// Nhận tài khoản
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

	// Nhận mật khẩu
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

	// Nếu tài khoản hay mật khẩu không đúng
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

	// Gửi thông báo đến client đã đăng nhập thành công
	iSendResult = send(clientSocket, loginSuccess, (int)(strlen(loginSuccess) + 1), 0);
	if (iSendResult == SOCKET_ERROR) {
		cout << "send failed: " << WSAGetLastError() << '\n';
		closesocket(clientSocket);
		return;
	}

	char recvBuf[BUFLEN];

	// Nhận các yêu cầu của client đến khi client ngắt kết nối
	do {
		iResult = recv(clientSocket, recvBuf, recvBufLen, 0);
		// Nếu nhận yêu cầu thành công
		if (iResult > 0) {
			int amount = atoi(recvBuf);

			for (int i = 0; i < recvBufLen; i++) {
				recvBuf[i] = '\0';
			}

			if (amount == 0) {
				string bal = to_string(balance);
				iSendResult = send(clientSocket, bal.c_str(), (int)(bal.size() + 1), 0);
			}
			else if (amount > 0) {
				criticalSection(amount);
				iSendResult = send(clientSocket, depositSuccess, (int)(strlen(depositSuccess) + 1), 0);
			}
			else {
				if (-amount > balance) {
					iSendResult = send(clientSocket, failure, (int)(strlen(failure) + 1), 0);
				}
				else {
					criticalSection(amount);
					iSendResult = send(clientSocket, withdrawSuccess, (int)(strlen(withdrawSuccess) + 1), 0);
				}
			}

			if (iSendResult == SOCKET_ERROR) {
				cout << "send failed: " << WSAGetLastError() << '\n';
				closesocket(clientSocket);
				return;
			}

		}

		// Nếu client ngắt kết nối
		else if (iResult == 0) {
			closesocket(clientSocket);
			cout << "A client has disconnected\n";
			return;
		}

		// Nếu lỗi trong lúc nhận yêu cầu
		else if (iResult < 0) {
			cout << "recv failed: " << WSAGetLastError() << '\n';
			closesocket(clientSocket);
			return;
		}

	} while (iResult > 0);

SHUT_DOWN:
	// Gửi thông điệp shutdown
	iResult = shutdown(clientSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		cout << "shutdown failed: " << WSAGetLastError() << '\n';
	}

	// Đóng socket
	closesocket(clientSocket);
	cout << "A client has disconnected\n";
}

// Xử lý shutdown server thông qua interrupt Ctrl C
void handleInterruptSignal(int signal) {
	closesocket(listenSocket);
	WSACleanup();
	exit(EXIT_SUCCESS);
}

int main() {
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

	// Lấy thông tin địa chỉ
	iResult = getaddrinfo(NULL, PORT, &hints, &result);
	if (iResult != 0) {
		cout << "getaddrinfo failed: " << iResult << '\n';
		WSACleanup();
		return -1;
	}

	// Tạo một socket để nghe các client
	listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (listenSocket == INVALID_SOCKET) {
		cout << "Error at socket(): " << WSAGetLastError() << '\n';
		freeaddrinfo(result);
		WSACleanup();
		return -1;
	}

	// Bind socket với địa chỉ
	iResult = bind(listenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		cout << "bind failed with error: " << WSAGetLastError() << '\n';
		freeaddrinfo(result);
		closesocket(listenSocket);
		WSACleanup();
		return -1;
	}

	freeaddrinfo(result);

	if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
		cout << "Listen failed with error: " << WSAGetLastError() << '\n';
		closesocket(listenSocket);
		WSACleanup();
		return -1;
	}

	// Socket của một tiến trình khách
	SOCKET clientSocket = INVALID_SOCKET;

	// Bắt interrupt Ctrl C
	signal(SIGINT, handleInterruptSignal);

	cout << "Press Ctrl + C to turn off the server\n";
	cout << "Balance left: " << balance << '\n';
	while (true) {
		// Chờ client kết nối
		clientSocket = accept(listenSocket, NULL, NULL);

		Sleep(1);

		if (clientSocket == INVALID_SOCKET) {
			cout << "accept failed: " << WSAGetLastError() << '\n';
			closesocket(listenSocket);
			WSACleanup();
			return -1;
		}

		// Giao socket của tiến trình khách này cho một thread riêng
		thread clientThread(handleClient, clientSocket);
		clientThread.detach();
		cout << "A client has connected\n";
	}

	closesocket(listenSocket);
	WSACleanup();

	return 0;
}
