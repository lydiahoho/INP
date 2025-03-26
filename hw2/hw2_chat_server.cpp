#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <bits/stdc++.h>
#include<vector>
#include <map>
#include <unordered_set> 
#include <algorithm>
#include <locale>
using namespace std;

#define MAX_CLIENTS 10
#define MAX_MSG_LEN 150
#define MAX_USERNAME_LEN 20
#define MAX_PASSWORD_LEN 20
#define MAX_ROOMS 100
#define MAX_CHAT_HISTORY 10

// Define structures and variables needed for the chat server
struct User {
    char username[MAX_USERNAME_LEN];
    char password[MAX_PASSWORD_LEN];
    bool registered;
    string status;
    int chatroom;
};

struct ChatRoom {
    int roomNumber;
    string creator;
    string pinnedMessage;
    vector<string> chatHistory;
    unordered_set<int> participants;
};

vector<string> filteringList = {
    "==",
    "Superpie",
    "hello",
    "Starburst Stream",
    "Domain Expansion"
};
vector<User> users;
unordered_map<int, string> loggedUsers;
vector<ChatRoom> chatRooms(MAX_ROOMS);

void sendWelcomeMessage(int client_socket) {
    const char *welcomeMsg = "*********************************\n"
                              "** Welcome to the Chat server. **\n"
                              "*********************************\n% ";
    send(client_socket, welcomeMsg, strlen(welcomeMsg), 0);
}

bool registerUser(int client_socket,const char *username, const char *password) {
    // Check for redundant or missing parameters
    if (strlen(username) == 0 || strlen(password) == 0) {
        const char *usageMsg = "Usage: register <username> <password>\n% ";
        send(client_socket, usageMsg, strlen(usageMsg), 0);
        return false;
    }
    // Check if the username already exists
    for (const auto &user : users) {
        if (strcmp(user.username, username) == 0) {
            const char *usernameExistsMsg = "Username is already used.\n% ";
            send(client_socket, usernameExistsMsg, strlen(usernameExistsMsg), 0);
            return false;
        }
    }
    // Create a new user and add it to the list of users
    User newUser;
    strncpy(newUser.username, username, MAX_USERNAME_LEN - 1);
    newUser.username[MAX_USERNAME_LEN - 1] = '\0';
    strncpy(newUser.password, password, MAX_PASSWORD_LEN - 1);
    newUser.password[MAX_PASSWORD_LEN - 1] = '\0';
    newUser.registered = true;
    newUser.status = "offline";
    newUser.chatroom = 0;

    // Add the new user to the list
    users.push_back(newUser);

    const char *successMsg = "Register successfully.\n% ";
    send(client_socket, successMsg, strlen(successMsg), 0);
    return true;
}

bool loginUser(int client_socket, const char *username, const char *password) {
    // Check for redundant or missing parameters
    //cout<<client_socket<<" "<<username<<" "<<password<<endl;
    if (strlen(username) == 0 || strlen(password) == 0) {
        const char *usageMsg = "Usage: login <username> <password>\n% ";
        send(client_socket, usageMsg, strlen(usageMsg), 0);
        return false;
    }
    for (const auto& pair : loggedUsers) {
        cout << "Client Socket: " << pair.first << " - Username: " << pair.second << endl;
    }
    // Check if the client has already logged in
    if (loggedUsers.find(client_socket) != loggedUsers.end()) {
        const char *logoutMsg = "Please logout first.\n% ";
        send(client_socket, logoutMsg, strlen(logoutMsg), 0);
        return false;
    }

    // Check if the username exists
    bool userExists = false;
    for (auto &user : users) {
        if (strcmp(user.username, username) == 0) {
            userExists = true;

            // Check if the password matches
            if (strcmp(user.password, password) == 0) {
                // Check if the user is already logged in from another client
                for (const auto &loggedUser : loggedUsers) {
                    if (loggedUser.second == username) {
                        const char *loginFailedMsg = "Login failed.\n% ";
                        send(client_socket, loginFailedMsg, strlen(loginFailedMsg), 0);
                        return false;
                    }
                }
                // Login successful
                loggedUsers[client_socket] = username;
                user.status = "online";
                const char *welcomeMsg = "Welcome, ";
                send(client_socket, welcomeMsg, strlen(welcomeMsg), 0);
                send(client_socket, username, strlen(username), 0);
                send(client_socket, ".\n% ", strlen(".\n% "), 0);
                return true;
            } else {
                const char *loginFailedMsg = "Login failed.\n% ";
                send(client_socket, loginFailedMsg, strlen(loginFailedMsg), 0);
                return false;
            }
        }
    }  
    if (!userExists) {
        const char *loginFailedMsg = "Login failed.\n% ";
        send(client_socket, loginFailedMsg, strlen(loginFailedMsg), 0);
        return false;
    }
    return false;
}

