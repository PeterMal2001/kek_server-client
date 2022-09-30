#include<iostream>
#include<cstdlib>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
using namespace std;

#define MAX_HASH 16769023
#define MAX_NAME 64

//powmod for D&F
size_t powmod(size_t a,size_t b){
	std::cout<<"powmod start\n";
	int i;
	for(i=0;i<b;i++){
		a*=a;
		if(a>MAX_HASH){
			a%=MAX_HASH;
		}
	}
	std::cout<<"powmod finish\n";
    return a;
}

int main(int argc, char **argv){
    if(SIZE_MAX<MAX_HASH){
		std::cout<<"size_t is too small!\n";
		return -1;
	}
    if(argc!=3){
        std::cout<<"Using ./client [address] [port]\n";
        return -1;
    }
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
    int a;
    char com,*str1,*str2;
    size_t hash_pass,base,code;
    std::hash<std::string> str_hash;
    //handshake
    // send(sd,&base,sizeof(size_t),0);
    // base=powmod(base,hash_pass);
    // send(sd,&base,sizeof(size_t),0);
    //service cycle
    while(1){
        std::cin>>a;
        com=a;
        send(sd,&com,1,0);
        //registration
        if(a==0){
            str1=new char[MAX_NAME];
            str2=new char[64];

            std::cin>>str1;
            std::cin>>str2;
            hash_pass=str_hash(string(str2))%MAX_HASH;
            send(sd,str1,64,0);
            send(sd,&hash_pass,sizeof(size_t),0);
            
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

            std::cin>>str1;
            std::cin>>str2;
            hash_pass=str_hash(string(str2))%MAX_HASH;
            send(sd,str1,64,0);
            send(sd,&hash_pass,sizeof(size_t),0);
            
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
        else break;
    }
    return 0;
}