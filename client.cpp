#include<iostream>
#include<cstdlib>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
using namespace std;

int main(int argc, char **argv){
    int sd;
    struct sockaddr_in addr;
    char *msg,*ans;
    ans=new char[1024];
    msg=new char[1024];
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
    //write-read
    cin.getline(msg,1024);
    while(msg[0]!='0'){
        send(sd,msg,1024,0);
        recv(sd,ans,1024,0);
        cout<<"The answer is: "<<msg<<"\n";
        cin.getline(msg,1024);
    }
    return 0;
}