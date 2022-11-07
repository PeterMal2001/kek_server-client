#include<cstdlib>
#include<cstring>
#include<iostream>
#include<sys/types.h>
#include<sys/socket.h>

#define MAX_NAME 64
#define MAX_MSG 1024
#define MAX_TITLE 64

class frame{};

class id_frame:public frame{
    public:
    char *name;
    size_t hash_pass;
    id_frame(char *a,size_t b){
        this->name=new char[MAX_NAME];
        setup(a,b);
    }
    id_frame(){
        this->name=new char[MAX_NAME];
    }
    void setup(char *a,size_t b){
        strcpy(this->name,a);
        this->hash_pass=b;
    }
    void send_frame(int id){
        send(id,this->name,MAX_NAME,0);
        send(id,&(this->hash_pass),sizeof(size_t),0);
    }
    int recv_frame(int id){
        if(!(recv(id,this->name,MAX_NAME,0))) return 0;
        if(!(recv(id,&(this->hash_pass),sizeof(size_t),0))) return 0;
        return 1;
    }
    void print(){
        std::cout<<"Name="<<this->name<<"\nhash_pass"<<this->hash_pass<<"\n";
    }
};

class msg_header_frame: public frame{
    public:
    int id,sender_id,receivers_count;
    char *title;
    time_t time;
    int *receivers;
    msg_header_frame(){
        title=new char[MAX_TITLE];
    }
    void setup(int id, int sender_id,int receivers_count,const char *title,time_t time,const int *receivers){
        this->id=id;
        this->sender_id=sender_id;
        this->receivers_count=receivers_count;
        strcpy(this->title,title);
        this->time=time;
        this->receivers=new int[receivers_count];
        int i;
        for(i=0;i<receivers_count;i++){
            this->receivers[i]=receivers[i];
        }
    }
    void send_frame(int id){
        send(id,&(this->id),sizeof(int),0);
        send(id,&(this->sender_id),sizeof(int),0);
        send(id,&(this->receivers_count),sizeof(int),0);
        send(id,&(this->time),sizeof(time_t),0);
        send(id,this->title,64,0);
        send(id,this->receivers,this->receivers_count*sizeof(int),0);
    }
    int recv_frame(int id){
        if(!(recv(id,&(this->id),sizeof(int),0))) return 0;
        if(!(recv(id,&(this->sender_id),sizeof(int),0))) return 0;
        if(!(recv(id,&(this->receivers_count),sizeof(int),0))) return 0;
        if(!(recv(id,&(this->time),sizeof(time_t),0))) return 0;
        if(!(recv(id,this->title,64,0))) return 0;
        this->receivers=new int[receivers_count];
        if(!(recv(id,this->receivers,this->receivers_count*sizeof(int),0))) return 0;
        return 1;
    }
    void print(){
        std::cout<<"ID="<<this->id<<"\nSender ID="<<this->sender_id<<"\nTitle="<<this->title<<"\nTime="<<ctime(&(this->time))<<"Receivers Count="<<this->receivers_count<<"\nReceivers:\n";
        int i;
        for(i=0;i<this->receivers_count;i++){
            std::cout<<"["<<i<<"]"<<this->receivers[i]<<"\n";
        }
    }
};

class msg_frame:public msg_header_frame{
    public:
    char *text;
    msg_frame(){
        this->text=new char[MAX_MSG];
    }
    void setup(int id,int sender_id,int receivers_count,const char *title,time_t time,const int *receivers,const char *text){
        msg_header_frame::setup(id,sender_id,receivers_count,title,time,receivers);
        strcpy(this->text,text);
    }
    void send_frame(int id){
        msg_header_frame::send_frame(id);
        send(id,this->text,MAX_MSG,0);
    }
    int recv_frame(int id){
        msg_header_frame::recv_frame(id);
        if(!(recv(id,this->text,MAX_MSG,0))) return 0;
        return 1;
    }
    void print(){
        msg_header_frame::print();
        std::cout<<this->text<<"\n";
    }
};

class msglist_frame:public frame{
    public:
    int length,cur;
    msg_header_frame *list;
    msglist_frame(){
        this->length=0;
        this->cur=0;
    }
    msglist_frame(int length){
        this->length=length;
        this->cur=0;
        list=new msg_header_frame[length];
    }
    void add(msg_header_frame a){
        list[cur]=a;
        cur++;
    }
    void send_frame(int id){
        send(id,&(this->cur),sizeof(int),0);
        for(int i=0;i<this->cur;i++){
            list[i].send_frame(id);
        }
    }
    int recv_frame(int id){
        if(!(recv(id,&(this->length),sizeof(int),0))) return 0;
        this->cur=this->length;
        this->list=new msg_header_frame[this->length];
        for(int i=0;i<this->length;i++){
            list[i].recv_frame(id);
        }
        return 1;
    }
    void print(){
        std::cout<<"Length="<<this->length<<"\nCur="<<this->cur<<"\nHeaders:\n";
        int i;
        for(i=0;i<this->cur;i++){
            std::cout<<"{\n";
            list[i].print();
            std::cout<<"}\n";
        }
    }
};