#undef UNICODE
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <iostream>
#include <string>

// Link với thư viện
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define PORT "27015" // Port của server
#define ADDR "127.0.0.1" // Địa chỉ IP của server (Đây là địa chỉ loopback, nghĩa là địa chỉ của chính máy tính đang chạy tiến trình này, vì vậy cần chạy server và client trên cùng một máy)

#define BUFLEN 512 // Chiều dài buffer

char recvBuf[BUFLEN]; // Buffer của client
int recvBufLen = BUFLEN;

const char loginFail[] = "Failed to login: incorrect account or password"; // Thông điệp server sẽ gửi về nếu đăng nhập thất bại

using namespace std;

int main() {
    WSADATA wsaData;
    addrinfo* result = nullptr, * ptr = nullptr, hints;
    int iResult, iSendResult;

    // Khởi tạo Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        cout << "WSAStartup failed with error: " << iResult << '\n';
        return -1;
    }

    // Thiết lập các thông tin của Socket
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Lấy thông tin địa chỉ
    iResult = getaddrinfo(ADDR, PORT, &hints, &result);
    if (iResult != 0) {
        cout << "getaddrinfo failed with error: " << iResult << '\n';
        WSACleanup();
        return -1;
    }

    SOCKET connectSocket = INVALID_SOCKET; // Cổng socket của client kết nối đến server

    // Kết nối với các địa chỉ lấy được ở trên
    for (ptr = result; ptr != nullptr; ptr = ptr->ai_next) {
        connectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (connectSocket == INVALID_SOCKET) {
            cout << "socket failed with error: " << WSAGetLastError() << '\n';
            WSACleanup();
            return -1;
        }

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

    // Nhập tài khoản và mật khẩu
    string account, password;
    cout << "Enter account: ";
    getline(cin, account);
    cout << "Enter password: ";
    getline(cin, password);

    // Gửi thông tin tài khoản đến server
    iSendResult = send(connectSocket, account.c_str(), (int)(account.size() + 1), 0);
    if (iSendResult == SOCKET_ERROR) {
        cout << "send failed with error: " << WSAGetLastError() << '\n';
        closesocket(connectSocket);
        WSACleanup();
        return -1;
    }

    // Gửi thông tin mật khẩu đến server
    iSendResult = send(connectSocket, password.c_str(), (int)(password.size() + 1), 0);
    if (iSendResult == SOCKET_ERROR) {
        cout << "send failed with error: " << WSAGetLastError() << '\n';
        closesocket(connectSocket);
        WSACleanup();
        return -1;
    }

    // Nhận lại thông báo đăng nhập của server
    iResult = recv(connectSocket, recvBuf, recvBufLen, 0);
    if (iResult == SOCKET_ERROR) {
        cout << "recv failed with error: " << WSAGetLastError() << '\n';
        closesocket(connectSocket);
        WSACleanup();
        return -1;
    }

    cout << recvBuf << '\n';
    // Nếu đăng nhập thất bại
    if (strcmp(recvBuf, loginFail) == 0) {
        goto SHUT_DOWN;
    }

    // Mô phỏng quá trình rút hoặc nạp tiền
    int mode, amount;
    do {
        cout << "Enter 0 to check the account balance, 1 to make a deposit, 2 to make a withdrawal, or any other number to exit: ";
        cin >> mode;

        if (mode < 0 || mode > 2) {
            break;
        }

        amount = 0;
        if (mode == 1 || mode == 2) {
            while (true) {
                cout << "Enter the amount of money (enter a non-negative number): ";
                cin >> amount;
                if (amount <= 0) {
                    cout << "Invalid amount! Please enter a non-negative number\n";
                }
                else {
                    break;
                }
            }
        }

        if (mode == 2) {
            amount = -amount;
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
    // Shutdown kết nối
    iResult = shutdown(connectSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        cout << "shutdown failed with error: " << WSAGetLastError() << '\n';
        closesocket(connectSocket);
        WSACleanup();
        return -1;
    }

    // Nhận thông điệp cuối cùng của server sau khi gửi thông điệp shutdown
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

    // Đóng socket
    closesocket(connectSocket);
    WSACleanup();

    system("pause");

    return 0;
}
