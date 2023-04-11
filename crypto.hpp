#include<cstdlib>
#include<iostream>
#include"ThreeFish.hpp"

#define MAX_INT64 18446744073709551616
#define POLY 239

void char_print(char* a, size_t len){
    size_t i;
    std::cout<<"print:\n";
    for(i=0;i<len;i++){
        std::cout<<"["<<i<<":"<<(int)a[i]<<"]";
    }
    std::cout<<"\n";
}

void char32_clean(char* c){
    u_int8_t i;
    for(i=0;i<32;i++){
        c[i]=0;
    }
}

//actualy, this is u_int256_t with sum,mul,pow operations per module 
class TFKey{
    private:
    u_int64_t data[4];

    void setup(){
        return;
    }
    void clean(){
        return;
    }
    public:
    TFKey(){
        //std::cout<<"start TFKey()\n";
        this->setup();
        for(int i=0;i<4;i++){
            this->data[i]=0;
        }
    }
    //converter from char[32]
    TFKey(char k[32]){
        set_from_char(k);
    }
    TFKey(u_int64_t a, u_int64_t b, u_int64_t c, u_int64_t d){
        //std::cout<<"start TFKey(ints)\n";
        this->setup();
        this->data[3]=a;
        this->data[2]=b;
        this->data[1]=c;
        this->data[0]=d;
    }
    ~TFKey(){
        //std::cout<<"start ~TFKey\n";
        this->clean();
        u_int8_t i;
        for(i=0;i<4;i++){
            this->data[i]=0;
        }
    }

    u_int64_t get_data(u_int8_t i){
        return this->data[i];
    }
    void set_data(u_int8_t i, u_int64_t a){
        //std::cout<<"start setdata\n";
        this->data[i]=a;
    }
    void set_random(){
        srand(time(NULL));
        u_int8_t i;
        for(i=0;i<32;i++){
            // std::cout<<"st cast "<<static_cast<u_int64_t>(k[i])<<"\n";
            data[i/8]+=static_cast<u_int64_t>(rand()%256)<<(i%8*8);
        }
    }
    void set_from_char(char k[32]){
        //std::cout<<"start TFKey(char*)\n";
        this->setup();
        u_int8_t i;
        for(i=0;i<32;i++){
            this->data[i/8]=this->data[i/8]*256+k[i];
        }
    }
    void to_bytes(char* a){
        int8_t i;
        u_int64_t buff[4];
        for(i=0;i<4;i++){
            buff[i]=this->data[i];
        }
        for(i=31;i>=0;i--){
            a[i]=buff[i/8]%256;
            buff[i/8]/=256;
        }
    }
    void print(const char* name){
        std::cout<<name<<" "<<this->get_data(3)<<" "<<this->get_data(2)<<" "<<this->get_data(1)<<" "<<this->get_data(0)<<"\n";
    }

