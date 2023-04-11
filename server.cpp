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
	int64_t i;
	for(i=0;i<MAX_NAME;i++){
		if(name[i]=='\0') break;
		else if (((name[i]>='A')&&(name[i]<='Z'))||((name[i]>='a')&&(name[i]<='z'))||(name[i]=='_')) continue;
		return 1;
	}
	return 0;
}

//handler for Ctrl+C
void kekstop(int signum){
	std::cout<<"\nI've been killed\n";
	exit(0);
}

//convert from c++ boolean to sql boolean
std::string btosql(bool a){
	if(a) return "TRUE";
	else return "FALSE";
}
//convert from sql boolean to c++ bolean
bool sqltob(std::string a){
	if ((a=="t")||(a=="TRUE")) return 1;
	else return 0;
}

//thread of client-server communication
class talk_thread{
	private:
	int64_t id;
	int64_t auth_id;
	TFKey key;
	const char *ip;
	bool is_authorised=0;
	pqxx::connection *sql_conn;
	pqxx::work *sql_work;
	std::hash<std::string> str_hash;
	
	#define RECV_FRAME(kframe) do{\
		keklock.lock();\
		if(!(kframe.recv_frame(this->id,this->key))){\
			keklock.unlock();\
			return -1;\
		}\
		keklock.unlock();\
	} while(0);
	#define SEND_FRAME(kframe) do{\
		keklock.lock();\
		if(!(kframe.send_frame(this->id,this->key))){\
			keklock.unlock();\
			return -1;\
		}\
		keklock.unlock();\
	} while(0);
	#define EXEC(res) do{\
		keklock.lock();\
		res=sql_work->exec(cmd);\
		keklock.unlock();\
		cmd.str("");\
	} while(0);
	int64_t registration(){
		char_frame ans;
		pqxx::result res;
		std::stringstream cmd;
		id_frame usr_id;

		RECV_FRAME(usr_id);

		std::cout<<usr_id.name<<" "<<usr_id.pass<<"\n";
		
		//sql name collision check
		cmd<<"select hash_pass from users where name=\'"<<usr_id.name<<"\';";
		keklock.lock();
		res=sql_work->exec(cmd);
		keklock.unlock();
		cmd.str("");

		if(namecheck(usr_id.name)){
			std::cout<<"Illegal name\n";
			ans.data=2;
		}
		else if(res.size()>0){
			std::cout<<"Existing name\n";
			ans.data=1;
		}
		else{
			int64_t hash_pass=str_hash(usr_id.pass);
			cmd<<"insert into users(name,hash_pass) values (\'"<<usr_id.name<<"\',"<<hash_pass<<");";
			keklock.lock();
			res=sql_work->exec(cmd);
			keklock.unlock();
			cmd.str("");

			std::cout<<"Registration "<<usr_id.name<<" with password "<<usr_id.pass<<"\n";
			ans.data=0;
		}
		//send answer
		SEND_FRAME(ans);
		std::cout<<"end register\n";
		return ans.data;
	}
	int64_t authorisation(){
		char_frame ans;
		pqxx::result res;
		std::stringstream cmd;
		id_frame usr_id;

		RECV_FRAME(usr_id);

		if(strcmp(usr_id.name,"")==0){
			return -1;
		}

		//sql login and password check
		cmd<<"select id,hash_pass from users where name=\'"<<usr_id.name<<"\';";
		std::cout<<cmd.str()<<"\n";
		EXEC(res);

		std::cout<<cmd.str()<<"\n";
		std::cout<<"selected "<<res.size()<<"\n";

		int64_t hash_pass=str_hash(usr_id.pass);
		std::cout<<"Login attempt "<<usr_id.name<<" with password "<<hash_pass<<"... ";
		if((res.size()==1)&&(hash_pass==res[0][1].as<int64_t>())){
			ans.data=0;
			is_authorised=1;
			auth_id=res[0][0].as<int>();
			std::cout<<"accepted id:"<<auth_id<<"\n";
		}
		else{
			ans.data=1;
			std::cout<<"denied\n";
		}

		//send answer
		SEND_FRAME(ans);

		std::cout<<"auth end\n";
		return ans.data;
	}
	int64_t view_income(){
		char *cstr;
		int64_t i;
		pqxx::result res;
		std::stringstream cmd;
		msg_header_frame msg_header;
		msglist_frame *msglist;

		if(!is_authorised){
			std::cout<<"Not authorised\n";
			return -1;
		}

		cmd<<"SELECT id,sender_id,msg_title,send_time \
		FROM messages \
		WHERE id IN (SELECT id FROM message_receivers WHERE receiver_id="<<auth_id<<");";
		EXEC(res);
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

		SEND_FRAME((*msglist));

		return 0;
	}
	int64_t send_message(){
		char_frame ans;
		int64_t i;
		pqxx::result res;
		std::stringstream cmd;
		msg_frame msg;

		if(!is_authorised){
			std::cout<<"Not authorised\n";
			return 1;
		}

		RECV_FRAME(msg);

		msg.print();
		msg.sender_id=auth_id;
		msg.received=0;

		cmd<<"insert into messages(sender_id,msg_title,send_time,received,msg_text) values ("<<msg.sender_id<<",\'"<<msg.title<<"\',"<<msg.time<<","<<btosql(msg.received)<<",\'"<<msg.text<<"\') returning id;";
		std::cout<<"cmd made\n";

		EXEC(res);

		std::cout<<res.size()<<"\n";
		msg.id=res[0][0].as<int>();
		std::cout<<"sql msg "<<msg.id<<" insert done\n";

		for(i=0;i<msg.receivers_count;i++){
			cmd<<"insert into message_receivers(msg_id,receiver_id) values ("<<msg.id<<","<<msg.receivers[i]<<");";
			keklock.lock();
			sql_work->exec(cmd);
			keklock.unlock();
			cmd.str("");
		}
		std::cout<<"Message sent\n";
		ans.data=0;

		SEND_FRAME(ans);

		return 0;
	}
	int64_t view_message(){
		int_frame ans;
		char *cstr,*cstr2;
		int64_t i,*receivers;
		pqxx::result res,res2;
		std::stringstream cmd;
		msg_frame msg;

		if(!is_authorised){
			std::cout<<"Not authorised\n";
			return -1;
		}

		RECV_FRAME(ans);
		std::cout<<ans.data<<"\n";

		cmd<<"select sender_id,msg_title,send_time,received,msg_text from messages \
		where \
		id="<<ans.data<<" \
		and (\
			id in (\
				select msg_id from message_receivers \
				where receiver_id="<<auth_id<<") \
			or \
			sender_id="<<auth_id<<");";
		std::cout<<"cmd made "<<cmd.str()<<"\n";

		EXEC(res);

		std::cout<<"message select "<<res.size()<<"\n";

		if(res.size()!=1){
			return -1;
		}

		cmd<<"select receiver_id from message_receivers \
		where msg_id="<<ans.data<<";";
		std::cout<<"cmd made "<<cmd.str()<<"\n";

		EXEC(res2);

		std::cout<<"receivers select"<<res2.size()<<"\n";

		receivers=new int64_t[res2.size()];
		for(i=0;i<res2.size();i++){
			receivers[i]=res2[i][0].as<int>();
			std::cout<<receivers[i]<<"\n";
		}
		cstr=new char[MAX_TITLE];
		cstr2=new char[MAX_TEXT];
		strcpy(cstr,res[0][1].as<std::string>().c_str());
		strcpy(cstr2,res[0][4].as<std::string>().c_str());
		std::cout<<"title = "<<cstr<<" msg = "<<cstr2<<"\n";
		msg.setup(ans.data,res[0][0].as<int>(),res2.size(),cstr,res[0][2].as<time_t>(),receivers,sqltob(res[0][3].as<std::string>()),cstr2);

		SEND_FRAME(msg);

		return 0;
	}
	#define RECV_FRAME(kframe) do{\
		keklock.lock();\
		if(!(kframe.recv_frame(this->id))){\
			keklock.unlock();\
			return -1;\
		}\
		keklock.unlock();\
	} while(0);
	#define SEND_FRAME(kframe) do{\
		keklock.lock();\
		if(!(kframe.send_frame(this->id))){\
			keklock.unlock();\
			return -1;\
		}\
		keklock.unlock();\
	} while(0);
	int64_t diffy_hellman(){
		TFKey ka,key,kmod(19662,16582360881486924019,5082632028705998950,895695448175102663);
		tfkey_frame keyframe;
		ka.set_random();
		RECV_FRAME(keyframe);
		key=TFKey::pow_mod(keyframe.data,ka,kmod);
		RECV_FRAME(keyframe);
		ka=TFKey::pow_mod(keyframe.data,ka,kmod);
		keyframe.data=key;
		SEND_FRAME(keyframe);
		key=ka;
		key.print("key");
		this->key=key;
		return 0;
	}
	#undef RECV_FRAME
	#undef SEND_FRAME
	#undef EXEC

	public:
	int64_t is_active=0;

	void setup(int64_t id,const char *ip,pqxx::connection *sql_conn){
		this->id=id;
		this->ip=ip;
		this->sql_conn=sql_conn;
	}
	void monitor(){
		char_frame ans;
		int64_t e;

		std::cout<<"Start of "<<this->ip<<" call\n";
		keklock.lock();
		this->is_active=1;
		//diffy-hellman
		keklock.unlock();
		this->diffy_hellman();
		while(1){
			sql_work=new pqxx::work(*sql_conn);
			//get action id
			keklock.lock();\
			if(!(ans.recv_frame(this->id,this->key))){
				keklock.unlock();
				break;
			}
			keklock.unlock();
			std::cout<<(int)ans.data<<"\n";
			if(ans.data==0) e=this->registration();
			else if(ans.data==1) e=this->authorisation();
			else if(ans.data==2) e=this->view_income();
			else if(ans.data==3) e=this->send_message();
			else if(ans.data==4) e=this->view_message();
			else break;
			if(!e){
				this->sql_work->commit();
			}
			else{
				delete sql_work;
			}
		}
		std::cout<<"break\n";
		delete sql_work;
		keklock.lock();
		this->is_active=0;
		keklock.unlock();
		std::cout<<"End of "<<this->ip<<" call\n";
	}
};

//because threads do not eat methods
void ktr_talk(talk_thread *a){
	a->monitor();
}

//socket class
class keksock{
	private:
	int64_t sd,err;
	pqxx::connection *sql_conn;
	talk_thread *thr_list=new talk_thread[MAX_THREADS];
	
	public:
	keksock(char* address,int64_t port,pqxx::connection *sql_conn){
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
	int64_t monitor(){
		struct sockaddr_in addr;
		int64_t ad,i;
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
			for(i=0;i<MAX_THREADS;i++){
				if(thr_list[i].is_active==0){
					ad=accept(sd,reinterpret_cast<sockaddr*>(&addr),&junk);
					ip=inet_ntoa(addr.sin_addr);
					thr_list[i].setup(ad,ip,sql_conn);
					std::cout<<"starting thread\n";
					thread read_thr(ktr_talk,&(thr_list[i]));
					read_thr.detach();
				}
			}
		}
	}
};

//main func
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