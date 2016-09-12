#ifndef USER_H
#define USER_H
#include "my_fs.h"

class User// (32 bytes)
{
public:
	User();
	virtual ~User();
    char name[USER_NAME_LENGTH];//ÓÃ»§Ãû (12 bytes)
    char password[USER_PASSWORD_LENGTH];//ÃÜÂë (12 bytes)
    unsigned int uid;//ÓÃ»§id (4 bytes)
    unsigned int gid;//ÓÃ»§×éid (4 bytes)

	static const unsigned int offset_name = 0;
	static const unsigned int offset_password = sizeof(name);
	static const unsigned int offset_uid = sizeof(password) + offset_password;
	static const unsigned int offset_gid = sizeof(uid) + offset_uid;
	static const unsigned int save_size = sizeof(gid) + offset_gid;

    char* getSaveData();//»ñÈ¡´æ´¢Êý¾Ý
    void setName(const char* n, unsigned int length);//ÉèÖÃÓÃ»§Ãû
    void setPassword(const char* p, unsigned int length);//ÉèÖÃÃÜÂë
};

#endif // USER_H
