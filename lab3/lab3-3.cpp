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
const int row = 101;
const int col = 101;
char mazeArray[row][col];
int dx[] = {-1, 1, 0, 0};
int dy[] = {0, 0, -1, 1};
string move_dir[] = {"A","D","W","S"};

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
    int port = 10303;
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
        memset(buffer, 0, 1024); 
        size_t found = rec_maze.find("Enter your move(s)>");
        if(found != string::npos) {
            break;}
    }
    size_t start_index = rec_maze.find("\n   "); 
    size_t exit_index = rec_maze.find("Enter your move(s)>",  start_index+1 ); 
    string extracted_maze = "";
    if (start_index != string::npos && exit_index != string::npos) {
        extracted_maze = rec_maze.substr(start_index, exit_index - start_index);
        //cout << "Extracted maze:\n" << extracted_maze ;
    } else {
        cout << "Maze not found in the input string." << endl;
    }
    memset(buffer, 0, 1024);
    string part;
    int sent_move;
    string ini = "kkkkkkklllllllllll\n";  //lllkk
    sent_move = send(sock, ini.c_str(), strlen(ini.c_str()), 0);
    bytes_received = recv(sock, buffer, sizeof(buffer), 0);
    
    while(true){
        memset(buffer, 0, 1024); 
        string up = "i\n";
        sent_move = send(sock, up.c_str(), strlen(up.c_str()), 0);
        bytes_received = recv(sock, buffer, sizeof(buffer), 0);
		buffer[bytes_received] = '\0';
        part.append(buffer, bytes_received);
        size_t found = part.find("-1");
        if(found != string::npos) {
            //c out<<1<<buffer;
            break;}  
    }
    part.clear();
    while(true){
        memset(buffer, 0, 1024); 
        string left = "j\n";
        sent_move = send(sock, left.c_str(), strlen(left.c_str()), 0);
        bytes_received = recv(sock, buffer, sizeof(buffer), 0);
		buffer[bytes_received] = '\0';
        part.append(buffer, bytes_received);
        size_t found = part.find("0:  ##########");
        if(found != string::npos) {
            //cout<<2<<buffer;
            break;}  
    }
    memset(buffer, 0, 1024);
    part.clear();
    part = "\n";

    string ori = "lllkk\n";  //lllkk
    sent_move = send(sock, ori.c_str(), strlen(ori.c_str()), 0);
    bytes_received = recv(sock, buffer, sizeof(buffer), 0);
    //cout<<3<<buffer;

    for(int i=0 ; i<15 ; i++){
        for(int j=0 ; j<9; j++){
            memset(buffer, 0, 1024); 
            string right = "lllllllllll\n";   // each row idx = 1-99
            sent_move = send(sock, right.c_str(), strlen(right.c_str()), 0);
            bytes_received = recv(sock, buffer, sizeof(buffer), 0);
            buffer[bytes_received] = '\0';
            part.append(buffer, bytes_received);
            //if(i==0 && j==0)cout<<"fi"<<i<<buffer;
        }
        for(int k=0; k<9; k++){
            memset(buffer, 0, 1024); 
            string left = "jjjjjjjjjjj\n";  
            sent_move = send(sock, left.c_str(), strlen(left.c_str()), 0);
            bytes_received = recv(sock, buffer, sizeof(buffer), 0);
        }
        string down_left = "kkkkkkk\n";
        sent_move = send(sock, down_left.c_str(), strlen(down_left.c_str()), 0);
        recv(sock, buffer, sizeof(buffer), 0);
    }
    cout<<"move fin"<<endl;
    
    vector<string> words{};
    string sw = "Enter your move(s)>";
    size_t pos = 0;
    while ((pos = part.find(sw)) != string::npos) {
        words.push_back(part.substr(0, pos));
        part.erase(0, pos + sw.length());
    }

    //cout<<words.size()<<endl;
    //cout<<words[134];
    //9-19, 28-38, 47-57, 66-76, 85-95, 104-114, 123-133

    for(int num=0; num<15; num++) {
        for(int i=0; i<9; i++){
            for(int j=0; j<7; j++){
                for(int k=0; k<11; k++){
                    if((num*7 + j)>100){
                        break;
                    }else{
                        mazeArray[num*7 + j][11*i + k +1] = words[num*9 + i][9+k + j*19]; //yx
                        //cout<<words[num*9 + i][9+k + j*19-4] << words[num*9 + i][9+k + j*19-3];
                        //cout<<num*7 + j << 11*i + k +1<<num*9 + i <<9+k + j*19<<endl;
                    }
                }
            }
        }  
    }
    int startX, startY, endX, endY;
    for(int i=0; i<101; i++){
        for(int j=0; j<101; j++){
            if(j==0 || j==100)
                mazeArray[i][j] = '#';
            //cout<<mazeArray[i][j];
            if (mazeArray[i][j] == '*') {
                startX = j, startY = i;
                //cout << "Found '*' at position (" << j << ", " << i << ")" << endl;
            } else if (mazeArray[i][j] == 'E') {
                endX = j, endY = i;
                //cout << "Found 'E' at position (" << j << ", " << i << ")" << endl;
            }
        }//cout<<endl;
    }  
    
    string movements = solve_maze(startX, startY, endX, endY);
    movements +="\n";
    
    int sent_ans = send(sock, movements.c_str(), strlen(movements.c_str()), 0);
    if (sent_ans < 0) {
        perror("sent_ans err");
    } else {
        cout << "sent ans:\n" << movements ;
    }
    //recv(sock, buffer, sizeof(buffer), 0);
    char response_buffer[2048];
    string server_message;

    int received2 = recv(sock, response_buffer, sizeof(response_buffer), 0);
    response_buffer[received2] = '\0';
    //cout<<"rec1"<<endl<<response_buffer;

    received2 = recv(sock, response_buffer, sizeof(response_buffer), 0);
    response_buffer[received2] = '\0';
    cout<<"rec"<<endl<<response_buffer;

    received2 = recv(sock, response_buffer, sizeof(response_buffer), 0);
    response_buffer[received2] = '\0';
    cout<<response_buffer;
    
    received2 = recv(sock, response_buffer, sizeof(response_buffer), 0);
    //cout<<response_buffer;
    close(sock);    
   

}

