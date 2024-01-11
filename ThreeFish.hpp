#include<cstdlib>
#include<iostream>
#include<fstream>
#include<cmath>
#include<cstring>
#include<ctime>
// #include"crypto.hpp"

/// @brief block 8 bytes size
class Word{
    private:
    //bits operations (in order left to right)
    const bool get_bit(int a){
        unsigned char b=data[a/8];
        return b&(128>>(a%8));
    }
    void set_bit(int a, bool b){
        if (b){
            data[a/8]|=(128>>(a%8));
        }
        else{
            data[a/8]&=~(128>>(a%8));
        }
    }
    
    public:
    unsigned char data[8];

    //constructors
    Word(){
        char i;
        for(i=0;i<8;i++){
            data[i]=0;
        }
    }
    Word(char a[8]){
        char i;
        for(i=0;i<8;i++){
            data[i]=a[i];
        }
    }
    //bytes operations (from left to right)
    // unsigned char get_byte(int a){
    //     return(data[a]);
    // }
    void set_byte(int a, char b){
        data[a]=b;
    }
    //roll left with pushed out bits transition to right
    void rotate_left(int a){
        int i;
        Word buff;
        for(i=0;i<8;i++){
            buff.set_byte(i,data[i]);
        }   
        for(i=a;i<64+a;i++){
            this->set_bit(i-a,buff.get_bit(i%64));
        }
        return;
    }
    //math operators
    static Word plus(Word& a,Word& b){
        int i,s=0;
        Word c;
        for(i=7;i>=0;i--){
            s=a.data[i]+b.data[i]+s;
            c.set_byte(i,s%256);
            s=(s>=256);
        }
        return c;
    }
    static Word minus(Word& a,Word& b){
        int i,s=0;
        Word c;
        for(i=7;i>=0;i--){
            s=256+a.data[i]-b.data[i]-s;
            c.set_byte(i,s%256);
            s=(s<256);
        }
        return c;
    }
    static Word wxor(Word& a, Word& b){
        int i;
        Word c;
        for(i=0;i<8;i++){
            c.set_byte(i,a.data[i]^b.data[i]);
        }
        return c;
    }
    static void mix (Word c[2],Word a, Word b, int d, int j){
        int r[2][8]={{14,52,23,5,25,46,58,32},{16,57,40,37,33,12,22,32}};
        c[0]=Word::plus(a,b);
        b.rotate_left(r[j][d%8]);
        c[1]=Word::wxor(b,c[0]);
    }
    static void unmix (Word c[2],Word a, Word b, int d, int j){
        int r[2][8]={{14,52,23,5,25,46,58,32},{16,57,40,37,33,12,22,32}};
        c[1]=Word::wxor(a,b);
        c[1].rotate_left(64-r[j][d%8]);
        c[0]=Word::minus(a,c[1]);
    }
};

class Tweak{
    private:

    public:
    Word data[3];
    Tweak(char a[16]){
        int i;
        for(i=0;i<16;i++){
            data[i/8].set_byte(i%8,a[i]);
        }
        data[2]=Word::wxor(data[0],data[1]);
    }
    // Word getWord(int a){
    //     return data[a];
    // }
};

class Key{
    private:
    
