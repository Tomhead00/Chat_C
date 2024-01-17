#include <iostream>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <cstring>
#include <thread>
#include <map>
#include <fstream>
#include <mutex>
#include <vector>
#include <algorithm>

using namespace std;

void start();
void processCase();

class UserAuthentication
{
private:
    string username;
    string password;

public:
    UserAuthentication() : username(""), password("") {}

    bool isUserRegistered(const string &checkUsername)
    {

        ifstream inputFile("data\\users.txt");
        string username;
        while (getline(inputFile, username))
        {
            // Check if the current line is the username
            if (username == checkUsername)
            {
                inputFile.close();
                return true; // User is already registered
            }

            // Skip the next line (password)
            if (!getline(inputFile, username))
            {
                break; // Break if unable to read the next line (no password found)
            }
        }

        inputFile.close();
        return false; // User is not registered
    }

    bool registerUser(const string &enteredUsername, const string &enteredPassword)
    {
        username = enteredUsername;
        password = enteredPassword;

        ofstream myFile("data\\users.txt", ios::app); // Open the file to continue writing to the last file
        if (myFile.is_open())
        {
            myFile << username << endl
                   << password << endl;
            myFile.close();
            return true;
        }
        else
        {
            cerr << "Unable to open file" << endl;
            return false;
        }
    }

    bool isLoggedIn(const string &enteredUsername, const string &enteredPassword)
    {
        string storedUsername, storedPassword;

        ifstream inputFile("data\\users.txt");
        if (inputFile.is_open())
        {
            while (getline(inputFile, storedUsername))
            {
                getline(inputFile, storedPassword);

                if (storedUsername == enteredUsername && storedPassword == enteredPassword)
                {
                    inputFile.close();
                    return true;
                }
            }
            inputFile.close();
        }
        return false;
    }
};

class Client
{
public:
    Client(int socket, const std::string &name, const std::string &roomName) : socket(socket), name(name), roomName(roomName) {}

    int getSocket() const
    {
        return socket;
    }

    const std::string &getName() const
    {
        return name;
    }

    const std::string &getRoomName() const
    {
        return roomName;
    }

private:
    int socket;
    std::string name;
    std::string roomName;
};

class listClient
{
public:
    static vector<Client> clients;
};

class TCPServer
{
private:
    int listening;
    int clientSocket;
    UserAuthentication auth;

private:
    void handleRegistration(int clientSocket)
    {
        char usernameBuffer[1024] = {0};
        char passwordBuffer[1024] = {0};

        recv(clientSocket, usernameBuffer, sizeof(usernameBuffer), 0);
        recv(clientSocket, passwordBuffer, sizeof(passwordBuffer), 0);

        string username(usernameBuffer);
        string password(passwordBuffer);

        bool status = auth.isUserRegistered(username);
        status = !status && auth.registerUser(username, password);
        const char *response;
        if (status)
        {
            response = "Registration successful.";
        }
        else
        {
            response = "Username already exists. Choose a different username.";
        }
        send(clientSocket, response, strlen(response), 0);
    }

    void handleLogin(int clientSocket)
    {
        char usernameBuffer[1024] = {0};
        char passwordBuffer[1024] = {0};

        read(clientSocket, usernameBuffer, 1024);
        read(clientSocket, passwordBuffer, 1024);

        string username(usernameBuffer);
        string password(passwordBuffer);

        bool status = auth.isLoggedIn(username, password);
        const char *response = status ? "Login successful." : "Login failed.";
        send(clientSocket, response, strlen(response), 0);
    }

public:
    TCPServer() : listening(-1), clientSocket(-1) {}

    bool initServer(int port)
    {
        listening = socket(AF_INET, SOCK_STREAM, 0);
        if (listening == -1)
        {
            cerr << "Unable to create socket! Cancel..." << endl;
            return false;
        }

        sockaddr_in hint;
        hint.sin_family = AF_INET;
        hint.sin_port = htons(port);
        inet_pton(AF_INET, "0.0.0.0", &hint.sin_addr);

        if (bind(listening, reinterpret_cast<sockaddr *>(&hint), sizeof(hint)) == -1)
        {
            cerr << "Binding failed..." << endl;
            return false;
        }

        if (listen(listening, SOMAXCONN) == -1)
        {
            cerr << "Listening failed..." << endl;
            return false;
        }

        return true;
    }

    void chatRoom()
    {
        // while (true) {
        // Receive room name from client
        cout << "accepted" << endl;
        std::string roomName = receiveString(clientSocket);
        cout << "Room name is " << roomName << endl;
        if (roomName.empty())
        {
            close(clientSocket);
            // continue;
        }

        // Receive client name from client
        std::string clientName = receiveString(clientSocket);
        cout << "Client name is " << clientName << endl;
        if (clientName.empty())
        {
            close(clientSocket);
            // continue;
        }

        std::lock_guard<std::mutex> guard(clientsMutex);
        listClient::clients.push_back(Client(clientSocket, clientName, roomName));

        std::thread clientThread(&TCPServer::handleRoom, this, clientSocket, clientName, roomName);
        clientThread.detach();
        // }
    }