void logoutUser(int client_socket) {
    // Check if the client is logged in
    if (loggedUsers.find(client_socket) == loggedUsers.end()) {
        const char *loginFirstMsg = "Please login first.\n% ";
        send(client_socket, loginFirstMsg, strlen(loginFirstMsg), 0);
        return;
    }
    string username = loggedUsers[client_socket];
    for (auto &user : users) {
        if (strcmp(user.username, username.c_str()) == 0) {
            user.status = "offline";
            break;
        }
    }
    const char *byeMsg = "Bye, ";
    send(client_socket, byeMsg, strlen(byeMsg), 0);
    send(client_socket, loggedUsers[client_socket].c_str(), loggedUsers[client_socket].size(), 0);
    send(client_socket, ".\n% ", strlen(".\n% "), 0);
    loggedUsers.erase(client_socket);
}

void exitServer(int client_socket, fd_set *master) {
    if (loggedUsers.find(client_socket) != loggedUsers.end()) {
        // no logout
        const char *byeMsg = "Bye, ";
        send(client_socket, byeMsg, strlen(byeMsg), 0);
        send(client_socket, loggedUsers[client_socket].c_str(), loggedUsers[client_socket].size(), 0);
        send(client_socket, ".\n", strlen(".\n"), 0);
        string username = loggedUsers[client_socket];
        cout<<username<<"exit"<<endl;
        for (auto &user : users) {
            if (strcmp(user.username, username.c_str()) == 0) {
                user.status = "offline";
                break;
            }
        }
        loggedUsers.erase(client_socket);
    }
    // const char *exitMsg = "=====Connection closed=====";
    // send(client_socket, exitMsg, strlen(exitMsg), 0);
    close(client_socket);
    FD_CLR(client_socket, master); // Remove the closed socket from the master set
}

void whoamiCommand(int client_socket) {
    // Check if the client is logged in
    if (loggedUsers.find(client_socket) != loggedUsers.end()) {
        const char *username = loggedUsers[client_socket].c_str();
        send(client_socket, username, strlen(username), 0);
        send(client_socket, "\n% ", strlen("\n% "), 0);
    } else {
        const char *loginFirstMsg = "Please login first.\n% ";
        send(client_socket, loginFirstMsg, strlen(loginFirstMsg), 0);
    }
}

void setStatus(int client_socket, const char* status) {
    // Check if the client is logged in
    if (loggedUsers.find(client_socket) != loggedUsers.end()) {
        string newStatus = status;

        // Check for valid status (online, offline, busy)
        if (newStatus != "online" && newStatus != "offline" && newStatus != "busy") {
            const char *invalidStatusMsg = "set-status failed\n% ";
            send(client_socket, invalidStatusMsg, strlen(invalidStatusMsg), 0);
            return;
        }

        // Find the user and update their status
        string username = loggedUsers[client_socket];
        for (auto& user : users) {
            if (strcmp(user.username, username.c_str()) == 0) {
                user.status = newStatus;
                string setStatusMsg = username + " " + newStatus + "\n% ";
                send(client_socket, setStatusMsg.c_str(), setStatusMsg.length(), 0);
                return;
            }
        }
    } else {
        const char *loginFirstMsg = "Please login first.\n% ";
        send(client_socket, loginFirstMsg, strlen(loginFirstMsg), 0);
    }
}

void listUsers(int client_socket) {
    // Check if the client is logged in
    if (loggedUsers.find(client_socket) != loggedUsers.end()) {
        // Sort users by username alphabetically
        sort(users.begin(), users.end(), [](const User &a, const User &b) {
            return strcmp(a.username, b.username) < 0;
        });
        // Send the list of users and their statuses
        for (const auto &user : users) {
            string userStatus = user.username;
            userStatus += " ";
            userStatus += user.status;
            userStatus += "\n";
            send(client_socket, userStatus.c_str(), userStatus.length(), 0);
        }
        send(client_socket, "% ", strlen("% "), 0);

    } else {
        const char *loginFirstMsg = "Please login first.\n% ";
        send(client_socket, loginFirstMsg, strlen(loginFirstMsg), 0);
    }
}

