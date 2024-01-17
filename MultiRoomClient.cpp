#include <iostream>
#include <thread>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

class Client {
private:
    int sock;
    std::thread receivingThread;

public:
    Client() {
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == -1) {
            std::cerr << "Error: Create the socket" << std::endl;
            return;
        }
    }

    ~Client() {
        close(sock);
        if (receivingThread.joinable()) {
            receivingThread.join();
        }
    }

    bool connectToServer(const std::string& ipAddress, int port, const std::string& roomName) {
        sockaddr_in hint;
        hint.sin_family = AF_INET;
        hint.sin_port = htons(port);
        inet_pton(AF_INET, ipAddress.c_str(), &hint.sin_addr);

        int connectRes = connect(sock, (sockaddr*)&hint, sizeof(hint));
        if (connectRes == -1) {
            std::cerr << "Error: Cannot connect to Server!" << std::endl;
            return false;
        }

        // Gửi tên phòng tới server khi kết nối
        send(sock, roomName.c_str(), roomName.size() + 1, 0);

        return true;
    }

    void startReceiving() {
        receivingThread = std::thread([this]() {
            char buffer[4096];
            while (true) {
                
                int bytesReceived = recv(sock, buffer, 4096, 0);
                if (bytesReceived <= 0) {
                    std::cerr << "Error: Server is down" << std::endl;
                    close(sock);
                    break;
                }
                std::cout << std::string(buffer, 0, bytesReceived) << std::endl;
            }
        });
    }

    void sendToServer(const std::string& message) {
        send(sock, message.c_str(), message.size() + 1, 0);
    }
};

int main() {
    std::string roomName;
    std::cout << "Enter the Room: ";
    std::getline(std::cin, roomName);

    Client client;

    if (client.connectToServer("10.188.9.20", 54000, roomName)) {
        client.startReceiving();

        std::string clientName;
        std::cout << "Enter Your name: ";
        std::getline(std::cin, clientName);
        client.sendToServer(clientName);

        while (true) {
            std::string userInput;
            std::getline(std::cin, userInput);
            client.sendToServer(userInput);
        }
    }

    return 0;
}