    bool operator>(const TFKey& b){
        //std::cout<<"start >\n";
        u_int8_t i;
        // std::cout<<">this "<<this->data[3]<<" "<<this->data[2]<<" "<<this->data[1]<<" "<<this->data[0]<<"\n";
        // std::cout<<">b "<<b.data[3]<<" "<<b.data[2]<<" "<<b.data[1]<<" "<<b.data[0]<<" addr "<<&b<<"\n"; 
        for(i=0;i<4;i++){
            if(this->data[3-i]>b.data[3-i]) return 1;
            if(this->data[3-i]<b.data[3-i]) return 0;
        }
        // std::cout<<"this "<<this->data[3]<<" "<<this->data[2]<<" "<<this->data[1]<<" "<<this->data[0]<<"\n";
        // std::cout<<"b "<<b.data[3]<<" "<<b.data[2]<<" "<<b.data[1]<<" "<<b.data[0]<<" addr "<<&b<<"\n"; 
        return 0;
    }
    bool operator==(const TFKey& b){
        u_int8_t i;
        for(i=0;i<4;i++){
            if(this->data[i]!=b.data[i]) return 0;
        }
        return 1;
    }
    void operator=(const TFKey& b){
        //std::cout<<"start ="<<this<<"\n";
        TFKey ret;
        u_int8_t i;
        for(i=0;i<4;i++){
            this->data[i]=b.data[i];
            // ret.data[i]=b.data[i];
        }
        // std::cout<<"end =\n";
    }
    TFKey operator-(const TFKey& b){
        //std::cout<<"start -\n";
        TFKey ret;
        // std::cout<<"-this "<<this->get_data(3)<<" "<<this->get_data(2)<<" "<<this->get_data(1)<<" "<<this->get_data(0)<<"\n"; 
        // std::cout<<"-b "<<b.data[3]<<" "<<b.data[2]<<" "<<b.data[1]<<" "<<b.data[0]<<" addr "<<&b<<"\n";  
        u_int8_t i,buff=0;
        for(i=0;i<4;i++){
            if((this->data[i]<b.data[i])||((this->data[i]==b.data[i])&&(buff))){
                ret.data[i]=this->data[i]-b.data[i]-buff;
                buff=1;
            }
            else{
                ret.data[i]=this->data[i]-b.data[i]-buff;
                buff=0;
            }
        }
        // std::cout<<"this "<<this->get_data(3)<<" "<<this->get_data(2)<<" "<<this->get_data(1)<<" "<<this->get_data(0)<<"\n";
        // std::cout<<"b "<<b.data[3]<<" "<<b.data[2]<<" "<<b.data[1]<<" "<<b.data[0]<<" addr "<<&b<<"\n";  
        return ret;
    }
    TFKey operator+(const TFKey& b){
        // std::cout<<"start +\n";
        TFKey ret;
        u_int8_t i;
        u_int64_t buff=0;
        for(i=0;i<4;i++){
            ret.data[i]=b.data[i]+this->data[i]+buff;
            if((!this->data[i])&&(b.data[i]!=-1)) buff=0;
            else if(-this->data[i]-buff>b.data[i]) buff=0;
            else buff=1;
        }
        return ret;
    }

    static TFKey mod(const TFKey& x,const TFKey& y){
        // std::cout<<"start mod\n";
        TFKey a=x,m=y;
        // std::cout<<"a "<<a.get_data(3)<<" "<<a.get_data(2)<<" "<<a.get_data(1)<<" "<<a.get_data(0)<<"\n";  
        if(m>a) return a;
        u_int8_t lm=0,buff;
        int16_t i,j;
        u_int64_t one=1,nb;
        TFKey b;
        // std::cout<<"mod start\n";
        // find index of first bit=1 of m
        for(i=3;i>=0;i--){
            for(j=63;j>=0;j--){
                if(m.data[i]&(one<<j)){
                    // std::cout<<"gotcha\n";
                    lm=i*64+j;
                    break;
                }
            }
            if(lm) break;
        }
        // std::cout<<"found"<<(int)lm<<"\n";
        for(i=255-lm;i>=0;i--){
            // std::cout<<"i="<<i<<" ";
            // set b as roll mod to i offset
            b=TFKey(0,0,0,0);
            for(j=lm;j>=0;j--){
                // std::cout<<"bdata="<<b.data[(i+j)/64]<<" mdata="<<m.data[j/64]<<" one="<<(one<<(j%64))<<" j="<<j<<"\n";
                if((i+j)%64-j%64>0){
                    nb=(m.data[j/64]&(one<<(j%64)))<<((i+j)%64-j%64);
                }
                else{
                    nb=(m.data[j/64]&(one<<(j%64)))>>(j%64-(i+j)%64);
                }
                b.data[(i+j)/64]|=nb;
            }
            // std::cout<<"a "<<a.get_data(3)<<" "<<a.get_data(2)<<" "<<a.get_data(1)<<" "<<a.get_data(0)<<" ";            
            // std::cout<<"new b "<<b.get_data(3)<<" "<<b.get_data(2)<<" "<<b.get_data(1)<<" "<<b.get_data(0)<<" ";
            if((a>b)||(a==b)) {
                // std::cout<<"[-]";
                a=a-b;
            }
            // std::cout<<"new a "<<a.get_data(3)<<" "<<a.get_data(2)<<" "<<a.get_data(1)<<" "<<a.get_data(0)<<"\n";
        }
        return a;
    }
    static TFKey sum_mod(const TFKey& x, const TFKey& y, const TFKey& z){
        // std::cout<<"start summod\n";
        TFKey a=x,b=y,m=z;
        // std::cout<<"a "<<a.get_data(3)<<" "<<a.get_data(2)<<" "<<a.get_data(1)<<" "<<a.get_data(0)<<"\n";            
        // std::cout<<"b "<<b.get_data(3)<<" "<<b.get_data(2)<<" "<<b.get_data(1)<<" "<<b.get_data(0)<<"\n";
        // std::cout<<"m "<<m.get_data(3)<<" "<<m.get_data(2)<<" "<<m.get_data(1)<<" "<<m.get_data(0)<<"\n";
        a=mod(a,m);
        b=mod(b,m);
        // a.print("new a");
        // b.print("new b");
        // (a+b).print("a+b");
        if(m-a>b) return a+b;
        else return b-(m-a);
    }
    static TFKey mul_mod(const TFKey& x, const TFKey& y, const TFKey& z){
        // std::cout<<"start mulmod\n";
        TFKey a=x,b=y,m=z;
        // a.print("a");
        // b.print("b");
        // m.print("m");
        a=mod(a,m);
        b=mod(b,m);
        // a.print("new a");
        // b.print("new b");
        if(a>b){
            TFKey buff = a;
            a=b;
            b=buff;
        }
        TFKey ret;
        int i,j,l=0;
        for(i=0;i<4;i++){
            if(!a.data[i]){
                // std::cout<<"zero data\n";
                for(j=0;j<64;j++){
                    b=sum_mod(b,b,m);
                }
            }
            else{
                for(j=0;j<64;j++){
                    if(a.data[i]&1) ret=sum_mod(ret,b,m);
                    b=sum_mod(b,b,m);
                    a.data[i]>>=1;
                }
            }
            // ret.print("ret");
        }
        // b.print("fin b");
        return ret;
    }
    static TFKey pow_mod(const TFKey& x, const TFKey& y, const TFKey& z){
        // std::cout<<"start powmod\n";
        TFKey a=x,b=y,m=z;
        a=mod(a,m);
        b=mod(b,m);
        TFKey ret(0,0,0,1);
        int i,j;
        for(i=0;i<4;i++){
            if(!b.data[i]){
                for(j=0;j<64;j++){
                    a=mul_mod(a,a,m);
                }
            }
            else{
                for(j=0;j<64;j++){
                    if(b.data[i]&1) ret=mul_mod(ret,a,m);
                    a=mul_mod(a,a,m);
                    b.data[i]>>=1;
                }
            }
            // ret.print("ret");
        }
        return ret;
    }
};

