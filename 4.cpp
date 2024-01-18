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
#include <mutex>
#include <algorithm>

using namespace std;

const int PORT = 4444;
const string IP_SERVER = "10.188.9.19";

class Client {
private:
    int clientSocket;
    thread receivingThread;

    void receiveData() {
        char buffer[4096];
        while (true) {
            int bytesReceived = recv(clientSocket, buffer, 4096, 0);
            if (bytesReceived == -1) {
                cerr << "Error while receiving data!" << endl;
                break;
            }
            if (bytesReceived == 0) {
                cout << "Server disconnected" << endl;
                break;
            }
            cout << "\nServer: " << string(buffer, 0, bytesReceived) << endl;
        }
    }



public:
    Client() {
        clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (clientSocket == -1) {
            cerr << "Unable to create socket! Cancel..." << endl;
            // Handle error accordingly
        }
    }

    // bool connectToServer(const string& ipAddress, int port) {
    //     sockaddr_in hint;
    //     hint.sin_family = AF_INET;
    //     hint.sin_port = htons(port);
    //     inet_pton(AF_INET, ipAddress.c_str(), &hint.sin_addr);

    //     int connectRes = connect(clientSocket, (sockaddr *)&hint, sizeof(hint));
    //     if (connectRes == -1) {
    //         cerr << "Connection to server failed" << endl;
    //         close(clientSocket);
    //         return false;
    //     }


    //     return true;
    // }


    bool connectToServer(const string& ipAddress, int port) {
        sockaddr_in hint;
        hint.sin_family = AF_INET;
        hint.sin_port = htons(port);
        inet_pton(AF_INET, ipAddress.c_str(), &hint.sin_addr);

        int connectRes = connect(clientSocket, (sockaddr*)&hint, sizeof(hint));
        if (connectRes == -1) {
            cerr << "Error: Cannot connect to Server!" << endl;
            return false;
        }
        return true;   
    }   


   void startReceiving() {
        receivingThread = thread([this]() {
            char buffer[4096];
            while (true) {
                int bytesReceived = recv(clientSocket, buffer, 4096, 0);
                if (bytesReceived <= 0) {
                    cerr << "Error: Server is down" << endl;
                    close(clientSocket);
                    break;
                }
                cout << string(buffer, 0, bytesReceived) << endl;
            }
        });
    }

    void sendToServer(const string& message) {
        send(clientSocket, message.c_str(), message.size() + 1, 0);
    }





    void displayMenu() {
        cout << "----- Services -----" << endl;
        cout << "1. Login and Registration" << endl;
        cout << "2. Start Communication" << endl;
        cout << "3. Start chat Room" << endl;
        cout << "---------------" << endl;
    }

    void startCommunication() {
        receivingThread = thread(&Client::receiveData, this);

        char buffer[4096];
        while (true) {
            string userInput;
            cout << "You: ";
            getline(cin, userInput);
            send(clientSocket, userInput.c_str(), userInput.size() + 1, 0);
        }
    }

    void closeConnection() {
        close(clientSocket);
        receivingThread.join();
    }


    void handleUserInteraction() {
        int option;
        cout << "1: Register" << endl << "2: Login" << endl << "Your Choice: ";
        cin >> option;
        cin.ignore();

        send(clientSocket, to_string(option).c_str(), to_string(option).size() + 1, 0);

        if (option == 1 || option == 2) {
            char username[1024];
            char password[1024];

            cout << "Enter Username: ";
            cin.getline(username, sizeof(username));
            send(clientSocket, username, strlen(username), 0);

            cout << "Enter Password: ";
            cin.getline(password, sizeof(password));
            send(clientSocket, password, strlen(password), 0);
            
            char buffer[1024] = {0};
            read(clientSocket, buffer, 1024);
            cout << buffer << endl;
        } else {
            cout << "Invalid option" << endl;
        }
    }



    void runMenu() {
        int choice;
        string roomName;
        connectToServer(IP_SERVER, PORT);
        do {
            displayMenu();
            cout << "Enter your choice: ";
            cin >> choice;
            cin.ignore(); // Clear input buffer
            send(clientSocket, to_string(choice).c_str(), to_string(choice).size() + 1, 0);
            switch (choice) {
                case 1:
                    startCommunication();
                    closeConnection();
                    break;
                case 2:
                    handleUserInteraction();
                    // closeConnection();
                    break;
                case 3:
                    {
                        std::string roomName;
                        std::cout << "Enter the Room: ";
                        std::getline(std::cin, roomName);
                        sendToServer(roomName);

                        startReceiving();

                        std::string clientName;
                        std::cout << "Enter Your name: ";
                        std::getline(std::cin, clientName);
                        sendToServer(clientName);

                        while (true) {
                            std::string userInput;
                            std::getline(std::cin, userInput);
                            sendToServer(userInput);
                        }
                    }
                    break;
                default:
                    cout << "Invalid choice. Please enter a valid option." << endl;
                    break;
            }
        } while (choice != 4);
    }
};

int main() {
    Client client;
    client.runMenu();
    return 0;
}
