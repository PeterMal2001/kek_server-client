#include<iostream>
#include<cstdlib>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<csignal>
#include<thread>
#include<mutex>

#define MAX_THREADS 5

using namespace std;

//global descryptor for socket
int sd,tc=0;

mutex keklock;

//handler for Ctrl+C
void kekstop(int signum){
	cout<<"I've been killed"<<endl;
	exit(0);
}

void talk(int id,char *ip){
	cout<<"gay boy "<<ip<<" calling"<<endl;
	keklock.lock();
	char *msg;
	int c;
	msg=static_cast<char*>(malloc(1024));
	keklock.unlock();
	while((c=recv(id,msg,1024,0))>0){
		cout<<ip<<" said: "<<msg<<" of "<<c<<endl;
	}
	cout<<"end of "<<ip<<" call"<<endl;
}

//socket class
class keksock{
	private:
	int sd,err;
	
	public:
	keksock(int port){
		struct sockaddr_in addr;
		err=0;
		//socket init
		sd=socket(AF_INET,SOCK_STREAM,0);
		if(sd<0){
			err+=1;
			perror("sock_init");
		}
		//address set
		addr.sin_family=AF_INET;
		addr.sin_addr.s_addr=INADDR_ANY;
		addr.sin_port=port;
		//socket bind
		if(bind(sd,reinterpret_cast<struct sockaddr*>(&addr),sizeof(addr))<0){
			err+=2;
			perror("sock_bind");
		}
		cout<<"Done"<<endl;
	}
	int monitor(){
		struct sockaddr_in addr;
		int ad;
		socklen_t junk;
		err=0;
		char *ip;
		ip=static_cast<char*>(malloc(256));
		while(1){
			if(listen(sd,5)<0){
				err+=4;
				return 1;
			}
			ad=accept(sd,reinterpret_cast<sockaddr*>(&addr),&junk);
			ip=inet_ntoa(addr.sin_addr);
			thread thr(talk,ad,ip);
			thr.detach();
		}
	}
};

int main(int argc, char** argv){
	keksock lul(6666);
	lul.monitor();
	return 0;
}
