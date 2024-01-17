#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <algorithm>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

class Client {
public:
    Client(int socket, const std::string& name, const std::string& roomName) : socket(socket), name(name), roomName(roomName) {}

    int getSocket() const {
        return socket;
    }

    const std::string& getName() const {
        return name;
    }

    const std::string& getRoomName() const {
        return roomName;
    }

private:
    int socket;
    std::string name;
    std::string roomName;
};

class Server {
public:
    Server() {
        serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket == -1) {
            std::cerr << "Can't create the socket" << std::endl;
            return;
        }

        sockaddr_in hint;
        hint.sin_family = AF_INET;
        hint.sin_port = htons(54000);
        hint.sin_addr.s_addr = INADDR_ANY;

        if (bind(serverSocket, (sockaddr*)&hint, sizeof(hint)) == -1) {
            std::cerr << "Blind failed!" << std::endl;
            close(serverSocket);
            return;
        }

        if (listen(serverSocket, SOMAXCONN) == -1) {
            std::cerr << "Listen failed!" << std::endl;
            close(serverSocket);
            return;
        }
    }

    void start() {
        while (true) {
            sockaddr_in clientAddr;
            socklen_t clientSize = sizeof(clientAddr);
            int clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientSize);
            if (clientSocket == -1) {
                std::cerr << "Error when create the socket form client" << std::endl;
                continue;
            }

            char buffer[4096];
            int bytesReceived = recv(clientSocket, buffer, 4096, 0);
            if (bytesReceived <= 0) {
                std::cerr << "Error:Recieved the name of clients failed!" << std::endl;
                close(clientSocket);
                continue;
            }
            std::string roomName = std::string(buffer, 0, bytesReceived);
            bytesReceived = recv(clientSocket, buffer, 4096, 0);
            if (bytesReceived <= 0) {
                std::cerr << "Error:Recieved the name of clients failed!" << std::endl;
                close(clientSocket);
                continue;
            }
            std::string clientName = std::string(buffer, 0, bytesReceived);

            std::lock_guard<std::mutex> guard(clientsMutex);
            clients.push_back(Client(clientSocket, clientName, roomName));

            std::thread clientThread(&Server::handleClient, this, clientSocket, clientName, roomName);
            clientThread.detach();
        }
    }

private:
    int serverSocket;
    std::vector<Client> clients;
    std::mutex clientsMutex;

    void handleClient(int clientSocket, const std::string& clientName, const std::string& roomName) {
        char buffer[4096];
        int bytesReceived;

        while (true) {
            bytesReceived = recv(clientSocket, buffer, 4096, 0);
            if (bytesReceived <= 0) {
                std::cerr << "Client " << clientName << " offline!" << std::endl;
                std::lock_guard<std::mutex> guard(clientsMutex);
                clients.erase(std::remove_if(clients.begin(), clients.end(), [clientSocket](const Client& client) {
                    return client.getSocket() == clientSocket;
                }), clients.end());
                close(clientSocket);
                return;
            }

            std::string message = clientName + ": " + std::string(buffer, 0, bytesReceived);
            std::lock_guard<std::mutex> guard(clientsMutex);
            for (const auto& client : clients) {
                if (client.getRoomName() == roomName && client.getSocket() != clientSocket) {
                    send(client.getSocket(), message.c_str(), message.size() + 1, 0);
                }
            }
        }
    }
};

int main() {
    Server server;
    server.start();
    return 0;
}