void enterChatRoom(int client_socket, int roomNumber, const string& username) {
    // Check for redundant or missing parameters
    if (roomNumber < 1 || roomNumber > 100) {
        string invalidRoomMsg = "Number " + to_string(roomNumber) + " is not valid.\n% ";
        send(client_socket, invalidRoomMsg.c_str(), invalidRoomMsg.length(), 0);
        return;
    }

    // Check if the client has logged in
    if (loggedUsers.find(client_socket) == loggedUsers.end()) {
        const char *loginFirstMsg = "Please login first.\n% ";
        send(client_socket, loginFirstMsg, strlen(loginFirstMsg), 0);
        return;
    }

    ChatRoom& room = chatRooms[roomNumber - 1]; // Room index starts from 0
    // If the chat room doesn't exist, create a new room
    char roomNumStr[5];
    if (room.roomNumber == 0) {
        room.roomNumber = roomNumber;
        room.creator = username;      
    }
    const char *welcomeMsg = "Welcome to the public chat room.\n";
    const char *roomInfoMsg = "Room number: ";
    sprintf(roomNumStr, "%d", room.roomNumber);
    send(client_socket, welcomeMsg, strlen(welcomeMsg), 0);
    send(client_socket, roomInfoMsg, strlen(roomInfoMsg), 0);
    send(client_socket, roomNumStr, strlen(roomNumStr), 0);
    send(client_socket, "\nOwner: ", strlen("\nOwner: "), 0);
    send(client_socket, room.creator.c_str(), room.creator.size(), 0);
    send(client_socket, "\n", strlen("\n"), 0);

    // Show chat history to the new participant (latest 10 records)
    int recordsToShow = min(MAX_CHAT_HISTORY, static_cast<int>(room.chatHistory.size()));
    int startIndex = room.chatHistory.size() - recordsToShow;
    for (decltype(room.chatHistory.size()) i  = startIndex; i < room.chatHistory.size(); ++i) {
        string record = room.chatHistory[i];
        send(client_socket, record.c_str(), record.length(), 0);
    }
    if (!room.pinnedMessage.empty()) {
        string p = room.pinnedMessage;
        send(client_socket, p.c_str(), p.length(), 0);
    }
    

    // Add client to the participants of the chat room
    room.participants.insert(client_socket);
    cout<<chatRooms[roomNumber - 1].participants.size()<<" participants"<<endl;

    // Notify all clients in the chat room about the new participant
    string participantMsg = username + " had enter the chat room.\n";
    for (const int& participant : room.participants) {
        if (participant != client_socket) {
            send(participant, participantMsg.c_str(), participantMsg.length(), 0);
        }
    }
    for (auto& user : users) {
        if (strcmp(user.username, username.c_str()) == 0) {
            user.chatroom = roomNumber;
            break;
        }
    }
    
}

void listChatRooms(int client_socket) {
    string roomsList = "";
    for (const ChatRoom& room : chatRooms) {
        if (room.roomNumber != 0) {
            roomsList += room.creator + " "+ to_string(room.roomNumber) + "\n";
        }
    }
    roomsList += "% " ;
    send(client_socket, roomsList.c_str(), roomsList.length(), 0);
}

