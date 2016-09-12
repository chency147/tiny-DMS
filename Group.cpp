#include "Group.h"

template<class T>
unsigned int convertToBinaryNotNew(char* b_data, T o_data);
Group::Group()
{
	//ctor
}

Group::~Group()
{
	//dtor
}

void Group::setName(const char* n, unsigned int length)
{
	unsigned int i;
	for(i = 0; i < USER_NAME_LENGTH; i++) {
		this->name[i] = i<length ? n[i] : 0;
	}
}
char* Group::getSaveData()
{
	char* data = new char[this->save_size];
    unsigned int i, curr = 0;
    for (i = 0;i<sizeof(name);i++){
        data[curr] = name[i];
        curr ++;
    }
    curr += convertToBinaryNotNew(data + curr, gid);
    return data;
}
