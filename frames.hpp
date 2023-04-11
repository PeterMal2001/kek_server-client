#include<cstdlib>
#include<cstring>
#include<iostream>
#include<sys/types.h>
#include<sys/socket.h>
#include"crypto.hpp"

#define MAX_NAME 64
#define MAX_PASS 64
#define MAX_TEXT 1024
#define MAX_TITLE 64

#define RECV(id,arg,size) do{\
    if(!(recv(id,arg,size,0))) return 0;\
} while(0);

#define BLOB_RECV(id) do{\
    if(!(blob.blob_recv(id))) return 0;\
} while(0);

#define SEND(id,arg,size) do{\
    if(send(id,arg,size,0)==-1) return 0;\
} while(0);

#define BLOB_SEND(id) do{\
    if(!(blob.blob_send(id))) return 0;\
} while(0);

class Blob{
    public:
    Blob(size_t size){
        this->size=size;
        this->cur=0;
        this->is_encrypted=0;
        data=new char[size+sizeof(size_t)+sizeof(TFKey)];
        this->push<size_t>(size);
    }
    Blob(){
        this->cur=0;
        this->is_encrypted=0;
    }

    template<typename Data>
    void push(Data a){
        push<Data>(a,this->cur);
    }
    template<typename Data>
    void push_array(Data* a, size_t len){
        for(size_t i=0;i<len;i++){
            push<Data>(a[i]);
        }
    }
    void add_crc(){
        this->crc=make_crc(this->data,this->size+sizeof(size_t));
        size_t pos=this->size+sizeof(size_t);
        this->push<TFKey>(this->crc,pos);
    }
    void encrypt(TFKey key){
        CryptedStruct a;
        a=TF_encrypt(this->data,this->size+sizeof(size_t)+sizeof(TFKey),key);
        this->size=a.size;
        this->data=new char[this->size];
        charcpy(this->data,a.data,this->size);
        this->is_encrypted=1;
    }
    int64_t blob_send(int64_t id){
        SEND(id,&(this->is_encrypted),sizeof(bool));
        if(this->is_encrypted){
            SEND(id,&(this->size),sizeof(size_t));
            SEND(id,this->data,this->size);
        }
        else{
            size_t fullsize=this->size+sizeof(size_t)+sizeof(TFKey);
            SEND(id,&(fullsize),sizeof(size_t));
            SEND(id,this->data,fullsize);
        }
        return 1;
    }
    int64_t blob_recv(int64_t id){
        RECV(id,&(this->is_encrypted),sizeof(bool));
        // std::cout<<"encrypted "<<this->is_encrypted<<"\n";
        RECV(id,&(this->size),sizeof(size_t));
        // std::cout<<"size "<<this->size<<"\n";
        this->data=new char[this->size];
        RECV(id,this->data,this->size);
        if(!this->is_encrypted){
            this->size-=sizeof(size_t)+sizeof(TFKey);
            this->cur=sizeof(size_t)+this->size;
            this->crc=this->pop<TFKey>();
        }
        this->cur=sizeof(size_t);
        return 1;
    }
    void decrypt(TFKey key){
        CryptedStruct a(this->size);
        charcpy(a.data,this->data,this->size);
        //no more room for initial vector
        this->size-=32;
        this->data=new char[this->size];
        charcpy(this->data,TF_decrypt<char>(a,key),this->size);
        this->cur=0;
        this->size=pop<size_t>();
        this->cur=sizeof(size_t)+this->size;
        this->crc=pop<TFKey>();
        this->cur=sizeof(size_t);
        this->is_encrypted=0;
    }
    //checks crc of blob if written same as freshly made then returns 1 else 0
    bool check_crc(){
        TFKey rcrc=make_crc(this->data,this->size+sizeof(size_t));
        this->crc.print("ccrc");
        rcrc.print("rcrc");
        return this->crc==rcrc;
    }
    template<typename Data>
    Data pop(){
        return pop<Data>(this->cur);
    }
    template<typename Data>
    void pop_array(Data *dst,size_t len){
            for(size_t i=0;i<len;i++){
                dst[i]=pop<Data>();
            }
            // std::cout<<"got "<<(char*)dst<<"\n";
    }

    void print(const char* msg){
        size_t i;
        std::cout<<msg<<" size:"<<this->size<<" cur:"<<this->cur<<" data:\n";
        if(is_encrypted){
            for(i=0;i<this->size;i++){
                std::cout<<"["<<i<<":"<<(int)this->data[i]<<"]";
            }
        }
        else{
            for(i=0;i<sizeof(size_t);i++){
                std::cout<<"["<<i<<":"<<(int)this->data[i]<<"]";
            }
            std::cout<<"\ndata";
            for(i=sizeof(size_t);i<this->size+sizeof(size_t);i++){
                std::cout<<"["<<i<<":"<<(int)this->data[i]<<"]";
            }
            std::cout<<"\ncrc";
            for(i=this->size+sizeof(size_t);i<sizeof(TFKey)+this->size+sizeof(size_t);i++){
                std::cout<<"["<<i<<":"<<(int)this->data[i]<<"]";
            }
        }
        std::cout<<"\n";
        this->crc.print("crc");
    }