void closeChatRoom(int client_socket, int roomNumber, const string& username) {
    // Check for redundant or missing parameters
    if (roomNumber < 1 || roomNumber > 100) {
        const char *invalidRoomMsg = "Usage: close-chat-room <number>\n% ";
        send(client_socket, invalidRoomMsg, strlen(invalidRoomMsg), 0);
        return;
    }

    // Check if the client has logged in
    if (loggedUsers.find(client_socket) == loggedUsers.end()) {
        const char *loginFirstMsg = "Please login first.\n% ";
        send(client_socket, loginFirstMsg, strlen(loginFirstMsg), 0);
        return;
    }

    ChatRoom& room = chatRooms[roomNumber - 1]; // Room index starts from 0

    // If the chat room doesn't exist
    if (room.roomNumber == 0) {
        string roomNotExistMsg = "Chat room " + to_string(roomNumber) + " does not exist.\n% ";
        cout<<roomNotExistMsg;
        send(client_socket, roomNotExistMsg.c_str(), roomNotExistMsg.length(), 0);
        return;
    }

    // Check if the user is the owner of the chat room
    if (room.creator != username) {
        const char *notOwnerMsg = "Only the owner can close this chat room.\n% ";
        send(client_socket, notOwnerMsg, strlen(notOwnerMsg), 0);
        return;
    }

    // Notify all clients in the chat room about the closure
    cout<<"close chatroom"<<room.roomNumber<<endl;
    string closureMsg = "Chat room " + to_string(roomNumber) + " was closed.\n% ";
    send(client_socket, closureMsg.c_str(), closureMsg.length(), 0);
    for (const int& participant : room.participants) {
        send(participant, closureMsg.c_str(), closureMsg.length(), 0);
    }
    
    
    for (auto& user : users) {
        if (user.chatroom == roomNumber){
            user.chatroom = 0;
        }
    }
    // Close the chat room by resetting its properties
    room.roomNumber = 0;
    room.creator = "";
    room.chatHistory.clear();
    room.participants.clear();
}

int isUserInChatRoom(int client_socket) {
    if (loggedUsers.find(client_socket) != loggedUsers.end()) {
        // The key exists in the map
        string username = loggedUsers[client_socket];
        for (auto& user : users) {
            if (strcmp(user.username, username.c_str()) == 0) {
                if(user.chatroom != 0 ) return user.chatroom;
                else break;
            }
        }
    } else return 0;
    
    return 0;
}

// 检查字符串是否包含在过滤列表中
string replaceKeywords(const string& input) {
    string result = input;
    auto toLowercase = [](const string& str) {
        string lowercaseStr = str;
        transform(lowercaseStr.begin(), lowercaseStr.end(), lowercaseStr.begin(),
            [](unsigned char c) { return std::tolower(c); });
        return lowercaseStr;
    };
    string lowercaseInput = toLowercase(input);
    for (const string& keyword : filteringList) {
        string lowercaseKeyword = toLowercase(keyword);
        size_t pos = lowercaseInput.find(lowercaseKeyword);
        while (pos != string::npos) {
            std::fill(result.begin() + pos, result.begin() + pos + keyword.length(), '*');
            pos = lowercaseInput.find(lowercaseKeyword, pos + 1);
        }
    }
    return result;
}

void processChatRoomCommand(int client_socket, const char* buffer, int roomNumber) {
    string command(buffer);
    ChatRoom& room = chatRooms[roomNumber - 1];
    cout<<"process chat room"<<roomNumber<<endl;
    if (command.find("/pin ") == 0) {
        // Pin message command implementation
        cout << "pin" << endl;
        string message = command.substr(5); // 提取 "/pin " 后的消息
        ChatRoom& room = chatRooms[roomNumber - 1];
        // 发送固定信息给所有聊天室参与者
        string pinnedMessage = "Pin -> [" + loggedUsers[client_socket] + "]: " + message ;
        pinnedMessage = replaceKeywords(pinnedMessage);
        room.pinnedMessage = pinnedMessage;
        for (const int& participant : room.participants) {
            send(participant, pinnedMessage.c_str(), pinnedMessage.length(), 0);
        }
        
    } 
    else if (strncmp(buffer, "/delete-pin", 11) == 0) {
        // Delete pin command implementation
        cout << "delete pin" << endl;
        ChatRoom& room = chatRooms[roomNumber - 1];
        if (room.pinnedMessage.empty()) {
            // If there's no pinned message
            const char *noPinMsg = "No pin message in chat room ";
            send(client_socket, (noPinMsg + to_string(roomNumber) + "\n").c_str(), strlen(noPinMsg) + 3, 0);
        } else {
            // Delete the pinned message in the chat room
            room.pinnedMessage = ""; // Clear the pinned message
        }
    } 
    else if (strncmp(buffer, "/exit-chat-room", 15) == 0){
        // Exit chat room command implementation
        string exitMsg = loggedUsers[client_socket] + " had left the chat room.\n";
        cout<<exitMsg;
        // Handle the client's exit from the chat room
        room.participants.erase(client_socket);
        string username = loggedUsers[client_socket];
        for (auto& user : users) {
            if (user.username == username) {
                user.chatroom = 0; // Set the user's chat room to 0 (server mode)
            }
        }
        send(client_socket, "% ", strlen("% "), 0);
        // Send exit message to all clients in the chat room
        for (const int& participant : room.participants) {
            send(participant, exitMsg.c_str(), exitMsg.length(), 0);
        }
        
    } 
    else if (strncmp(buffer, "/list-user", 10) == 0) {
        // List users command implementation
        string usersList;
        cout<<"list users"<<endl;       
        // List all users in the chat room with their statuses
        for (const int& participant : room.participants) {
            // Find the username of the participant using their socket
            string username, status;
            for (const auto& user : users) {
                if (loggedUsers[participant] == user.username) {
                    username = user.username;
                    status = user.status;
                    break;
                }
            }
            // Append the participant's username and status to the list
            usersList += username + " " + status + "\n";
        }
        // for (const auto &user : users) {
        //     if (room.participants.find(client_socket) != room.participants.end()) {
        //         string userStatus = user.username;
        //         userStatus += " ";
        //         userStatus += user.status;
        //         userStatus += "\n";
        //         usersList += userStatus;
        //     }
        // }
        send(client_socket, usersList.c_str(), usersList.length(), 0);
    } 
    else if (buffer[0] == '/') {
        // Any command not matching the existing ones will reach here
        string errorMsg = "Error: Unknown command\n";
        send(client_socket, errorMsg.c_str(), errorMsg.length(), 0);
    }
    else {
        // Message content (Typical chat message)
        // Broadcast the message to all users in the chat room
        cout<<"chat msg"<<endl;
        string message = "[" + loggedUsers[client_socket] + "]: " + command ;
        // Apply content filtering (replace certain keywords with '*')
        message = replaceKeywords(message);
        // Broadcast the filtered message to all users in the chat room
        for (const int& participant : room.participants) {
            send(participant, message.c_str(), message.length(), 0);
        }
        // Update chat history with the message
        room.chatHistory.push_back(message);
    }
}


