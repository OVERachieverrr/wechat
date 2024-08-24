#include<iostream>
#include<WinSock2.h>
#include<WS2tcpip.h>
#include<tchar.h>
#include<thread>
#include<string>
#include<fstream>
#include<vector>
#include<stdio.h>


using namespace std;

#pragma comment(lib,"ws2_32.lib")

class ChatClient {
public:
    ChatClient(const string& serverAddress, int port)
        : serverAddress(serverAddress), port(port) {}

    bool Initialize() {
        WSADATA wsaData;
        return WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
    }

    bool Connect() {
        s = socket(AF_INET, SOCK_STREAM, 0);
        if (s == INVALID_SOCKET) {
            cout << "Couldn't create socket" << endl;
            return false;
        }

        sockaddr_in serveraddr;
        serveraddr.sin_family = AF_INET;
        serveraddr.sin_port = htons(port);
        inet_pton(AF_INET, serverAddress.c_str(), &(serveraddr.sin_addr));

        if (connect(s, reinterpret_cast<sockaddr*>(&serveraddr), sizeof(serveraddr)) == SOCKET_ERROR) {
            cout << "Couldn't connect to server" << endl;
            closesocket(s);
            WSACleanup();
            return false;
        }

        cout << "Successfully connected to server" << endl;
        return true;
    }

    void Start() {
        thread SenderThread(&ChatClient::SendMsg, this);
        thread ReceiverThread(&ChatClient::ReceiveMessage, this);

        SenderThread.join();
        ReceiverThread.join();
    }

private:
    SOCKET s;
    string serverAddress;
    int port;

    void SendMsg() {
        string name;
        cout << "Enter your chat name: ";
        getline(cin, name);

        string message;
        while (true) {
            cout << "Enter your message : ";
            getline(cin, message);

            

            if (message == "/sendphoto") {
                cout << "Enter the path to the image file: ";
                string imagePath;
                getline(cin, imagePath);

                ifstream imageFile(imagePath, ios::binary);
                if (!imageFile.is_open()) {
                    cout << "Failed to open the image file!" << endl;
                    continue;
                }

                vector<char> imageData((istreambuf_iterator<char>(imageFile)), istreambuf_iterator<char>());
                imageFile.close();

                char header = 2;  // Image header
                send(s, &header, sizeof(header), 0);

                int imgSize = imageData.size();
                send(s, reinterpret_cast<char*>(&imgSize), sizeof(imgSize), 0);

                send(s, imageData.data(), imgSize, 0);

                cout << "Image sent successfully!" << endl;
            }
            else {
                string msg = name + ": " + message;

                char header = 1;  // Text message header
                send(s, &header, sizeof(header), 0);

                int bytesent = send(s, msg.c_str(), msg.length(), 0);
                if (bytesent == SOCKET_ERROR) {
                    cout << "Error in sending message" << endl;
                    break;
                }

                if (message == "quit") {
                    cout << "Stopping chat!!" << endl;
                    break;
                }
            }
        }

        closesocket(s);
        WSACleanup();
    }

    void ReceiveMessage() {
        char buffer[4096];
        while (true) {
            char header;
            int bytesReceived = recv(s, &header, sizeof(header), 0);
            if (bytesReceived <= 0) {
                cout << "Disconnected" << endl;
                break;
            }

            if (header == 1) {  // Text message
                int recvlenght = recv(s, buffer, sizeof(buffer), 0);
                if (recvlenght <= 0) {
                    cout << "Disconnected" << endl;
                    break;
                }
                else {
                    string msg(buffer, recvlenght);
                    cout << "/n" << msg << endl;
                }
            }
            else if (header == 2) {  // Image data
                int imgSize;
                recv(s, reinterpret_cast<char*>(&imgSize), sizeof(imgSize), 0);

                vector<char> imgData(imgSize);
                int totalBytesReceived = 0;
                while (totalBytesReceived < imgSize) {
                    bytesReceived = recv(s, imgData.data() + totalBytesReceived, imgSize - totalBytesReceived, 0);
                    if (bytesReceived <= 0) {
                        cout << "Disconnected during image transfer" << endl;
                        break;
                    }
                    totalBytesReceived += bytesReceived;
                }

                ofstream outputImage("received_image.jpg", ios::binary);
                outputImage.write(imgData.data(), imgSize);
                outputImage.close();

                cout << "Image received and saved as 'received_image.jpg'" << endl;
            }
        }

        closesocket(s);
        WSACleanup();
    }
};

int main() {
    ChatClient client("127.0.0.1", 12345);

    if (!client.Initialize()) {
        cout << "Failed setting up Windows sockets" << endl;
        return 1;
    }

    if (!client.Connect()) {
        return 1;
    }

    client.Start();
    return 0;
}