    void receiveData()
    {
        char buffer[4096];
        while (true)
        {
            int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
            if (bytesReceived == -1)
            {
                cerr << "Error while receiving data!" << endl;
                break;
            }
            if (bytesReceived == 0)
            {
                cout << "Client has disconnected" << endl;
                break;
            }
            cout << "\nClient: " << string(buffer, 0, bytesReceived) << endl;
        }
    }

    void sendData(const string &data)
    {
        send(clientSocket, data.c_str(), data.size() + 1, 0);
    }

    void handleClient(int clientSocket)
    {
        char buffer[4096];
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        int option = stoi(string(buffer, 0, bytesReceived));

        if (option == 1)
        {
            // Register
            handleRegistration(clientSocket);
        }
        else if (option == 2)
        {
            // Login
            handleLogin(clientSocket);
        }
    }

    void closeConnection()
    {
        close(clientSocket);
        close(listening);
    }

private:
    // vector<Client> listClient::clients;
    std::mutex clientsMutex;

    std::string receiveString(int clientSocket)
    {
        char buffer[4096];
        int bytesReceived = recv(clientSocket, buffer, 4096, 0);
        if (bytesReceived <= 0)
        {
            std::cerr << "Error: Received data from client failed!" << std::endl;
            return "";
        }
        return std::string(buffer, 0, bytesReceived);
    }

    void handleRoom(int clientSocket, const std::string &clientName, const std::string &roomName)
    {
        char buffer[4096];
        int bytesReceived;

        while (true)
        {
            bytesReceived = recv(clientSocket, buffer, 4096, 0);
            if (bytesReceived <= 0)
            {
                std::cerr << "Client " << clientName << " offline!" << std::endl;
                std::lock_guard<std::mutex> guard(clientsMutex);
                listClient::clients.erase(std::remove_if(listClient::clients.begin(), listClient::clients.end(), [clientSocket](const Client &client)
                                                         { return client.getSocket() == clientSocket; }),
                                          listClient::clients.end());
                close(clientSocket);
                return;
            }

            std::string message = clientName + ": " + std::string(buffer, 0, bytesReceived);
            std::lock_guard<std::mutex> guard(clientsMutex);
            for (const auto &client : listClient::clients)
            {
                if (client.getRoomName() == roomName && client.getSocket() != clientSocket)
                {
                    send(client.getSocket(), message.c_str(), message.size() + 1, 0);
                }
            }
        }
    }

private:
    void displayClientInfo(const sockaddr_in &client)
    {
        char host[NI_MAXHOST];
        char service[NI_MAXSERV];

        memset(host, 0, NI_MAXHOST);
        memset(service, 0, NI_MAXSERV);

        if (getnameinfo(reinterpret_cast<const sockaddr *>(&client), sizeof(client),
                        host, NI_MAXHOST, service, NI_MAXSERV, 0))
        {
            cout << host << " connected on port " << service << endl;
        }
        else
        {
            inet_ntop(AF_INET, &client.sin_addr, host, NI_MAXHOST);
            cout << host << " connected on port " << ntohs(client.sin_port) << endl;
        }
    }
};

int main()
{
    start();
    return 0;
}

void start()
{
    sockaddr_in client;
    socklen_t clientSize = sizeof(client);

    clientSocket = accept(listening, reinterpret_cast<sockaddr *>(&client), &clientSize);

    displayClientInfo(client);
    processCase();
}

void processCase()
{
    char buffer[4096];

    TCPServer server;
    if (!server.initServer(54000))
    {
        cerr << "Server initialization failed!" << endl;
        return -1;
    }
    while (true)
    {
        cout << "debug" << endl;
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);

        if (bytesReceived == -1)
        {
            cerr << "Error while receiving data!" << endl;
            return;
        }
        if (bytesReceived == 0)
        {
            cout << "Client has disconnected" << endl;
            return;
        }

        int choice = stoi(string(buffer, 0, bytesReceived));
        // string userInput;
        cout << "Client selected option " << choice << endl;
        switch (choice)
        {
        case 1:
        {
            thread receivingThread(&TCPServer::receiveData, this);
            while (true)
            {
                string userInput;
                cout << "You: ";
                getline(cin, userInput);
                sendData(userInput);
            }
            break;
        }
        case 2:
        {
            if (clientSocket < 0)
            {
                cerr << "Error in accepting connection." << endl;
                // continue;
            }
            cout << "New client connected." << endl;
            handleClient(clientSocket);
            break;
        }
        case 3:
        {
            // while(true)
            // {
            //     // thread roomThread(&TCPServer::chatRoom, this);
            //     // Receive room name from client
            //     cout <<"accepted" << endl;
            //     std::string roomName = receiveString(clientSocket);
            //     cout << "Room name is " << roomName << endl;
            //     if (roomName.empty()) {
            //         close(clientSocket);
            //         continue;
            //     }

            //     // Receive client name from client
            //     std::string clientName = receiveString(clientSocket);
            //     cout << "Client name is " << clientName << endl;
            //     if (clientName.empty()) {
            //         close(clientSocket);
            //         continue;
            //     }

            //     std::lock_guard<std::mutex> guard(clientsMutex);
            //     clients.push_back(Client(clientSocket, clientName, roomName));

            //     std::thread clientThread(&TCPServer::handleRoom, this, clientSocket, clientName, roomName);
            //     clientThread.detach();
            // }
            chatRoom();
            break;
        }
        default:
            return;
        }
    }
}
