#ifndef INODE_H
#define INODE_H

#include "my_fs.h"
/**
 * @class Inode
 * @author Rick
 * @date 11/01/2016
 * @file Inode.h
 * @brief I节点
 */
class Inode//设计大小 64 bytes
{
public:
	Inode();
	~Inode();
	/*属性类*/
	unsigned int number;//i结点的编号（4 bytes）
	unsigned int size;//i结点对应文件的大小（单位：byte） （4 bytes）
	short file_type;//文件类型 （2 bytes）
	short authority = 0b000000000;//文件权限 采用-rwxrwxrwx的形式 （2 bytes）
	/*地址*/
	unsigned int address[INODE_DIRECT_ADDRESS_COUNT];//直接地址（16 bytes）
	unsigned int address_1;//一次间址（4 bytes）
	unsigned int address_2;//二次间址（4 bytes）

	/*各类时间*/
	time_t create_time;//创建时间（8 bytes）
	time_t modify_time;//修改时间（8 bytes）
	/*计数类*/
	unsigned int link_count;//连接次数（4 bytes）
	/*用户类记录数据*/
	unsigned int user_id;//文件所有者id （4 bytes）
	unsigned int group_id;//文件所有组id （4 bytes）

	short modify_flag = MODIFIED;//i节点修改标志


	const static unsigned int offset_number = 0;//i节点编号
	const static unsigned int offset_size = sizeof(number);//文件大小
	const static unsigned int offset_file_type = offset_size + sizeof(size);//文件类型
	const static unsigned int offset_authority = offset_file_type + sizeof(file_type);//文件权限
	const static unsigned int offset_address = offset_authority + sizeof(authority);//直接地址列表
	const static unsigned int offset_address_1 = offset_address + sizeof(address);//一次间址
	const static unsigned int offset_address_2 = offset_address_1 + sizeof(address_1);//二次间址
	const static unsigned int offset_create_time = offset_address_2 + sizeof(address_2);//创建时间
	const static unsigned int offset_modify_time = offset_create_time + sizeof(create_time);//修改时间
	const static unsigned int offset_link_count = offset_modify_time + sizeof(modify_time);//连接计数
	const static unsigned int offset_user_id = offset_link_count + sizeof(link_count);//文件所有者id
	const static unsigned int offset_group_id = offset_user_id + sizeof(user_id);//文件所有组id


	const static unsigned int save_size = offset_group_id + sizeof(group_id);//i节点在文件中存储所占的大小


	static unsigned int getSingleResetData(char* data, unsigned int i_number);//获取复位数据

	unsigned int getSaveData(char*& data);//获取当前i节点内保存的数据
	bool save(fstream* file);//保存i节点信息
	bool load(fstream* file);//读取i节点信息
	unsigned long getSaveOffset();//获取在文件中保存的起始位置
	unsigned int freeBlocksBySize(fstream* file, unsigned int new_size);//释放多余的盘块
	string getContent(fstream* file);//获取inode节点对应文件的内容（字符串形式）
	char* getBinaryContent(fstream* file);//获取inode节点对应文件的内容（二进制形式）
	string getStringAuthority();//获取权限的字符串形式
	char getCharFileType();//获取文件类型对应的字母
	bool canChmod();//是否可以更改权限
	bool canRead();//是否可读
	bool canWrite();//是否可写
	bool canExecute();//是否可执行
	bool canDelete();//是否可删除
	bool canChangeUidOrGid();//是否可以更改用户id或者组id
};

#endif // INODE_H
