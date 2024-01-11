#include"consts.hpp"

//check for legal symbols return 0 if good
bool namecheck(char *name){
	int64_t i;
	if(name[0]=='\n') return 1;
	for(i=0;i<MAX_NAME;i++){
		if(name[i]=='\0') break;
		else if (((name[i]>='A')&&(name[i]<='Z'))||((name[i]>='a')&&(name[i]<='z'))||(name[i]=='_')) continue;
		return 1;
	}
	return 0;
}

//escaping message text ('->'') translates src=char[MAX_TEXT] dst=char[2*MAX_TEXT]
void textescape(char* dst,const char *src){
	u_int64_t i,j=0;
	for(i=0;i<MAX_TEXT;i++){
		if(src[i]=='\''){
			dst[j]='\'';
			j++;
		}
		dst[j]=src[i];
		j++;
	}
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