    private:
    char *data;
    TFKey crc;
    size_t size;//size of data without size and crc fields or fullsize if encrypted
    size_t cur;
    bool is_encrypted;

    template<typename Data>
    void push(Data a,size_t& pos){
        size_t i,n=sizeof(Data);
        char *ca=(char*)&a;
        for(i=0;i<n;i++){
            this->data[pos]=ca[i];
            pos++;
        }
    }
    template<typename Data>
    Data pop(size_t& pos){
        size_t i,n=sizeof(Data);
        char *c=new char[n];
        Data ret;
        for(i=0;i<n;i++){
            // if(pos>this->size+sizeof(size_t)){
            //     std::cout<<"You're out of bounds, bitch!\n";
            //     return NULL;
            // }
            c[i]=this->data[pos];
            pos++;
        }
        ret=*((Data*)c);
        return ret;
    }
};

class char_frame{
    public:
    char data;
    int64_t send_frame(int64_t id,TFKey key){
        Blob blob(sizeof(char));
        blob.push<char>(this->data);
        blob.add_crc();
        blob.print("char sent");
        blob.encrypt(key);
        BLOB_SEND(id);
        return 1;
    }
    int64_t recv_frame(int64_t id,TFKey key){
        Blob blob;
        BLOB_RECV(id);
        blob.decrypt(key);
        blob.print("char recvd");
        if(!blob.check_crc()){
            std::cout<<"Damaged frame!\n";
            return 0;
        }
        this->data=blob.pop<char>();
        return 1;
    }
};

class int_frame{
    public:
    int64_t data;
    int64_t send_frame(int64_t id,TFKey key){
        Blob blob(sizeof(int64_t));
        blob.push<int64_t>(this->data);
        blob.add_crc();
        blob.encrypt(key);
        BLOB_SEND(id);
        return 1;
    }
    int64_t recv_frame(int64_t id,TFKey key){
        Blob blob;
        BLOB_RECV(id);
        blob.decrypt(key);
        if(!blob.check_crc()){
            std::cout<<"Damaged frame!\n";
            return 0;
        }
        this->data=blob.pop<int64_t>();
        return 1;
    }
};

class tfkey_frame{
    public:
    TFKey data;
    int64_t send_frame(int64_t id){
        Blob blob(sizeof(tfkey_frame));
        blob.push<TFKey>(this->data);
        blob.add_crc();
        BLOB_SEND(id);
        return 1;
    }
    int64_t recv_frame(int64_t id){
        Blob blob;
        BLOB_RECV(id);
        if(!blob.check_crc()){
            std::cout<<"Damaged frame!\n";
            return 0;
        }
        this->data=blob.pop<TFKey>();
        return 1;
    }
};

class id_frame{
    public:
    char *name,*pass;
    id_frame(char *a,char* b){
        this->name=new char[MAX_NAME];
        this->pass=new char[MAX_PASS];
        setup(a,b);
    }
    id_frame(){
        this->name=new char[MAX_NAME];
        this->pass=new char[MAX_PASS];
    }
    void setup(char *a,char *b){
        strcpy(this->name,a);
        strcpy(this->pass,b);
    }
    int64_t send_frame(int64_t id,TFKey key){
        Blob blob(MAX_NAME+MAX_PASS);
        blob.push_array<char>(this->name,MAX_NAME);
        blob.push_array<char>(this->pass,MAX_PASS);
        blob.add_crc();
        blob.encrypt(key);
        BLOB_SEND(id);
        return 1;
    }
    int64_t recv_frame(int64_t id,TFKey key){
        Blob blob;
        BLOB_RECV(id);
        blob.decrypt(key);
        if(!blob.check_crc()){
            std::cout<<"Damaged frame!\n";
            return 0;
        }
        blob.pop_array<char>(this->name,MAX_NAME);
        blob.pop_array<char>(this->pass,MAX_PASS);
        return 1;
    }
    void print(){
        std::cout<<"Name="<<this->name<<"\nhash_pass"<<this->pass<<"\n";
    }
};

