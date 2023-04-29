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
    int64_t sd;
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
    int64_t a,id,*recvrs,i;
    char *str1,*str2;
    char_frame com;
    int_frame icom;
    time_t cur_time;
    std::string str;
    size_t base,code;
    id_frame usr_id;
    msg_frame msg;
    msglist_frame msglist;
    //diffy-hellman
    TFKey ka,key,kmod(19662,16582360881486924019,5082632028705998950,895695448175102663);
    tfkey_frame keyframe;
    key.set_random();
    keyframe.data=key;
    keyframe.send_frame(sd);
    ka.set_random();
    keyframe.data=TFKey::pow_mod(key,ka,kmod);
    keyframe.send_frame(sd);
    keyframe.recv_frame(sd);
    key=TFKey::pow_mod(keyframe.data,ka,kmod);
    key.print("key");
    //service cycle
    #define SEND_FRAME(kframe) do{\
		if(!(kframe.send_frame(sd,key))){\
			return -1;\
		}\
        key_change(key);\
        key.print("key_change");\
	} while(0);
    #define RECV_FRAME(kframe) do{\
		if(!(kframe.recv_frame(sd,key))){\
			return -1;\
		}\
        key_change(key);\
        key.print("key_change");\
	} while(0);
    while(1){
        //shit for piping cin from file
        if(cin.eof()) break;
        std::cout<<"0 - register, 1 - authorisate, 2 - incoming list, 3 - send, 4 - view msg\n";
        std::cin>>a;
        com.data=a;
        SEND_FRAME(com);
        //registration
        if(a==0){
            str1=new char[MAX_NAME];
            str2=new char[MAX_PASS];
            //frame setup
            std::cin>>str1;
            std::cin>>str2;
            usr_id.setup(str1,str2);
            std::cout<<usr_id.name<<" "<<usr_id.pass<<"\n";
            //frame send
            SEND_FRAME(usr_id);
            //answer recv
            RECV_FRAME(com);
            if(com.data==0){
                std::cout<<"Registered\n";
            }
            else if(com.data==1){
                std::cout<<"Existing name\n";
            }
            else if(com.data==2){
                std::cout<<"Incorrect name\n";
            }
            else{
                std::cout<<"Error\n";
            }
        }
        //authorization
        else if(a==1){
            str1=new char[MAX_NAME];
            str2=new char[MAX_PASS];
            //frame setup
            std::cin>>str1;
            std::cin>>str2;
            usr_id.setup(str1,str2);
            //frame send
            SEND_FRAME(usr_id);
            //answer recv
            RECV_FRAME(com);
            if(com.data==0){
                std::cout<<"Accepted\n";
            }
            else if(com.data==1){
                std::cout<<"Rejected\n";
            }
            else{
                std::cout<<"Error\n";
            }
        }
        //view incoming list
        else if(a==2){
            RECV_FRAME(msglist);
            msglist.print();
        }
        //send message
        else if(a==3){
            std::cin>>a;
            recvrs=new int64_t[a];
            for(i=0;i<a;i++){
                std::cin>>recvrs[i];
            }
            str1=new char[MAX_TITLE];
            str2=new char[MAX_TEXT];
            std::cin >> std::ws;
            getline(std::cin,str);
            std::strcpy(str1,str.c_str());
            getline(std::cin,str);
            std::strcpy(str2,str.c_str());
            cur_time=time(NULL);
            msg.setup(0,0,a,str1,cur_time,recvrs,0,str2);
            msg.print();
            SEND_FRAME(msg);
            RECV_FRAME(com);
            if(com.data==0){
                std::cout<<"Message sent\n";
            }
            else{
                std::cout<<"Error\n";
            }
        }
        //view message
        else if(a==4){
            std::cin>>a;
            icom.data=a;
            std::cout<<icom.data;
            SEND_FRAME(icom);
            RECV_FRAME(msg);
            msg.print();
        }
        else break;
    }
    return 0;
}