    public:
    Word data[19][4];
    Key(char key[32],Tweak b){
        int i;
        Word wk[4];
        for(i=0;i<32;i++){
            wk[i/8].set_byte(i%8,key[i]);
        }
        char c[8]={(char)27,(char)209,(char)27,(char)218,(char)169,(char)252,(char)26,(char)34};
        Word k[5],c240(c),ws;
        for(i=0;i<4;i++){
            k[i]=wk[i];
            k[4]=Word::wxor(k[4],wk[i]);
        }
        k[4]=Word::wxor(k[4],c240);
        for(i=0;i<19;i++){
            data[i][0]=k[i%5];
            data[i][1]=Word::plus(k[i%5],b.data[i%3]);
            data[i][2]=Word::plus(k[i%5],b.data[(i+1)%3]);
            ws.set_byte(7,i);
            data[i][3]=Word::plus(k[i%5],ws);
        }
    }
    Key(Word a[4],Tweak b){
        int i;
        char c[8]={(char)27,(char)209,(char)27,(char)218,(char)169,(char)252,(char)26,(char)34};
        Word k[5],c240(c),ws;
        for(i=0;i<4;i++){
            k[i]=a[i];
            k[4]=Word::wxor(k[4],a[i]);
        }
        k[4]=Word::wxor(k[4],c240);
        for(i=0;i<19;i++){
            data[i][0]=k[i%5];
            data[i][1]=Word::plus(k[i%5],b.data[i%3]);
            data[i][2]=Word::plus(k[i%5],b.data[(i+1)%3]);
            ws.set_byte(7,i);
            data[i][3]=Word::plus(k[i%5],ws);
        }
    }
    // Word getWord(int a,int b){
    //     return data[a][b];
    // }
};

class Block{
    private:

    public:
    Word data[4];
    Block(){
        int i;
        for(i=0;i<32;i++){
            data[i/8].set_byte(i%8,static_cast<char>(0));
        }
    }
    Block(char a[32]){
        int i;
        for(i=0;i<32;i++){
            data[i/8].set_byte(i%8,a[i]);
        }
    }
    // Word getWord(int a){
    //     return data[a];
    // }
    void setBlock(char a[32]){
        int i;
        for(i=0;i<32;i++){
            data[i/8].set_byte(i%8,a[i]);
        }
    }
    void encrypt(Key& key){
        int i,j;
        Word *pair;
        pair=(Word*)malloc(sizeof(Word)*2);
        for(i=0;i<72;i++){
            //step 1
            if(i%4==0){
                for(j=0;j<4;j++){
                    this->data[j]=Word::plus(this->data[j],key.data[i/4][j]);
                }
            }
            //step 2
            for(j=0;j<2;j++){
                Word::mix(pair,this->data[2*j],this->data[2*j+1],i,j);
                this->data[2*j]=pair[0];
                this->data[2*j+1]=pair[1];
            }
            //step 3
            pair[0]=this->data[1];
            this->data[1]=this->data[3];
            this->data[3]=pair[0];
        }
    }
    void decrypt(Key& key){
        int i,j;
        Word *pair;
        pair=(Word*)malloc(sizeof(Word)*2);
        for(i=71;i>=0;i--){
            //step 3
            pair[0]=this->data[1];
            this->data[1]=this->data[3];
            this->data[3]=pair[0];
            //step 2
            for(j=0;j<2;j++){
                Word::unmix(pair,this->data[2*j],this->data[2*j+1],i,j);
                this->data[2*j]=pair[0];
                this->data[2*j+1]=pair[1];
            }
            //step 1
            if(i%4==0){
                for(j=0;j<4;j++){
                    this->data[j]=Word::minus(this->data[j],key.data[i/4][j]);
                }
            }
        }
    }
    static Block bxor(Block& a,Block& b){
        int i;
        Block c;
        for(i=0;i<4;i++){
            c.data[i]=Word::wxor(a.data[i],b.data[i]);
        }
        return c;
    }
    void clone(Block& b){
        int i;
        for(i=0;i<4;i++){
            this->data[i]=b.data[i];
        }
    }
};

