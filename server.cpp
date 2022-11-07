#include<iostream>
#include<cstdlib>
#include<sstream>

#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<csignal>
#include<thread>
#include<mutex>
#include<pqxx/pqxx>

#include"frames.hpp"

#define MAX_THREADS 5

using namespace std;

mutex keklock;

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
void ktr_talk(int id,char *ip,pqxx::connection *sql_conn){
	std::cout<<"gay boy "<<ip<<" calling"<<endl;
	keklock.lock();
	char ans,*cstr;
	std::string str;
	size_t base,hash_pass,code;
	bool is_authorised=0;
	int action_id,auth_id,i;
	pqxx::work *sql_work;
	pqxx::result res;
	std::stringstream cmd;
	id_frame usr_id;
	msg_frame msg;
	msg_header_frame msg_header;
	msglist_frame *msglist;
	keklock.unlock();
	//handshake
	// recv(id,&base,sizeof(size_t),0);
	// recv(id,&code,sizeof(size_t),0);
	//service cycle
	while(1){
		sql_work=new pqxx::work(*sql_conn);
		//get action id
		if(!(recv(id,&ans,1,0))) break;
		action_id=ans;
		std::cout<<action_id<<"\n";
		//register
		if(ans==0){
			if(!(recv(id,&usr_id,sizeof(id_frame),0))) break;
			std::cout<<usr_id.name<<" "<<usr_id.hash_pass<<"\n";
			
			//sql name collision check
			cmd<<"select hash_pass from users where name=\'"<<usr_id.name<<"\';";
			keklock.lock();
			res=sql_work->exec(cmd);
			keklock.unlock();
			cmd.str("");

			if(namecheck(usr_id.name)){
				std::cout<<"Illegal name\n";
				ans=2;
			}
			else if(res.size()>0){
				std::cout<<"Existing name\n";
				ans=1;
			}
			else{
				cmd<<"insert into users(name,hash_pass) values (\'"<<usr_id.name<<"\',"<<usr_id.hash_pass<<");";
				keklock.lock();
				res=sql_work->exec(cmd);
				keklock.unlock();
				cmd.str("");
				std::cout<<"Registration "<<usr_id.name<<" with password "<<usr_id.hash_pass<<"\n";
				ans=0;
			}
			//send answer
			send(id,&ans,1,0);
		}
		//authorisate
		else if(ans==1){
			std::cout<<"recv\n";
			if(!(usr_id.recv_frame(id))) break;
			//sql login and password check
			keklock.lock();
			cmd<<"select id,hash_pass from users where name=\'"<<usr_id.name<<"\';";
			std::cout<<cmd.str()<<"\n";
			res=sql_work->exec(cmd);
			keklock.unlock();
			cmd.str("");
			std::cout<<cmd.str()<<"\n";
			std::cout<<"selected "<<res.size()<<"\n";

			std::cout<<"Login attempt "<<usr_id.name<<" with password "<<usr_id.hash_pass<<"... ";
			if((res.size()==1)&&(usr_id.hash_pass==res[0][1].as<size_t>())){
				ans=0;
				is_authorised=1;
				auth_id=res[0][0].as<int>();
				std::cout<<"accepted id:"<<auth_id<<"\n";
			}
			else{
				ans=1;
				std::cout<<"denied\n";
			}
			//send answer
			send(id,&ans,1,0);
		}
		//view incoming list
		else if(ans==2){
			if(!is_authorised){
				std::cout<<"Not authorised\n";
				break;
			}

			keklock.lock();
			cmd<<"SELECT id,sender_id,msg_title,send_time \
			FROM messages \
			WHERE id IN (SELECT id FROM message_receivers WHERE receiver_id="<<auth_id<<");";
			res=sql_work->exec(cmd);
			keklock.unlock();
			std::cout<<"sql done\n";

			msglist=new msglist_frame(res.size());
			cstr=new char[MAX_TITLE];
			for(i=0;i<res.size();i++){
				strcpy(cstr,res[i][2].as<std::string>().c_str());
				std::cout<<"add "<<i<<" "<<res[i][0].as<int>()<<" "<<res[i][1].as<int>()<<" "<<1<<" "<<cstr<<" "<<res[i][3].as<time_t>()<<"\n";
				msg_header.setup(res[i][0].as<int>(),res[i][1].as<int>(),1,cstr,res[i][3].as<time_t>(),&auth_id);
				msglist->add(msg_header);
			}
			std::cout<<"add end\n";
			msglist->print();
			msglist->send_frame(id);
		}
		//send msg
		else if(ans==3){
			if(!is_authorised){
				std::cout<<"Not authorised\n";
				break;
			}
			if(!(msg.recv_frame(id))) break;
			msg.print();
			msg.sender_id=auth_id;

			cmd<<"insert into messages(sender_id,msg_title,send_time,msg_text) values ("<<msg.sender_id<<",\'"<<msg.title<<"\',"<<msg.time<<",\'"<<msg.text<<"\') returning id;";
			std::cout<<"cmd made\n";
			keklock.lock();
			res=sql_work->exec(cmd);
			keklock.unlock();
			cmd.str("");
			std::cout<<res.size()<<"\n";
			msg.id=res[0][0].as<int>();
			std::cout<<"sql msg "<<msg.id<<" insert done\n";

			for(i=0;i<msg.receivers_count;i++){
				keklock.lock();
				cmd<<"insert into message_receivers(msg_id,receiver_id) values ("<<msg.id<<","<<msg.receivers[i]<<");";
				sql_work->exec(cmd);
				cmd.str("");
				keklock.unlock();
			}
			std::cout<<"Message sent\n";
			ans=1;
			send(id,&ans,1,0);
		}
		//invalid action
		else{
			std::cout<<"Incorrect action ID\n";
			break;
		}
		sql_work->commit();
	}
	std::cout<<"end of "<<ip<<" call"<<endl;
}

//socket class
class keksock{
	private:
	int sd,err;
	pqxx::connection *sql_conn;
	
	public:
	keksock(char* address,int port,pqxx::connection *sql_conn){
		struct sockaddr_in addr;
		err=0;
		//socket
		sd=socket(AF_INET,SOCK_STREAM,0);
		if(sd<0){
			err+=1;
			perror("sock_init");
		}
		//bind
		addr.sin_family=AF_INET;
		addr.sin_addr.s_addr=inet_addr(address);
		addr.sin_port=port;
		if(bind(sd,reinterpret_cast<struct sockaddr*>(&addr),sizeof(addr))<0){
			err+=2;
			perror("sock_bind");
		}
		//psql work init
		this->sql_conn=sql_conn;
		std::cout<<"Done\n";
	}
	int monitor(){
		struct sockaddr_in addr;
		int ad;
		socklen_t junk;
		err=0;
		char *ip;
		ip=new char[256];
		//main cycle
		while(1){
			if(listen(sd,MAX_THREADS)<0){
				err+=4;
				return 1;
			}
			ad=accept(sd,reinterpret_cast<sockaddr*>(&addr),&junk);
			ip=inet_ntoa(addr.sin_addr);
			thread read_thr(ktr_talk,ad,ip,sql_conn);
			read_thr.detach();
		}
	}
};

int main(int argc, char** argv){
	//startup check
	// if(SIZE_MAX<MAX_HASH){
	// 	std::cout<<"size_t is too small!\n";
	// 	return -1;
	// }
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
	//signal setup
	signal(SIGINT,kekstop);
	//main class init
	keksock lul(argv[1],atoi(argv[2]),sql_conn);
	lul.monitor();
	return 0;
}
