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
#include"aux.hpp"
#include"consts.hpp"

using namespace std;

mutex keklock;
std::ofstream flog("server.log",_S_app);

void m_log(string msg){
	time_t t=time(NULL);
	tm *lt=localtime(&t);
	flog<<"["<<lt->tm_year+1900<<"."<<lt->tm_mon<<"."<<lt->tm_mday<<" "<<lt->tm_hour<<":"<<lt->tm_min<<":"<<lt->tm_sec<<"] "<<msg<<"\n";
}

//handler for Ctrl+C
void kekstop(int signum){
	m_log("Server shutdown (CTRL+C)\n");
	std::cout<<"\nI've been killed\n";
	exit(0);
}

//thread of client-server communication
class talk_thread{
	private:
	int64_t id;
	int64_t auth_id;
	TFKey key;
	char *ip;
	bool is_authorised=0;
	pqxx::connection *sql_conn;
	pqxx::work *sql_work;
	string* log_context;
	string log_head;
	
	void log(string msg){
		stringstream m;
		m<<*log_context<<log_head<<msg;
		m_log(m.str());
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
	/// @brief diffie-hellman keys exchange
	/// @return 0 if ok, 1 if something wrong
	int64_t diffy_hellman(){
		TFKey ka,key,kmod(19662,16582360881486924019,5082632028705998950,895695448175102663);
		tfkey_frame keyframe;
		char_frame check;
		ka.set_random();
		RECV_FRAME(keyframe);
		RECV_FRAME(check);
		if(check.data!=CHECK_CHAR) return 1;
		key=TFKey::pow_mod(keyframe.data,ka,kmod);
		RECV_FRAME(keyframe);
		RECV_FRAME(check);
		if(check.data!=CHECK_CHAR) return 1;
		ka=TFKey::pow_mod(keyframe.data,ka,kmod);
		keyframe.data=key;
		SEND_FRAME(keyframe);
		check.data=CHECK_CHAR;
		SEND_FRAME(check);
		key=ka;
		//key.print("key");
		this->key=key;
		return 0;
	}
	#undef RECV_FRAME
	#undef SEND_FRAME
	#define RECV_FRAME(kframe) do{\
		if(!(kframe.recv_frame(this->id,this->key))){\
			return -1;\
		}\
		key_change(key);\
	} while(0);
	#define SEND_FRAME(kframe) do{\
		if(!(kframe.send_frame(this->id,this->key))){\
			return -1;\
		}\
		key_change(key);\
	} while(0);
	#define EXEC(res) do{\
		std::cout<<"cmd made "<<cmd.str()<<"\n";\
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
		TFKey kmod(19662,16582360881486924019,5082632028705998950,895695448175102663);

		RECV_FRAME(usr_id);

		std::cout<<usr_id.name<<" "<<usr_id.pass<<"\n";
		
		//check for legal name
		if(namecheck(usr_id.name)){
			std::cout<<"Illegal name\n";
			ans.data=2;
			SEND_FRAME(ans);
			std::cout<<"end register\n";
			return ans.data;
		}

		//sql name collision check
		cmd<<"select hash_pass from users where name=\'"<<usr_id.name<<"\';";
		keklock.lock();
		res=sql_work->exec(cmd);
		keklock.unlock();
		cmd.str("");

		if(res.size()>0){
			std::cout<<"Existing name\n";
			ans.data=1;
		}
		else{
			TFKey hash_pass=TFKey::pow_mod(make_crc(usr_id.name,MAX_NAME),make_crc(usr_id.pass,MAX_PASS),kmod);
			cmd<<"insert into users(name,hash_pass) values (\'"<<usr_id.name<<"\',\'{"<<(int64_t)hash_pass.get_data(3)<<","<<(int64_t)hash_pass.get_data(2)<<","<<(int64_t)hash_pass.get_data(1)<<","<<(int64_t)hash_pass.get_data(0)<<"}\');";
			EXEC(res);

			std::cout<<"Registration "<<usr_id.name<<" with password "<<usr_id.pass<<"\n";
			hash_pass.print("hash_pass");
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
		TFKey kmod(19662,16582360881486924019,5082632028705998950,895695448175102663);

		RECV_FRAME(usr_id);

		//check for legal name
		if(namecheck(usr_id.name)){
			ans.data=1;
			std::cout<<"denied\n";
			SEND_FRAME(ans);
			std::cout<<"auth end\n";
			return ans.data;
		}

		//sql login and password check
		cmd<<"select id,hash_pass[1],hash_pass[2],hash_pass[3],hash_pass[4] from users where name=\'"<<usr_id.name<<"\';";
		std::cout<<cmd.str()<<"\n";		EXEC(res);

		std::cout<<cmd.str()<<"\n";
		std::cout<<"selected "<<res.size()<<"\n";

		TFKey hash_pass=TFKey::pow_mod(make_crc(usr_id.name,MAX_NAME),make_crc(usr_id.pass,MAX_PASS),kmod);
		hash_pass.print("hash_pass");
		std::cout<<"Login attempt "<<usr_id.name<<" with password "<<usr_id.pass<<"... ";
		if((res.size()==1)&&(hash_pass==TFKey((u_int64_t)(res[0][1].as<int64_t>()),(u_int64_t)(res[0][2].as<int64_t>()),(u_int64_t)(res[0][3].as<int64_t>()),(u_int64_t)(res[0][4].as<int64_t>())))){
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
		WHERE id IN (SELECT msg_id FROM message_receivers WHERE receiver_id="<<auth_id<<");";		EXEC(res);
		std::cout<<"sql done\n";

		msglist=new msglist_frame(res.size());
		cstr=new char[MAX_TITLE];
		for(i=0;i<res.size();i++){
			strcpy(cstr,res[i][2].as<std::string>().c_str());
			std::cout<<"add "<<i<<" "<<res[i][0].as<int>()<<" "<<res[i][1].as<int>()<<" "<<1<<" "<<cstr<<" "<<res[i][3].as<time_t>()<<"\n";
			msg_header.setup(res[i][0].as<int>(),res[i][1].as<int>(),1,cstr,res[i][3].as<time_t>(),&auth_id);
			msglist->add(msg_header);
			msglist->print();
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

		//recv message
		RECV_FRAME(msg);

		msg.print();
		msg.sender_id=auth_id;
		msg.received=0;
		char *esc_text=new char[2*MAX_TEXT];
		textescape(esc_text,msg.text);

		//add message to sql
		cmd<<"insert into messages(sender_id,msg_title,send_time,received,msg_text) values ("<<msg.sender_id<<",\'"<<msg.title<<"\',"<<msg.time<<","<<btosql(msg.received)<<",\'"<<esc_text<<"\') returning id;";
		delete[] esc_text;
		std::cout<<"cmd made\n";
		EXEC(res);

		std::cout<<res.size()<<"\n";
		msg.id=res[0][0].as<int>();
		std::cout<<"sql msg "<<msg.id<<" insert done\n";
		
		//add receivers to sql
		for(i=0;i<msg.receivers_count;i++){
			cmd<<"select count(*) from users where id="<<msg.receivers[i]<<";";
			EXEC(res);

			if(res[0][0].as<int64_t>()==1){
				cmd<<"insert into message_receivers(msg_id,receiver_id) values ("<<msg.id<<","<<msg.receivers[i]<<");";
				EXEC(res);
			}
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
		EXEC(res);

		std::cout<<"message select "<<res.size()<<"\n";

		if(res.size()!=1){
			return -1;
		}

		cmd<<"select receiver_id from message_receivers \
		where msg_id="<<ans.data<<";";
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

		cmd<<"update messages set received=TRUE where id="<<ans.data<<";";
		EXEC(res);

		return 0;
	}
	int64_t account_exit(){
		is_authorised=0;
		std::cout<<"account "<<auth_id<<" exited\n";
		return 0;
	}
	int64_t status(){
		int_frame com;
		if(!is_authorised) com.data=0;
		else com.data=1;
		SEND_FRAME(com);
		if(is_authorised){
			com.data=auth_id;
			SEND_FRAME(com);
		}
		return 0;
	}
	int64_t delete_message(){
		int_frame msg_id;
		pqxx::result res,res2;
		std::stringstream cmd;

		if(!is_authorised){
			std::cout<<"Not authorised\n";
			return -1;
		}

		//check ownership
		RECV_FRAME(msg_id)

		cmd<<"select receiver_id from message_receivers \
		where msg_id="<<msg_id.data<<";";
		EXEC(res);

		if(!(res[0][0].as<int64_t>()==auth_id)){
			std::cout<<"Not owned\n";
			return -1;
		}
		
		//delete from message_receivers
		cmd<<"delete from message_receivers where (msg_id="<<msg_id.data<<" and receiver_id="<<auth_id<<");";
		EXEC(res);

		//delete from messages if no more receivers
		cmd<<"select count(*) from message_receivers where msg_id="<<msg_id.data<<";";
		EXEC(res);

		if(res[0][0].as<int64_t>()==0){
			cmd<<"delete from messages where id="<<msg_id.data<<";";
			std::cout<<"cmd made "<<cmd.str()<<"\n";

			EXEC(res);
		}
		
		return 0;
	}
	#undef EXEC
	#undef RECV_FRAME
	#undef SEND_FRAME

	public:
	int64_t is_active=0;

	int64_t setup(int64_t id,const char *ip, string* log_head){
		this->id=id;
		this->ip=new char[20];
		this->log_context=log_head;
		strcpy(this->ip,ip);
		try{
		this->sql_conn=new pqxx::connection("dbname=ktr_server_db hostaddr=127.0.0.1 port=5432 user=ktr_server password=cthdth");
		}
		catch(const std::exception &e){
			perror(e.what());
			return -1;
		}
		
		stringstream m;
		m<<"[conn:"<<id<<",client:"<<ip<<"] ";
		this->log_head=m.str();

		this->log("Thread setup complete");
		return 0;
	}
	void monitor(){
		char_frame ans;
		int64_t e;

		this->log("Monitoring started");
		std::cout<<"Start of "<<this->ip<<" call\n";
		keklock.lock();
		this->is_active=1;
		//diffy-hellman
		keklock.unlock();
		if(this->diffy_hellman()) this->is_active=0;
		while(this->is_active){
			sql_work=new pqxx::work(*sql_conn);
			//get action id
			if(!(ans.recv_frame(this->id,this->key))){
				break;
			}
			key_change(key);
			// key.print("key_change");
			std::cout<<(int)ans.data<<"\n";
			if(ans.data==0) e=this->registration();
			else if(ans.data==1) e=this->authorisation();
			else if(ans.data==2) e=this->view_income();
			else if(ans.data==3) e=this->send_message();
			else if(ans.data==4) e=this->view_message();
			else if(ans.data==5) e=this->account_exit();
			else if(ans.data==6) e=this->status();
			else if(ans.data==7) e=this->delete_message();
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
		std::cout<<"End of "<<this->ip<<" call "<<this<<"\n";
		this->log("Monitoring finished");
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
	talk_thread *thr_list=new talk_thread[MAX_THREADS];
	char* address;
	int64_t port;
	string log_head;
	
	public:
	keksock(char* address,int64_t port){
		this->port=port;

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

		this->address=new char[20];
		strcpy(this->address,address);
		
		stringstream m;
		m<<"[sock:"<<this->sd<<",addr:"<<this->address<<",port:"<<this->port<<"] ";
		this->log_head=m.str();
		this->log("Socket binded");
		std::cout<<"Done\n";
	}
	void log(string msg){
		stringstream m;
		m<<log_head<<" "<<msg;
		m_log(m.str());
	}
	int64_t monitor(){
		struct sockaddr_in addr;
		int64_t ad,i;
		socklen_t junk;
		err=0;
		char *ip;
		ip=new char[20];
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
					if(thr_list[i].setup(ad,ip,&log_head)) break;
					std::cout<<"starting thread "<<i<<"\n";
					thread read_thr(ktr_talk,&(thr_list[i]));
					read_thr.detach();
				}
			}
		}
	}
};

//main func
int main(int argc, char** argv){
	m_log("Server start");
	//startup check
	// if(SIZE_MAX<MAX_HASH){
	// 	std::cout<<"size_t is too small!\n";
	// 	return -1;
	// }
	if(argc!=3){
		std::cout<<"Using ./server [address] [port]\n";
		return -1;
	}
	//signal setup
	signal(SIGINT,kekstop);
	//main class init
	keksock lul(argv[1],atoi(argv[2]));
	lul.monitor();
	return 0;
}