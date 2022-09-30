#include<iostream>
#include<cstdlib>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<csignal>
#include<thread>
#include<mutex>
#include<pqxx/pqxx>

#define MAX_THREADS 5
#define MAX_HASH 16769023
#define MAX_NAME 64

using namespace std;

mutex keklock;

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

//check for legal symbols return 0 if good
bool namecheck(char *name){
	int i;
	for(i=0;i<MAX_NAME;i++){
		if(name[i]=='\0') break;
		else if (((name[i]>='A')&&(name[i]<='Z'))||((name[i]>='a')&&(name[i]<='z'))||(name[i]=='_')) continue;
		return 1;
	}
	return 0;
}

//handler for Ctrl+C
void kekstop(int signum){
	std::cout<<"I've been killed"<<endl;
	exit(0);
}

//client-server communication
void ktr_talk(int id,char *ip,pqxx::work *sql_work){
	std::cout<<"gay boy "<<ip<<" calling"<<endl;
	keklock.lock();
	char *login,ans;
	size_t base,hash_pass,code;
	bool is_authorised=0;
	int action_id;
	pqxx::result res;
	std::stringstream cmd;
	keklock.unlock();
	//handshake
	// recv(id,&base,sizeof(size_t),0);
	// recv(id,&code,sizeof(size_t),0);
	//service cycle
	while(1){
		//get action id
		if(!(recv(id,&ans,1,0))) break;
		action_id=ans;
		std::cout<<action_id<<"\n";
		//register
		if(ans==0){
			keklock.lock();
			login=new char[MAX_NAME];
			keklock.unlock();
			if(!(recv(id,login,64,0))) break;
			if(!(recv(id,&hash_pass,sizeof(size_t),0))) break;

			cmd<<"select hash_pass from users where name=\'"<<login<<"\'";
			keklock.lock();
			res=sql_work->exec(cmd);
			keklock.unlock();
			cmd.str("");

			if(namecheck(login)){
				std::cout<<"Illegal name\n";
				ans=2;
			}
			else if(res.size()>0){
				std::cout<<"Existing name\n";
				ans=1;
			}
			else{
				cmd<<"insert into users(name,hash_pass) values (\'"<<login<<"\',"<<hash_pass<<")";
				keklock.lock();
				res=sql_work->exec(cmd);
				keklock.unlock();
				cmd.str("");
				std::cout<<"Registration "<<login<<" with password "<<hash_pass<<"\n";
				ans=0;
			}
			send(id,&ans,1,0);
		}
		//authorisate
		else if(ans==1){
			keklock.lock();
			login=new char[MAX_NAME];
			keklock.unlock();
			if(!(recv(id,login,64,0))) break;
			if(!(recv(id,&hash_pass,sizeof(size_t),0))) break;
			//check
			keklock.lock();
			cmd<<"select hash_pass from users where name=\'"<<login<<"\'";
			res=sql_work->exec(cmd);
			cmd.str("");
			keklock.unlock();

			std::cout<<"Login attempt "<<login<<" with password "<<hash_pass<<"... ";
			if((res.size()==1)&&(hash_pass==res[0][0].as<size_t>())){
				ans=0;
				is_authorised=1;
				std::cout<<"accepted\n";
			}
			else{
				ans=1;
				std::cout<<"denied\n";
			}
			send(id,&ans,1,0);
		}
		else{
			std::cout<<"Incorrect action ID\n";
			break;
		}
	}
	sql_work->commit();
	std::cout<<"end of "<<ip<<" call"<<endl;
}

//socket class
class keksock{
	private:
	int sd,err;
	pqxx::work *sql_work;
	
	public:
	keksock(char* address,int port,pqxx::connection *sql_conn){
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
		addr.sin_addr.s_addr=inet_addr(address);
		addr.sin_port=port;
		//socket bind
		if(bind(sd,reinterpret_cast<struct sockaddr*>(&addr),sizeof(addr))<0){
			err+=2;
			perror("sock_bind");
		}
		//psql work init
		this->sql_work=new pqxx::work(*sql_conn);
		std::cout<<"Done"<<endl;
	}
	int monitor(){
		struct sockaddr_in addr;
		int ad;
		socklen_t junk;
		err=0;
		char *ip;
		ip=new char[256];
		while(1){
			if(listen(sd,MAX_THREADS)<0){
				err+=4;
				return 1;
			}
			ad=accept(sd,reinterpret_cast<sockaddr*>(&addr),&junk);
			ip=inet_ntoa(addr.sin_addr);
			thread read_thr(ktr_talk,ad,ip,sql_work);
			read_thr.detach();
		}
	}
};

int main(int argc, char** argv){
	if(SIZE_MAX<MAX_HASH){
		std::cout<<"size_t is too small!\n";
		return -1;
	}
	if(argc!=3){
		std::cout<<"Using ./server [address] [port]\n";
		return -1;
	}
	//psql connect
	pqxx::connection *sql_conn;
	try{
	sql_conn=new pqxx::connection("dbname=ktr_server_db hostaddr=127.0.0.1 port=5432 user=ktr_server password=cthdth");
	}
	catch(const std::exception &e){
		perror(e.what());
		return -1;
	}
	signal(SIGINT,kekstop);
	keksock lul(argv[1],atoi(argv[2]),sql_conn);
	lul.monitor();
	return 0;
}
