#include <bits/stdc++.h>
#include <iostream>
#include <arpa/inet.h> // inet_addr()
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h> // read(), write(), close()
#include <chrono>
#include <thread>
using namespace std;
const int row = 7;
const int col = 11;

int main(){  
  	// create a tcp socket
    int port = 10301;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0){
        perror("socket fail");
        return 1;
    }else
    	printf("socket created\n");
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr("140.113.213.213");
    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0){
        perror("connect fail");
        return 1;
    }else 
		printf("Connect...\n");

	int bytes_received;
	char buffer[1024];
    string rec_maze;
	while (true) {
        bytes_received = recv(sock, buffer, sizeof(buffer), 0);
		buffer[bytes_received] = '\0';
        rec_maze.append(buffer, bytes_received);
        size_t found = rec_maze.find("[16 steps left] Enter your move(s)>");
        if(found != string::npos) {
            break;}
    }
        size_t start_index = rec_maze.find("###########\n"); // Find the index of '*'
        size_t exit_index = rec_maze.find("###########\n",  start_index + 1);  // Find the index of 'E'
        string extracted_maze;
        if (start_index != string::npos && exit_index != string::npos) {
            extracted_maze = rec_maze.substr(start_index, exit_index - start_index + 12);
            cout << "Extracted maze:\n" << extracted_maze << endl;
        } else {
            cout << "Maze not found in the input string." << endl;
        }
        vector<string> words{};
        string sw = "\n";
        size_t pos = 0;
        while ((pos = extracted_maze.find(sw)) != string::npos) {
            words.push_back(extracted_maze.substr(0, pos));
            extracted_maze.erase(0, pos + sw.length());
        }
        char mazeArray[row][col];
        int y=0, startX, startY, endX, endY;
        for (const string& row : words) {
            int x = 0;
            for (char c : row) {
                mazeArray[y][x] = c;
                if (c == '*') {
                    startX = x, startY = y;
                    cout << "Found '*' at position (" << x << ", " << y << ")" << endl;
                } else if (c == 'E') {
                    endX = x, endY = y;
                    cout << "Found 'E' at position (" << x << ", " << y << ")" << endl;
                }
                x++;
            }
            y++;
        }
        int stepX = endX - startX;
        int stepY = endY - startY;
        string movements = "";
        if (stepX > 0) {
            movements += string(stepX, 'D'); // Add 'D' (right) multiple times
        } else if (stepX < 0) {
            movements += string(-stepX, 'A'); // Add 'A' (left) multiple times
        }

        if (stepY > 0) {
            movements += string(stepY, 'S'); // Add 'S' (down) multiple times
        } else if (stepY < 0) {
            movements += string(-stepY, 'W'); // Add 'W' (up) multiple times
        }
        movements +="\n";
        int sent_ans = send(sock, movements.c_str(), strlen(movements.c_str()), 0);
        if (sent_ans < 0) {
            perror("sent_ans");
        } else {
            cout << "Sent movements to the server: " << movements;
        }
        
    char response_buffer[1024];
    string server_message;
    while (true) {
        int received2 = recv(sock, response_buffer, sizeof(response_buffer), 0);
		//response_buffer[bytes_received] = '\0';
        server_message.append(response_buffer, received2);
        memset(response_buffer, 0, 1024);
        size_t found = server_message.find("\n");
        if(found != string::npos) {
            break;}
    }
    cout<<server_message;
    cout << "close\n" << endl;
    close(sock);
    

        
    	
}

