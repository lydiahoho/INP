#include <bits/stdc++.h>
#include <iostream>
#include <arpa/inet.h> 
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
using namespace std;
const int row = 220;
const int col = 220;
char mazeArray[row][col];
int dx[] = {-1, 1, 0, 0};
int dy[] = {0, 0, -1, 1};
string move_dir[] = {"A","D","W","S"};
int startX ,startY;

bool is_valid(int x, int y) {
    return x >= 0 && x < row && y >= 0 && y < col && mazeArray[x][y] != '#';
}

// Function to solve the maze using DFS
string solve_maze(int endX, int endY) {
    stack<pair<int, int>> st;
    st.push({startX, startY});
    vector<vector<bool>> visited(row, vector<bool>(col, 0));
    vector<vector<pair<int, int>>> parent(row, vector<pair<int, int>>(col, make_pair(-1, -1)));
    int sx = startX; int sy = startY;
    while (!st.empty()) {
        int x = st.top().first;
        int y = st.top().second;
        st.pop();
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
            startX = 500;
            cout<<"Success!"<<endl;
            return path;
        }

        else if(mazeArray[y][x] == 'p'){
            string path = "";
            int tmpx = parent[y][x].first;
            int tmpy = parent[y][x].second;
            x = tmpx; y = tmpy;
            while (x != sx || y != sy) {
                int prevX = parent[y][x].first;
                int prevY = parent[y][x].second;
                for (int i = 0; i < 4; i++){
                    if (x - prevX == dx[i] && y - prevY == dy[i]) {
                        startX += dx[i];
                        startY += dy[i];
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
    int port = 10304;
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
    struct timeval time1, time2;
    gettimeofday(&time1, 0);
    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0){
        perror("connect fail");
        return 1;
    }else 
		printf("Connect...\n");

	int bytes_received, trash_recv;
	char buffer[1024], trash[1024];
    string rec_maze;
	while (true) {
        bytes_received = recv(sock, buffer, sizeof(buffer), 0);
		buffer[bytes_received] = '\0';
        rec_maze.append(buffer, bytes_received);
        memset(buffer, 0, 1024); 
        size_t found = rec_maze.find("Enter your move(s)>");
        if(found != string::npos) {break;}
    }
    size_t start_index = rec_maze.find("\n   "); 
    size_t exit_index = rec_maze.find("Enter your move(s)>",  start_index+1 ); 
    string extracted_maze = "";
    if (start_index != string::npos && exit_index != string::npos) {
        extracted_maze = rec_maze.substr(start_index, exit_index - start_index);
        cout << "Initial:" << extracted_maze ;
    } else {
        cout << "Maze not found in the input string." << endl;
    }

    for(int i=0; i<220; i++){
        for(int j=0; j<220; j++){
            mazeArray[i][j] = 'p';
        }
    }
    memset(buffer, 0, 1024); 
    string part;
    int sent_move;
    int endX=300, endY=300;
    string movements = "\n";
    startX = 105, startY = 105;
    int fin=1;
    while(fin){
        sent_move = send(sock, movements.c_str(), strlen(movements.c_str()), 0);
        if (sent_move < 0) {
            perror("sent_move err");
        } else {
            //cout << "sent move:" << movements ;
        }
        part.clear();
        while (true) {
            if(startX ==500){fin= 0; break;}
            bytes_received = recv(sock, buffer, sizeof(buffer), 0);
            buffer[bytes_received] = '\0';
            part.append(buffer, bytes_received);
            size_t found = part.find("Enter your move(s)>");
            if(found != string::npos) {break;}
        } 
        movements.clear();

        for(int i=0 ; i<7; i++){
            for(int j=0; j<11; j++){
                mazeArray[startY-3+i][startX-5+j] = part[i*19 + 8 + j];
                if ( mazeArray[startY-3+i][startX-5+j] == 'E') {
                    endX = startX-5+j, endY = startY-3+i;
                    cout << "Found 'E' at position (" << endX << ", " << endY << ")" << endl;
                }
                
            }
        }
        memset(buffer, 0, 1024);
        movements = solve_maze(endX, endY);  
        movements +="\n";
    }
    char buf[1024];
    while(1){
        bytes_received = recv(sock, buf, sizeof(buffer), 0);
        buf[bytes_received] = '\0'; 
        cout<<buf;
        if(bytes_received<=0) {break;}
    }

    cout << "\nclose" << endl;
    close(sock);


}

