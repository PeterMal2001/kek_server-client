#include<iostream>
#include<cstdlib>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include"frames.hpp"
using namespace std;

int main(int argc, char **argv){
    //startup check
    if(argc!=3){
        std::cout<<"Using ./client [address] [port]\n";
        return -1;
    }
    //socket
    int sd;
    size_t pass_hash;
    struct sockaddr_in addr;
    sd=socket(AF_INET,SOCK_STREAM,0);
    if(sd<0){
        perror("sock_init");
        return 1;
    }
    //connect
    addr.sin_family=AF_INET;
    addr.sin_addr.s_addr=inet_addr(argv[1]);
    addr.sin_port=atoi(argv[2]);
    if(connect(sd,reinterpret_cast<struct sockaddr*>(&addr),sizeof(addr))<0){
        perror("connect");
        return 1;
    }
    //operations
    int a,id,*recvrs,i;
    char com,*str1,*str2;
    time_t cur_time;
    std::string str;
    size_t hash_pass,base,code;
    std::hash<std::string> str_hash;
    id_frame usr_id;
    msg_frame msg;
    msglist_frame msglist;
    //service cycle
    while(1){
        std::cin>>a;
        com=a;
        send(sd,&com,1,0);
        //registration
        if(a==0){
            str1=new char[MAX_NAME];
            str2=new char[64];
            //frame setup
            std::cin>>str1;
            std::cin>>str2;
            hash_pass=str_hash(string(str2));
            usr_id.setup(str1,hash_pass);
            std::cout<<usr_id.name<<" "<<hash_pass<<" "<<usr_id.hash_pass<<"\n";
            //frame send
            send(sd,&usr_id,sizeof(id_frame),0);
            //answer recv
            recv(sd,&com,1,0);
            if(com==0){
                std::cout<<"Registered\n";
            }
            else if(com==1){
                std::cout<<"Existing name\n";
            }
            else if(com==2){
                std::cout<<"Incorrect name\n";
            }
            else{
                std::cout<<"Error\n";
            }
        }
        //authorization
        else if(a==1){
            str1=new char[MAX_NAME];
            str2=new char[64];
            //frame setup
            std::cin>>str1;
            std::cin>>str2;
            hash_pass=str_hash(string(str2));
            usr_id.setup(str1,hash_pass);
            //frame send
            usr_id.send_frame(sd);
            //answer recv
            recv(sd,&com,1,0);
            if(com==0){
                std::cout<<"Accepted\n";
            }
            else if(com==1){
                std::cout<<"Rejected\n";
            }
            else{
                std::cout<<"Error\n";
            }
        }
        //view incoming list
        else if(a==2){
            msglist.recv_frame(sd);
            msglist.print();
        }
        //send message
        else if(a==3){
            std::cin>>a;
            recvrs=new int[a];
            for(i=0;i<a;i++){
                std::cin>>recvrs[i];
            }
            str2=new char[MAX_MSG];
            str1=new char[MAX_TITLE];
            std::cin >> std::ws;
            getline(std::cin,str);
            std::strcpy(str1,str.c_str());
            getline(std::cin,str);
            std::strcpy(str2,str.c_str());
            cur_time=time(NULL);
            msg.setup(0,0,a,str1,cur_time,recvrs,str2);
            msg.print();
            msg.send_frame(sd);
            recv(sd,&com,1,0);
            if(com==1){
                std::cout<<"Message sent\n";
            }
        }
        else break;
    }
    return 0;
}