#include<iostream>
#include<thread>
using namespace std;

class kekclass{
    public:
    void kekthread(int a){
        cout<<"this is a thread "<<a;
    }
};

int main(){
    int a;
    kekclass kek;
    cin>>a;
    thread thr(a);
    thr.join();
    return 0;
}