// Function to process client commands and messages
void processClientCommand(int client_socket, fd_set *master) {
    // Receive command/message from client
    char buffer[MAX_MSG_LEN];
    int nbytes = recv(client_socket, buffer, sizeof(buffer), 0);
    buffer[nbytes] = '\0'; 
    int cr = isUserInChatRoom(client_socket);
    if (nbytes <= 0) {
        // Connection closed or error occurred
        if (nbytes == 0) {
            // Connection closed by client
            printf("Socket %d hung up\n", client_socket);
        } else {
            perror("recv");
        }
        close(client_socket);
        FD_CLR(client_socket, master);
    } 
    else if (cr != 0) {//該user有進入聊天室
        processChatRoomCommand(client_socket, buffer, cr); // 处理聊天室中的命令
    }
    else if (strncmp(buffer, "register", 8) == 0) {
        // Parse the username and password from the received command
        char username[MAX_USERNAME_LEN];
        char password[MAX_PASSWORD_LEN];
        if (sscanf(buffer, "register %19s %19s", username, password) == 2) {
            registerUser(client_socket, username, password);
        } else {
            const char *usageMsg = "Usage: register <username> <password>\n% ";
            send(client_socket, usageMsg, strlen(usageMsg), 0);
        }
    } 
    else if (strncmp(buffer, "login", 5) == 0) {
        // Parse the username and password from the received command
        char username[MAX_USERNAME_LEN];
        char password[MAX_PASSWORD_LEN];
        if (sscanf(buffer, "login %19s %19s", username, password) == 2) {
            loginUser(client_socket, username, password);
        } else {
            const char *usageMsg = "Usage: login <username> <password>\n% ";
            send(client_socket, usageMsg, strlen(usageMsg), 0);
        }
    } 
    else if (strncmp(buffer, "logout", 6) == 0) {
        logoutUser(client_socket);
    }
    else if (strncmp(buffer, "exit", 4) == 0) {
        exitServer(client_socket, master);
        return;
    }
    else if (strncmp(buffer, "whoami", 6) == 0) {
        whoamiCommand(client_socket);
    }
    else if (strncmp(buffer, "set-status", 10) == 0) {
        char status[MAX_MSG_LEN];
        if (sscanf(buffer, "set-status %s", status) == 1) {
            setStatus(client_socket, status);
        } else {
            const char *usageMsg = "Usage: set-status <status>\n% ";
            send(client_socket, usageMsg, strlen(usageMsg), 0);
        }
    }
    else if (strncmp(buffer, "list-user", 9) == 0) {
        // Check for any extra parameters in the command
        char extraParam[MAX_MSG_LEN];
        if (sscanf(buffer, "list-user %s", extraParam) == 1) {
            const char *usageMsg = "Usage: list-user\n% ";
            send(client_socket, usageMsg, strlen(usageMsg), 0);
        } else {
            listUsers(client_socket);
        }
    }
    else if (strncmp(buffer, "enter-chat-room", 15) == 0) {
        int roomNumber;
        string username = loggedUsers[client_socket];
        if (sscanf(buffer, "enter-chat-room %d", &roomNumber) == 1) {
            enterChatRoom(client_socket, roomNumber, username);
        } else {
            const char *usageMsg = "Usage: enter-chat-room <number>\n% ";
            send(client_socket, usageMsg, strlen(usageMsg), 0);
        }
    } 
    else if (strncmp(buffer, "list-chat-room", 14) == 0) {
        listChatRooms(client_socket);
    } 
    else if (strncmp(buffer, "close-chat-room", 15) == 0) {
        int roomNumber;
        string username = loggedUsers[client_socket];
        if (sscanf(buffer, "close-chat-room %d", &roomNumber) == 1) {
            closeChatRoom(client_socket, roomNumber, username);
        } else {
            const char *usageMsg = "Usage: close-chat-room <number>\n% ";
            send(client_socket, usageMsg, strlen(usageMsg), 0);
        }
    } 
    else {
        // Handle other commands accordingly
        const char *unknownCmd = "Error: Unknown command\n% ";
        send(client_socket, unknownCmd, strlen(unknownCmd), 0);
        
    }

}


