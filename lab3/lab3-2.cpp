#include <bits/stdc++.h>
#include <iostream>
#include <arpa/inet.h> // inet_addr()
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
using namespace std;
const int row = 21;
const int col = 79;
char mazeArray[row][col];
int dx[] = {-1, 1, 0, 0};
int dy[] = {0, 0, -1, 1};
string move_dir[] = {"A", "D","W", "S"};

bool is_valid(int x, int y) {
    return x >= 0 && x < row && y >= 0 && y < col && mazeArray[x][y] != '#';
}

// Function to solve the maze using DFS
string solve_maze( int startX, int startY, int endX, int endY) {
    stack<pair<int, int>> st;
    st.push({startX, startY});
    vector<vector<bool>> visited(row, vector<bool>(col, 0));
    vector<vector<pair<int, int>>> parent(row, vector<pair<int, int>>(col, make_pair(-1, -1)));
    while (!st.empty()) {
        int x = st.top().first;
        int y = st.top().second;
        st.pop();
        //cout<<x<<" "<<y<<endl;
        visited[y][x] = true;
        if (x == endX && y == endY) {
            string path = "";
            while (x != startX || y != startY) {
                int prevX = parent[y][x].first;
                int prevY = parent[y][x].second;
                for (int i = 0; i < 4; i++) {
                    if (x - prevX == dx[i] && y - prevY == dy[i]) {
                        path = move_dir[i] + path;
                        break;
                    }
                }
                
                x = prevX;
                y = prevY; 
            }
            return path;
        }
       
        for (int i = 0; i < 4; i++) {
            int newX = x + dx[i];
            int newY = y + dy[i];
            if (is_valid(newY, newX) && !visited[newY][newX]) {
                //cout<<newX<<" "<<newY<<endl;
                st.push({newX, newY});
                parent[newY][newX].first = x;
                parent[newY][newX].second = y;
            }
        }
    }
    return "No path found";  // No path exists from '*' to 'E'
}

int main(){  
  	// create a tcp socket
    int port = 10302;
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
        size_t found = rec_maze.find("Enter your move(s)>");
        memset(buffer, 0, 1024);
        if(found != string::npos) {
            break;}
    }
    cout<<rec_maze;
    size_t start_index = rec_maze.find("##"); 
    size_t exit_index = rec_maze.find("Enter your move(s)>",  start_index+1 ); 
    string extracted_maze;
    if (start_index != string::npos && exit_index != string::npos) {
        extracted_maze = rec_maze.substr(start_index, exit_index - start_index);
        //cout << "Extracted maze:\n" << extracted_maze ;
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
    words.pop_back();
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
    string movements = solve_maze(startX, startY, endX, endY);
    movements +="\n";
    int sent_ans = send(sock, movements.c_str(), strlen(movements.c_str()), 0);
    if (sent_ans < 0) {
        perror("sent_ans");
    } else {
        cout << "movements:" << movements <<endl ;
    }
        
    char response_buffer[1024];
    string server_message;
    while (true) {
        int received2 = recv(sock, response_buffer, sizeof(response_buffer), 0);
		//response_buffer[bytes_received] = '\0';
        server_message.append(response_buffer, received2);
        memset(buffer, 0, 1024);
        size_t found = server_message.find("\n");
        if(found != string::npos) {
            break;}
    }
    cout<<server_message;
    cout << "\nclose" << endl;
    close(sock);    
    	
}

