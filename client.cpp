#include<iostream>
#include<cstdlib>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
using namespace std;

int main(){
    int sd;
    struct sockaddr_in addr;
    char *msg;
    msg=static_cast<char*>(malloc(1024));
    sd=socket(AF_INET,SOCK_STREAM,0);
    if(sd<0){
        perror("sock_init");
        return 1;
    }

    addr.sin_family=AF_INET;
    addr.sin_addr.s_addr=INADDR_ANY;
    addr.sin_port=6666;
    if(connect(sd,reinterpret_cast<struct sockaddr*>(&addr),sizeof(addr))<0){
        perror("connect");
        return 1;
    }

    cin.getline(msg,1024);
    while(msg[0]!='0'){
        send(sd,msg,1024,0);
        cin.getline(msg,1024);
    }
    return 0;
}