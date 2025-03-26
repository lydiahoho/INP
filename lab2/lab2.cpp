#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <arpa/inet.h>
#include<bits/stdc++.h>
#include<sstream>
#include<vector>
#include <curl/curl.h>
using namespace std;
# define MAX_PAYLOAD_SIZE 3072

struct HEAD {
    uint64_t magic;     /* 'BINFLAG\x00' */
    uint32_t datasize;  /* in big-endian */
    uint16_t n_blocks;  /* in big-endian */
    uint16_t zeros;
}__attribute((packed)) binflag_header_t;

struct BLOCK {
    uint32_t offset;        /* in big-endian */
    uint16_t cksum;         /* XOR'ed results of each 2-byte unit in payload */
    uint16_t length;        /* ranges from 1KB - 3KB, in big-endian */
    uint8_t  payload[MAX_PAYLOAD_SIZE];
}__attribute((packed)) block_t;

struct INFO{
   uint16_t length;        /* length of the offset array, in big-endian */
   uint32_t offset[MAX_PAYLOAD_SIZE];     /* offset of the flags, in big-endian */
} __attribute((packed)) flag_t;

size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t totalSize = size * nmemb;
    output->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

int main(){
    string student = "110550080";
    string url = "https://inp.zoolab.org/binflag/challenge?id=" + student;
    //const char* outputFileName = "chals.bin"; 
    string wgetCommand = "wget -O " + string("chals.bin") + " " + url;
    int returnValue = system(wgetCommand.c_str());
    if(returnValue){cout<<"Download Success"<<endl;}

    const char *fi = "chals.bin";
    // const char *fi = "demo2.bin";
    printf("input file=%s\n", fi);
    int f = open(fi, O_RDONLY); 
    if(f<0){ return (-1);}

    struct HEAD head;
    int b_read = read(f, &head, sizeof(struct HEAD));
    uint32_t datasize = ntohl(head.datasize);
    head.n_blocks = ntohs (head.n_blocks);
    int num  = head.n_blocks;
    //printf("%zu\n", head.n_blocks);
    
    struct BLOCK *block = (struct BLOCK*)malloc(head.n_blocks * sizeof(struct BLOCK));
    off_t off = lseek(f, sizeof(struct HEAD), SEEK_SET);
    
    vector<pair<int,int>> vec;
    for (int i = 0; i < head.n_blocks; i++) {
        b_read = read(f, &block[i], sizeof(struct BLOCK)-MAX_PAYLOAD_SIZE);
        block[i].length = ntohs(block[i].length);
        
        block[i].offset = ntohl(block[i].offset);
        //pri ntf("%u  ",block[i].offset);
        block[i].cksum = ntohs(block[i].cksum);
        //printf("%zu\n",block[i].cksum);
        b_read = read(f, block[i].payload, block[i].length);
        int seg = block[i].length/2 ;
        //printf("%d \n",seg); 
        uint16_t chk = 0;
        for(int j=0 ; j<seg ; j++){
            uint16_t segment = 0;
            memcpy(&segment, block[i].payload + 2*j  , 2);
            segment = ntohs(segment);
            chk = chk^ segment;
        }
        int off = block[i].offset;
        if(chk == block[i].cksum){
            //printf("good: %lu  ",block[i].cksum);
            //printf("chk: %u\n",chk);
            vec.push_back({off, i});
        }       
    }
    sort(vec.begin(), vec.end());
   
    struct INFO info;
    string str;
    b_read = read(f, &info, sizeof(struct INFO)-4*MAX_PAYLOAD_SIZE);
    info.length = ntohs(info.length);
    //int vec_s = vec.size();
    for (int i=0 ; i< info.length ; i++){
        uint32_t tmp ;
        b_read = read(f, &tmp, sizeof(uint32_t));
        info.offset[i] = ntohl(tmp);
        int idx = info.offset[i]; 
        //printf("%lx\n", info.offset[i]);    
    }
    stringstream ss;
    for (int i=0 ; i< info.length ; i++){
        int idx = info.offset[i]; 
        for(int j=0 ; j<vec.size()-1 ; j++){
            int down = vec[j].first;
            int top = vec[j+1].first;
            int id = vec[j].second;
            if(idx>=down && idx<top){
                ss<<hex << setw(2)<<setfill('0')<<static_cast<uint16_t>(block[id].payload[idx-down]);
                ss<< hex <<setw(2)<<setfill('0')<<static_cast<uint16_t>(block[id].payload[idx-down+1]);
                break;
            }else if(j==vec.size()-2) {
                ss <<hex <<setw(2)<<setfill('0')<<static_cast<uint16_t>(block[vec[j+1].second].payload[idx-vec[j+1].first-1]);
                ss <<hex <<setw(2)<<setfill('0')<<static_cast<uint16_t>(block[vec[j+1].second].payload[idx-vec[j+1].first]);
                break;
            }else{continue;}
            
        } 
    }    
    string ans = ss.str();
    cout<<"ans: "<<ss.str()<<endl;
    close(f);
    string ver = "https://inp.zoolab.org/binflag/verify?v=" + ans;
    //cout<<"\n ver  "<<ver<<endl;
    CURL* curl = curl_easy_init();
    if (curl) {
        string responseData;
        curl_easy_setopt(curl, CURLOPT_URL, ver.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseData);
        CURLcode res = curl_easy_perform(curl);
        if (res == CURLE_OK) {
            cout << "Successful\n" << responseData << endl;
        } else {
            cerr << "Failed " << curl_easy_strerror(res) << endl;
        }
        curl_easy_cleanup(curl);
    }

      return 0;
}