void key_change(TFKey& key){}

TFKey make_crc(char* data, size_t size){
    size_t i,j,rnds=size/sizeof(TFKey);
    TFKey buf,crc,kmod(19662,16582360881486924019,5082632028705998950,895695448175102663);
    char poly;
    // std::cout<<"make_crc:";
    for(i=0;i<rnds;i++){
        buf.set_from_char(data+i*sizeof(TFKey));
        // char_print(data+i*sizeof(TFKey),sizeof(TFKey));
        poly=POLY;
        for(j=0;j<8;j++){
            if(poly&1){
                crc=TFKey::sum_mod(crc,TFKey::pow_mod(buf,TFKey(0,0,0,j),kmod),kmod);
            }
            poly>>=1;
            // crc.print("base_crc");
        }
    }
    if(size%sizeof(TFKey)){
        char *tail=new char[sizeof(TFKey)];
        // std::cout<<"kek";
        // char_print(tail,sizeof(TFKey));
        for(i=0;i<size%sizeof(TFKey);i++){
            tail[i]=data[rnds*sizeof(TFKey)+i];
        }
        for(;i<sizeof(TFKey);i++){
            tail[i]=0;
        }
        // std::cout<<"tail"<<size%sizeof(TFKey)<<":";
        // char_print(tail,sizeof(TFKey));
        buf.set_from_char(tail);
        poly=POLY;
        for(j=0;j<8;j++){
            if(poly&1){
                crc=TFKey::sum_mod(crc,TFKey::pow_mod(buf,TFKey(0,0,0,j),kmod),kmod);
            }
            poly>>1;
            // crc.print("tail_crc");
        }
    }
    return crc;
}

class CryptedStruct{
    public:
    char *data;
    size_t size;
    CryptedStruct(size_t size){
        this->size=size;
        this->data=new char[size];
    }
    CryptedStruct(){}
};