int main(int argc, char *argv[]) {
    // Check for the correct number of arguments
    
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " [port number]" << std::endl;
        return 1;
    }

    // Parse the port number from command line argument
    int port = atoi(argv[1]);

    // Create socket, bind, listen, etc.
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("socket");
        exit(1);
    }
    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port); // 使用從命令行解析的端口
    server_address.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
        perror("bind");
        exit(1);
    }
    if (listen(server_socket, SOMAXCONN) == -1) {
        perror("listen");
        exit(1);
    }

    // Code omitted for brevity

    // Initialize file descriptor sets
    fd_set master;
    FD_ZERO(&master);

    fd_set read_fds;
    FD_ZERO(&read_fds);

    int fdmax; // maximum file descriptor number

    // Add server socket to master set
    FD_SET(server_socket, &master);
    fdmax = server_socket;
    cout << "fdmax" << fdmax << endl;
       // vector<User> users;
    // unordered_map<int, string> loggedUsers;

    // Main loop
    cout<<"server start..."<<endl;
    while (true) {
        read_fds = master;

        if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(1);
        }
        // Check all file descriptors for data to read
        for (int i = 0; i <= fdmax; ++i) {
            if (FD_ISSET(i, &read_fds)) {
                if (i == server_socket) {
                    // New connection request
                    // Accept the new connection, add it to master set, etc.
                    // Code omitted for brevity
                    struct sockaddr_in client_address; // 定义客户端地址结构体
                    socklen_t client_length = sizeof(client_address); // 初始化客户端地址结构体的大小
                    int new_client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_length);
                    if (new_client_socket == -1) {
                        perror("accept");
                        continue;
                    }
                    FD_SET(new_client_socket, &master); // 將新的客戶端套接字添加到master集合中
                    if (new_client_socket > fdmax) {
                        fdmax = new_client_socket; // 更新最大文件描述符
                    }
                    cout<<"new connect"<<endl;
                    sendWelcomeMessage(new_client_socket);
                } else {
                    // Check if the socket is closed
                    char buffer[1024];
                    int nbytes = recv(i, buffer, sizeof(buffer), MSG_PEEK);
                    if (nbytes <= 0) {
                        // Socket is closed or an error occurred
                        if (nbytes == 0) {
                            // Connection closed by client
                            cout << "Socket " << i << " hung up" << endl;
                        } else {
                            perror("recv");
                        }
                        close(i);
                        FD_CLR(i, &master); // Remove the closed socket from the master set
                        continue;
                    }
                    // Handle data from a client
                    processClientCommand(i, &master);
                }
            }
        }
    }

    return 0;
}

