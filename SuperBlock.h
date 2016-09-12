#ifndef SUPERBLOCK_H
#define SUPERBLOCK_H
#include "my_fs.h"
#include "Dentry.h"
#include "Inode.h"
#include "User.h"
#include "Group.h"
#include <sstream>
using std::stringstream;
/**
 * @class SuperBlock
 * @author Rick
 * @date 13/01/2016
 * @file SuperBlock.h
 * @brief 超级块
 */

class SuperBlock //(204 bytes)
{
public:
	SuperBlock();
	~SuperBlock();
	unsigned int block_amount;//盘块数量 (4 bytes)
	unsigned int inode_amount;//i节点数量 (4 bytes)
	unsigned int per_block_size = BLOCK_SIZE;//每个盘块的大小 (4 bytes)
	unsigned int free_inodes_stack[FREE_INODE_STACK_SIZE];//空闲i节点栈 (20*4 bytes)
	short free_inodes_stack_modify_flag = MODIFIED;//空闲i节点栈修改标志
	unsigned int free_inodes_amount;//空闲i节点数量 (4 bytes)
	unsigned int free_blocks_stack[FREE_BLOCK_STACK_SIZE];//空闲盘块栈 (20*4 bytes)
	short free_blocks_stack_modify_flag = MODIFIED;//空闲盘块栈修改标志
	unsigned int free_blocks_amount;//空闲盘块数量 (4 bytes)
	time_t modify_time;//修改时间 (8 bytes)
	time_t create_time;//创建时间 (8 bytes)

	int curr_block_stack = FREE_BLOCK_STACK_SIZE - 1;//盘块栈顶指针
	int curr_inode_stack = FREE_INODE_STACK_SIZE - 1;//i节点栈顶指针

	unsigned int free_blocks_stack_bottom;//空闲盘块栈底 (4 bytes)
	unsigned int root_inode_number;//根i节点编号（4bytes）

	Dentry* root;//根目录
	short modify_flag = MODIFIED;//超级块修改标志
	list<Inode> inodes;//所有i节点


    User* current_user;
    map<string, User> users;
    map<string, Group> groups;

    unsigned short umask = 0b111111111;

	/*参数在文件中的偏移量*/
	const static unsigned int offset_block_amount = 0;//盘块数量
	const static unsigned int offset_inode_amount = sizeof(block_amount);//i节点数量
	const static unsigned int offset_per_block_size = offset_inode_amount + sizeof(inode_amount);//盘块大小
	const static unsigned int offset_free_inodes_stack = offset_per_block_size + sizeof(per_block_size);//空闲i节点栈
	const static unsigned int offset_free_inodes_amount = offset_free_inodes_stack + sizeof(free_inodes_stack); //空闲i节点数量
	const static unsigned int offset_free_blocks_stack = offset_free_inodes_amount + sizeof(free_inodes_amount);//空闲盘块栈
	const static unsigned int offset_free_blocks_amount = offset_free_blocks_stack + sizeof(free_blocks_stack);//空闲盘块栈大小
	const static unsigned int offset_modify_time = offset_free_blocks_amount + sizeof(free_blocks_amount);//修改时间
	const static unsigned int offset_create_time = offset_modify_time + sizeof(modify_time);//创建时间
	const static unsigned int offset_free_blocks_stack_bottom = offset_create_time + sizeof(create_time);//空闲盘块栈底 (4 bytes)
	const static unsigned int offset_root_inode_number = offset_free_blocks_stack_bottom + sizeof(free_blocks_stack_bottom);//空闲盘块栈底 (4 bytes)

	const static unsigned int save_size = offset_root_inode_number + sizeof(root_inode_number); //超级块在文件中保存所占的大小

	char* getResetData();//获取超级快初始数据

	unsigned int getSaveData(char*& data);//获取当前超级块保存的数据

	bool save(fstream* file);//将超级块的数据保存到文件

	bool load(fstream* file);//从文件中初始化超级块

	bool loadFreeBlockStack(fstream* file, unsigned int block_number);//重置空闲盘块栈区

	int reloadFreeInodeStack(fstream* file);//填充空闲i节点栈区

    void saveUsers(fstream* file);//保存用户信息

    void readUsers(fstream* file);//读取用户信息

    void saveGroups(fstream* file);//保存组信息

    void readGroups(fstream* file);//读取组信息

    string getUsernameByUid(unsigned int uid);//通过用户名获取用户id

    string getGroupNameByGid(unsigned int gid);//通过组名获取组id
};

#endif // SUPERBLOCK_H