template<typename T>
CryptedStruct TF_encrypt(T *src,size_t size, TFKey k){
    char t[16]={(char)69,'l','e','t','n','o','n','e','s','u','r','v','i','v','e',(char)228};
    char starter[32],c[32],*csrc;
    csrc=(char*)src;
    int i,tail;
    size_t cur_src=0,cur_dst=0;
    Tweak twek(t);
    //key init
    char *ck=new char[32];
    k.to_bytes(ck);
    Key key(ck,twek);
    char32_clean(ck);
    delete[] ck;
    Block kek,prev;
    //setup of starter initial vector
    srand(time(0));
    for(i=0;i<32;i++){
        starter[i]=static_cast<char>(rand()%256);
    }
    prev.setBlock(starter);
    //set output size
    tail=size%32;
    CryptedStruct dst(32+size/32*32+(bool)tail*32);
    //initial vector writed without change
    insert<char>(starter,dst.data,cur_dst,32);
    //tail insertion
    if(tail){
        cur_dst+=32;
        for(i=0;i<32-tail;i++){
            // std::cout<<i<<" put tail\n";
            c[i]=0;
        }
        for(i=32-tail;i<32;i++){
            c[i]=csrc[cur_src];
            cur_src++;
        }
    }
    kek.setBlock(c);
    kek=Block::bxor(kek,prev);
    kek.encrypt(key);
    prev.clone(kek);
    for(i=0;i<32;i++){
        c[i]=kek.getWord(i/8).get_byte(i%8);
    }
    insert<char>(c,dst.data,cur_dst,32);
    cur_dst+=32;
    //next blocks encrypting
    while(cur_src<size){
        cut<char>(csrc,c,cur_src,32);
        cur_src+=32;
        kek.setBlock(c);
        kek=Block::bxor(kek,prev);
        kek.encrypt(key);
        prev.clone(kek);
        for(i=0;i<32;i++){
            c[i]=kek.getWord(i/8).get_byte(i%8);
        }
        insert<char>(c,dst.data,cur_dst,32);
        cur_dst+=32;
    }
    return dst;
}

template<typename T>
T* TF_decrypt(CryptedStruct src, TFKey k){
    char t[16]={(char)69,'l','e','t','n','o','n','e','s','u','r','v','i','v','e',(char)228};
    char c[32],*cdst;
    int i;
    cdst=new char[src.size];
    Tweak twek(t);
    size_t cur_src=0,cur_dst=0;
    //key init
    char *ck=new char[32];
    k.to_bytes(ck);
    Key key(ck,twek);
    char32_clean(ck);
    delete[] ck;
    Block kek,prev;
    Block buff;
    //get starter for XOR
    cut<char>(src.data,c,cur_src,32);
    cur_src+=32;
    prev.setBlock(c);
    for(i=0;i<32;i++){
        // std::cout<<"("<<(int)src.data[cur_src+i]<<")";
    }
    // std::cout<<"1\n";
    //first block treating
    cut(src.data,c,cur_src,32);
    for(i=0;i<32;i++){
        // std::cout<<"("<<(int)c[i]<<")";
    }
    // std::cout<<"2\n";
    cur_src+=32;
    kek.setBlock(c);
    buff.clone(kek);
    kek.decrypt(key);
    kek=Block::bxor(kek,prev);
    prev.clone(buff);
    for(i=0;i<32;i++){
        c[i]=kek.getWord(i/8).get_byte(i%8);
        // std::cout<<"("<<(int)c[i]<<")";
    }
    // std::cout<<"\n";
    i=0;
    while(c[i]==0){
        if(i==31){
            std::cout<<"gay alarm\n";
            return NULL;
        }
        i++;
    }
    for(;i<32;i++){
        insert<char>(&c[i],cdst,cur_dst,1);
        cur_dst+=1;
    }
    //next blocks treating
    while(cur_src<src.size){
        cut<char>(src.data,c,cur_src,32);
        cur_src+=32;
        kek.setBlock(c);
        buff.clone(kek);
        kek.decrypt(key);
        kek=Block::bxor(kek,prev);
        prev.clone(buff);
        for(i=0;i<32;i++){
            c[i]=kek.getWord(i/8).get_byte(i%8);
        }
        insert<char>(c,cdst,cur_dst,32);
        cur_dst+=32;
    }
    return (T*)cdst;
}

inline void charcpy(char* dst, char* src, size_t len){
    for(int i=0;i<len;i++){
        dst[i]=src[i];
    }
}