// int main(int argc, char **argv){
//     if(argc!=5){
//         std::cout<<"Usage:\nTreeFish {-e/-d} source_file key_file destination_file";
//     }
//     else if(argc==0){
//         std::cout<<"What magic is this?";
//     }
//     else{
//         FILE *sfile,*kfile,*dfile;
//         //files open
//         sfile=fopen(argv[2],"r");
//         kfile=fopen(argv[3],"r");
//         dfile=fopen(argv[4],"w");
//         int len=0;
//         char c[32],k[32],r;
//         char t[16]={(char)69,'l','e','t','n','o','n','e','s','u','r','v','i','v','e',(char)228};
//         char starter[32];
//         Word wk[4];
//         //get 32 bytes linear key from file
//         fread(k,1,32,kfile);
//         //split linear key to table (may be implemented in lib)
//         int i;
//         for(i=0;i<32;i++){
//             wk[i/8].set_byte(i%8,k[i]);
//         }
//         Tweak twek(t);
//         Key key(wk,twek);
//         Block kek,prev;
//         //encryption
//         if(!strcmp(argv[1],"-e")){
//             //setup of starter XOR
//             srand(time(0));
//             for(i=0;i<32;i++){
//                 starter[i]=static_cast<char>(rand()%256);
//             }
//             //initial vector writed without change
//             prev.setBlock(starter);
//             fwrite(starter,1,32,dfile);
//             //first block encrypting
//             while(fread(c,1,1,sfile)!='\0'){
//                 len++;
//                 if(len==32){
//                     len=0;
//                 }
//             }
//             fseek(sfile,0,SEEK_SET);
//             //first block padding
//             for(i=0;i<32-len;i++){
//                 c[i]=0;
//             }
//             for(i=32-len;i<32;i++){
//                 fread(&c[i],1,1,sfile);
//             }
//             kek.setBlock(c);
//             kek=Block::bxor(kek,prev);
//             kek.encrypt(key);
//             prev.clone(kek);
//             for(i=0;i<32;i++){
//                 c[i]=kek.getWord(i/8).get_byte(i%8);
//             }
//             //next blocks encrypting
//             fwrite(c,1,32,dfile);
//             while(fread(c,1,32,sfile)==32){
//                 kek.setBlock(c);
//                 kek=Block::bxor(kek,prev);
//                 kek.encrypt(key);
//                 prev.clone(kek);
//                 for(i=0;i<32;i++){
//                     c[i]=kek.getWord(i/8).get_byte(i%8);
//                 }
//                 fwrite(c,1,32,dfile);
//             }
//         }
//         //decryption
//         if(!strcmp(argv[1],"-d")){
//             Block buff;
//             //get starter for XOR
//             fread(starter,1,32,sfile);
//             prev.setBlock(starter);
//             //first block treating
//             fread(c,1,32,sfile);
//             kek.setBlock(c);
//             buff.clone(kek);
//             kek.decrypt(key);
//             kek=Block::bxor(kek,prev);
//             prev.clone(buff);
//             for(i=0;i<32;i++){
//                 c[i]=kek.getWord(i/8).get_byte(i%8);
//             }
//             i=0;
//             while(c[i]==0){
//                 if(i==31){
//                     return -1;
//                 }
//                 i++;
//             }
//             for(;i<32;i++){
//                 fwrite(&c[i],1,1,dfile);
//             }
//             //next blocks treating
//             while(fread(c,1,32,sfile)==32){
//                 kek.setBlock(c);
//                 buff.clone(kek);
//                 kek.decrypt(key);
//                 kek=Block::bxor(kek,prev);
//                 prev.clone(buff);
//                 for(i=0;i<32;i++){
//                     c[i]=kek.getWord(i/8).get_byte(i%8);
//                 }
//                 fwrite(c,1,32,dfile);
//             }
//         }
//         fclose(sfile);
//         fclose(kfile);
//         fclose(dfile);
//         return 0;
//     }
// }

/// @brief inserts number of items from src array to dst array with offset
/// @tparam T type of items in arrays
/// @param src src array
/// @param dst dst array
/// @param start where to put in dst array
/// @param len how mutch to take from src array
template<typename T>
void insert(T* src, T* dst, size_t start, size_t len){
    size_t i;
    for(i=0;i<len;i++){
        dst[start+i]=src[i];
    }
}

/// @brief cut number of items with offset from src array to dst array
/// @tparam T type of items in arrays
/// @param src src array
/// @param dst dst array
/// @param start from where to take in src
/// @param len how mutch to put in dst array
template<typename T>
void cut(T* src, T* dst, size_t start, size_t len){
    size_t i;
    for(i=0;i<len;i++){
        dst[i]=src[start+i];
    }
}