#include "User.h"
#include <iostream>
using namespace std;
template<class T>
unsigned int convertToBinaryNotNew(char* b_data, T o_data);
User::User()
{
	//ctor
}

User::~User()
{
	//dtor
}

char* User::getSaveData()
{
    char* data = new char[this->save_size];
    unsigned int i, curr = 0;
    for (i = 0;i<sizeof(name);i++){
        data[curr] = name[i];
        curr ++;
    }
    for (i = 0;i<sizeof(password);i++){
        data[curr] = password[i];
        curr ++;
    }
    curr += convertToBinaryNotNew(data + curr, uid);
    curr += convertToBinaryNotNew(data + curr, gid);
    return data;
}

void User::setName(const char* n, unsigned int length)
{
    unsigned int i;
	for(i = 0; i < USER_NAME_LENGTH; i++) {
		this->name[i] = i<length ? n[i] : 0;
	}
}

void User::setPassword(const char* p, unsigned int length)
{
	unsigned int i;
	for(i = 0; i < USER_PASSWORD_LENGTH; i++) {
		this->password[i] = i<length ? p[i] : 0;
	}
}

