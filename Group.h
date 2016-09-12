#ifndef GROUP_H
#define GROUP_H
#include "my_fs.h"

class Group// (16 bytes)
{
public:
	Group();
	virtual ~Group();
    char name[GROUP_NAME_LENGHT];//×éÃû (12 bytes)
    unsigned int gid;//×éºÅ (4 bytes)

	static const unsigned int offset_name = 0;
	static const unsigned int offset_gid = sizeof(name);
	static const unsigned int save_size = offset_gid + sizeof(gid);

    void setName(const char* n, unsigned int length);//ÉèÖÃÃû×Ö
    char* getSaveData();//»ñÈ¡´æ´¢Êý¾Ý
};

#endif // GROUP_H
