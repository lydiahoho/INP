#include <bits/stdc++.h>
#include <iostream>
#include <arpa/inet.h> // inet_addr()
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fstream>


using namespace std;
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t totalSize = size * nmemb;
    output->append(static_cast<char*>(contents), totalSize);
    return totalSize;
} 

int main(){  
  	// create a tcp socket
    string student = "110550080";
    string int_addr = "172.21.0.4";
    string address= "140.113.213.213";
    string get_otp = "http://inp.zoolab.org:10314/otp?name=" + student;
    string upload = "http://inp.zoolab.org:10314/upload";
    
    int port = 10314;
    int int_port = 10001;
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

    string get_otp_cmd = "GET "+ get_otp + " HTTP/1.1\r\nHost: " + get_otp + "\r\nConnection: keep-alive\r\n\r\n";
   
    string rec_otp,otp;
    char buf[1024];
    int sent= send(sock, get_otp_cmd.c_str(), strlen(get_otp_cmd.c_str()), 0);
        if (sent < 0) {perror("sent_move err");} 
    while(1){
        int bytes_received = recv(sock, buf, sizeof(buf), 0);
        buf[bytes_received] = '\0'; 
        rec_otp.append(buf);
        if(bytes_received<=0) {break;}
    }
    size_t index = rec_otp.find(student); 
    if (index != string::npos) {
        otp = rec_otp.substr(index);
        cout << "otp:" << otp <<endl;
    } 
    close(sock);
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0){
        perror("socket fail");
        return 1;
    }else
    	printf("socket created\n");
    // struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr("140.113.213.213");
    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0){
        perror("connect fail");
        return 1;
    }else 
		printf("Connect...\n");
    string post_cmd = "POST /upload HTTP/1.1\r\n"
                      "Host: " + address +":"+ to_string(port) + "\r\n"
                      "Connection: keep-alive\r\n"
                      "Content-Type: multipart/form-data; boundary=----fuckboundary\r\n";
                      
    string content=   "------fuckboundary\r\n"
                      "Content-Disposition: form-data; name=\"file\"; filename=\"otp.txt\"\r\n"
                      "\r\n" + otp + "\r\n"
                      "------fuckboundary--\r\n";
                      
    post_cmd +="Content-Length: " + to_string(content.length()) + "\r\n\r\n" + content;

    send(sock, post_cmd.c_str(), strlen(post_cmd.c_str()), 0);
    //send(sock2, content.c_str(), strlen(content.c_str()), 0);

    while(1){
        int bytes_received = recv(sock, buf, sizeof(buf), 0);
        buf[bytes_received] = '\0'; 
        cout<<buf;
        if(bytes_received<=0) {break;}
    } 
    
    close(sock);  	
}