class msg_header_frame{
    public:
    int64_t id,sender_id,receivers_count;
    char *title;
    time_t time;
    int64_t *receivers;
    msg_header_frame(){
        title=new char[MAX_TITLE];
    }
    void setup(int64_t id, int64_t sender_id,int64_t receivers_count,const char *title,time_t time,const int64_t *receivers){
        this->id=id;
        this->sender_id=sender_id;
        this->receivers_count=receivers_count;
        strcpy(this->title,title);
        this->time=time;
        this->receivers=new int64_t[receivers_count];
        int64_t i;
        for(i=0;i<receivers_count;i++){
            this->receivers[i]=receivers[i];
        }
    }
    int64_t send_frame(int64_t id,TFKey key){
        Blob blob(this->header_size());
        this->push_header(blob);
        blob.add_crc();
        blob.encrypt(key);
        BLOB_SEND(id);
        return 1;
    }
    int64_t recv_frame(int64_t id,TFKey key){
        Blob blob;
        BLOB_RECV(id);
        blob.decrypt(key);
        if(!blob.check_crc()){
            std::cout<<"Damaged frame!\n";
            return 0;
        }
        this->pop_header(blob);
        return 1;
    }
    void print(){
        std::cout<<"ID="<<this->id<<"\nSender ID="<<this->sender_id<<"\nTitle="<<this->title<<"\nTime="<<ctime(&(this->time))<<"Receivers Count="<<this->receivers_count<<"\nReceivers:\n";
        int64_t i;
        for(i=0;i<this->receivers_count;i++){
            std::cout<<"["<<i<<"]"<<this->receivers[i]<<"\n";
        }
    }
    size_t header_size(){
        return (3+this->receivers_count)*sizeof(int64_t)+MAX_TITLE+sizeof(time_t);
    }
    void push_header(Blob& blob){
        std::cout<<"push header\n";
        blob.push<int64_t>(this->id);
        blob.push<int64_t>(this->sender_id);
        blob.push<int64_t>(this->receivers_count);
        blob.push_array<char>(this->title,MAX_TITLE);
        blob.push<time_t>(this->time);
        blob.push_array<int64_t>(this->receivers,this->receivers_count);
    }
    void pop_header(Blob& blob){
        std::cout<<"pop header\n";
        this->id=blob.pop<int64_t>();
        this->sender_id=blob.pop<int64_t>();
        this->receivers_count=blob.pop<int64_t>();
        blob.pop_array<char>(this->title,MAX_TITLE);
        this->time=blob.pop<time_t>();
        this->receivers=new int64_t[this->receivers_count];
        blob.pop_array<int64_t>(this->receivers,this->receivers_count);
        this->print();
    }
};

class msg_frame: public msg_header_frame{
    public:
    bool received;
    char *text;
    msg_frame(){
        this->text=new char[MAX_TEXT];
    }
    void setup(int64_t id,int64_t sender_id,int64_t receivers_count,const char *title,time_t time,const int64_t *receivers,bool received,const char *text){
        msg_header_frame::setup(id,sender_id,receivers_count,title,time,receivers);
        this->received=received;
        strcpy(this->text,text);
    }
    int64_t send_frame(int64_t id,TFKey key){
        Blob blob(this->header_size()+sizeof(bool)+MAX_TEXT);
        this->push_header(blob);
        blob.push<bool>(this->received);
        blob.push_array<char>(this->text,MAX_TEXT);
        blob.add_crc();
        blob.encrypt(key);
        BLOB_SEND(id);
        return 1;
    }
    int64_t recv_frame(int64_t id,TFKey key){
        Blob blob;
        BLOB_RECV(id);
        blob.decrypt(key);
        if(!blob.check_crc()){
            std::cout<<"Damaged frame!\n";
            return 0;
        }
        this->pop_header(blob);
        std::cout<<"pop body\n";
        this->received=blob.pop<bool>();
        blob.pop_array<char>(this->text,MAX_TEXT);
        return 1;
    }
    void print(){
        msg_header_frame::print();
        std::cout<<"Received="<<this->received<<"\n";
        std::cout<<"Text:\n"<<this->text<<"\n";
    }
};

class msglist_frame{
    public:
    int64_t length,cur;
    msg_header_frame *list;
    msglist_frame(){
        this->length=0;
        this->cur=0;
    }
    msglist_frame(int64_t length){
        this->length=length;
        this->cur=0;
        list=new msg_header_frame[length];
    }
    void add(msg_header_frame a){
        list[cur]=a;
        cur++;
    }
    int64_t send_frame(int64_t id,TFKey key){
        size_t total_size=0;
        int64_t i;
        for(i=0;i<this->cur;i++){
            total_size+=this->list[i].header_size();
        }
        Blob blob(sizeof(int64_t)+total_size);
        blob.push<int64_t>(this->cur);
        for(i=0;i<this->cur;i++){
            this->list[i].push_header(blob);
        }
        blob.add_crc();
        blob.encrypt(key);
        BLOB_SEND(id);
        return 1;
    }
    int64_t recv_frame(int64_t id,TFKey key){
        Blob blob;
        BLOB_RECV(id);
        blob.decrypt(key);
        if(!blob.check_crc()){
            std::cout<<"Damaged frame!\n";
            return 0;
        }
        this->length=blob.pop<int64_t>();
        this->cur=this->length;
        this->list=new msg_header_frame[this->length];
        for(int64_t i=0;i<this->length;i++){
            this->list[i].pop_header(blob);
        }
        return 1;
    }
    void print(){
        std::cout<<"Length="<<this->length<<"\nCur="<<this->cur<<"\nHeaders:\n";
        int64_t i;
        for(i=0;i<this->cur;i++){
            std::cout<<"{\n";
            list[i].print();
            std::cout<<"}\n";
        }
    }
};
