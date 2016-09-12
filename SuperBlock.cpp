#include "SuperBlock.h"
#include <iostream>
#include <string.h>
extern SuperBlock super_block;//超级块对象
extern Dentry root_dentry;//根目录对象
extern Inode root_inode;//根目录的i节点
extern Dentry *current_dentry;//当前目录
template<class T>
unsigned int convertToBinaryNotNew(char* b_data, T o_data);
template<class T>
static unsigned int convertArrayToBinaryNotNew(char* b_data, T* o_data, unsigned int size);
bool changeDirectory(fstream* file, Dentry* &dentry, const char* dir_name, bool isTip = true);

SuperBlock::SuperBlock()
{
}

SuperBlock::~SuperBlock()
{
}

/**
 * @brief 获取初始化的超级块数据
 * @return 超级块数据
 */
char* SuperBlock::getResetData()
{
	unsigned int i, j, curr = 0;
	char* reset_data = new char[save_size];


	//初始化盘块数量
	this->block_amount = BLOCK_AMOUNT;
	for(i = 0 ; i < sizeof(this->block_amount); i++) {
		reset_data[curr] = ((char*) & (this->block_amount))[i];
		curr ++;
	}
	//初始化i节点数量
	this->inode_amount = INODE_AMOUNT;
	for(i = 0 ; i < sizeof(this->inode_amount); i++) {
		reset_data[curr] = ((char*) & (this->inode_amount))[i];
		curr ++;
	}
	//初始化盘块大小
	this->per_block_size = BLOCK_SIZE;
	for(i = 0 ; i < sizeof(this->per_block_size); i++) {
		reset_data[curr] = ((char*) & (this->per_block_size))[i];
		curr ++;
	}
	//初始化空闲i节点栈
	for(i = 0; i < FREE_INODE_STACK_SIZE; i++) {
		free_inodes_stack[i] = i + 1;
		for(j = 0 ; j < sizeof(this->free_inodes_stack[i]); j++) {
			reset_data[curr] = ((char*) & (this->free_inodes_stack[i]))[j];
			curr ++;
		}
	}
	//初始化空闲i节点数量
	this->free_inodes_amount = INODE_AMOUNT;
	for(i = 0 ; i < sizeof(this->free_inodes_amount); i++) {
		reset_data[curr] = ((char*) & (this->free_inodes_amount))[i];
		curr ++;
	}
	//初始化空闲盘块栈
	for(i = 0; i < FREE_BLOCK_STACK_SIZE; i++) {
		free_blocks_stack[i] = 0;
		for(j = 0 ; j < sizeof(this->free_blocks_stack[i]); j++) {
			reset_data[curr] = ((char*) & (this->free_blocks_stack[i]))[j];
			curr ++;
		}
	}

	//初始化空闲盘块数量
	this->free_blocks_amount = BLOCK_AMOUNT - INODE_AMOUNT / (this->per_block_size / Inode::save_size) - 1;
	for(i = 0 ; i < sizeof(this->free_blocks_amount); i++) {
		reset_data[curr] = ((char*) & (this->free_blocks_amount))[i];
		curr ++;
	}

	//初始化修改时间
	this->modify_time = time(0);
	for(i = 0 ; i < sizeof(this->modify_time); i++) {
		reset_data[curr] = ((char*) & (this->modify_time))[i];
		curr ++;
	}

	//初始化创建时间
	this->create_time = time(0);
	for(i = 0 ; i < sizeof(this->create_time); i++) {
		reset_data[curr] = ((char*) & (this->create_time))[i];
		curr ++;
	}

	//初始化空闲盘块栈底
	this->free_blocks_stack_bottom = 0;
	for(i = 0 ; i < sizeof(this->free_blocks_stack_bottom); i++) {
		reset_data[curr] = ((char*) & (this->free_blocks_stack_bottom))[i];
		curr ++;
	}

	//初始化根i节点编号
	this->root_inode_number = 0;
	for(i = 0 ; i < sizeof(this->root_inode_number); i++) {
		reset_data[curr] = ((char*) & (this->root_inode_number))[i];
		curr ++;
	}

	return reset_data;
}

/**
 * @brief 获取当前超级块保存的数据
 * @param data 字符串头指针，用于保存数据
 * @return 数据的长度
 */
unsigned int SuperBlock::getSaveData(char*& data)
{
	data = new char[this->save_size];
	convertToBinaryNotNew(data + offset_block_amount, this->block_amount);
	convertToBinaryNotNew(data + offset_inode_amount, this->inode_amount);
	convertToBinaryNotNew(data + offset_per_block_size, this->per_block_size);
	convertArrayToBinaryNotNew(data + offset_free_inodes_stack, this->free_inodes_stack, FREE_INODE_STACK_SIZE);
	convertToBinaryNotNew(data + offset_free_inodes_amount, this->free_inodes_amount);
	convertArrayToBinaryNotNew(data + offset_free_blocks_stack, this->free_blocks_stack, FREE_BLOCK_STACK_SIZE);
	convertToBinaryNotNew(data + offset_free_blocks_amount, this->free_blocks_amount);
	convertToBinaryNotNew(data + offset_modify_time, this->modify_time);
	convertToBinaryNotNew(data + offset_create_time, this->create_time);
	convertToBinaryNotNew(data + offset_free_blocks_stack_bottom, this->free_blocks_stack_bottom);
	convertToBinaryNotNew(data + offset_root_inode_number, this->root_inode_number);
	return this->save_size;
}

/**
 * @brief 将超级块的数据保存到文件
 * @param file 文件流对象指针
 * @return 保存的结果
 */
bool SuperBlock::save(fstream* file)
{
	char *data_p;
	this->getSaveData(data_p);//获取当前数据
	file->seekp(BLOCK_NUMBER_OF_SUPER_BLOCK * this->per_block_size, std::ios::beg);//文件指针定位
	file->write(data_p, SuperBlock::save_size);
	releaseBinaryData(data_p);
	return true;
}

/**
 * @brief 从文件读入超级块信息
 * @param file 文件流指针
 * @return 获取的结果
 */
bool SuperBlock::load(fstream* file)
{
	unsigned int i;
	file->seekp(BLOCK_NUMBER_OF_SUPER_BLOCK * this->per_block_size, std::ios::beg);
	file->read((char*)&this->block_amount, sizeof(this->block_amount));
	file->read((char*)&this->inode_amount, sizeof(this->inode_amount));
	file->read((char*)&this->per_block_size, sizeof(this->per_block_size));
	for(i = 0; i < FREE_INODE_STACK_SIZE; i++) {
		file->read((char*)&this->free_inodes_stack[i], sizeof(this->free_inodes_stack[i]));
	}
	file->read((char*)&this->free_inodes_amount, sizeof(this->free_inodes_amount));
	for(i = 0; i < FREE_BLOCK_STACK_SIZE; i++) {
		file->read((char*)&this->free_blocks_stack[i], sizeof(this->free_blocks_stack[i]));
	}
	file->read((char*)&this->free_blocks_amount, sizeof(this->free_blocks_amount));
	file->read((char*)&this->modify_time, sizeof(this->modify_time));
	file->read((char*)&this->create_time, sizeof(this->create_time));
	file->read((char*)&this->free_blocks_stack_bottom, sizeof(this->free_blocks_stack_bottom));
	file->read((char*)&this->root_inode_number, sizeof(this->root_inode_number));
	for(i = FREE_BLOCK_STACK_SIZE - 1; i >= 0; i--) {
		if(this->free_blocks_stack[i] != 0) {
			this->curr_block_stack = i;
			break;
		}
	}
	if (i < 0) {
		this->curr_block_stack = 0;
	}
	for(i = FREE_INODE_STACK_SIZE - 1; i >= 0; i--) {
		if(this->free_inodes_stack[i] != INODE_AMOUNT) {
			this->curr_inode_stack = i;
			break;
		}
	}
	if (i < 0) {
		this->curr_inode_stack = 0;
	}
	return true;
}

/**
 * @brief 从空闲盘块组头一个盘块获取心的空闲盘块栈信息
 * @param file 文件流对象指针
 * @param block_number 带有盘快栈信息的盘块
 */
bool SuperBlock::loadFreeBlockStack(fstream* file, unsigned int block_number)
{
	if(block_number == BLOCK_NUMBER_OF_SUPER_BLOCK) {
		return false;
	}
	unsigned int i;
	file->seekp(block_number * this->per_block_size, std::ios::beg);
	unsigned int stack_size;
	file->read((char*)&stack_size, sizeof(stack_size));
	for(i = 0; i < FREE_BLOCK_STACK_SIZE; i++) {
		if(i < stack_size) {
			file->read((char*)&this->free_blocks_stack[i], sizeof(this->free_blocks_stack[i]));
		} else {
			this->free_blocks_stack[i] = BLOCK_NUMBER_OF_SUPER_BLOCK;
		}
	}
	for(i = FREE_BLOCK_STACK_SIZE - 1; i >= 0; i--) {
		if(this->free_blocks_stack[i] != 0) {
			this->curr_block_stack = i;
			break;
		}
	}
	this->modify_time = time(0);
	this->free_blocks_stack_bottom = block_number;
	return this->save(file);
}

int SuperBlock::reloadFreeInodeStack(fstream* file)
{
	unsigned int i;
	int j = FREE_INODE_STACK_SIZE - 1, count = 0;
	Inode inode;
	for(i = 0; i < this->inode_amount && j >= 0; i++) {
		inode.number = i;
		inode.load(file);
		if(inode.file_type == FILE_TYPE_VOID) {
			this->free_inodes_stack[j] = i;
			j--;
			count ++;
		}
	}
	for(i = count; i < FREE_INODE_STACK_SIZE; i++) {
		this->free_inodes_stack[i] = INODE_AMOUNT;
	}
	this->curr_inode_stack = count - 1;
	this->modify_time = time(0);
	this->save(file);
	return count;
}

void SuperBlock::saveUsers(fstream* file)
{
	Dentry *dentry = &root_dentry;
	changeDirectory(file, dentry, DATA_PATH, false);
	if (dentry != nullptr) {
		Dentry *c_dentry = dentry->getChildByName(file, USER_DATA_FILENAME);
		if (c_dentry != nullptr) {
			unsigned int curr = 0, i;
			char *data = new char[this->users.size() * User::save_size];
			char *temp_data;
			map<string, User>::iterator it = this->users.begin();
			while(it != this->users.end()) {
				temp_data = it->second.getSaveData();
				for (i = 0; i<User::save_size; i++) {
					data[curr] = temp_data[i];
					curr ++;
				}
				delete [] temp_data;
				it ++;
			}
			c_dentry->saveFileContent(file, data, this->users.size() * User::save_size);
		}
	}
}

void SuperBlock::readUsers(fstream* file)
{
	Dentry *dentry = &root_dentry;
	dentry->loadChildren(file);
	changeDirectory(file, dentry, DATA_PATH, false);
	if (dentry != nullptr) {
		Dentry *c_dentry = dentry->getChildByName(file, USER_DATA_FILENAME);
		if (c_dentry != nullptr) {
			Inode inode;
			inode.number = c_dentry->inode_number;
			inode.load(file);
			char* data = inode.getBinaryContent(file);
			unsigned int i, j;
			User* user = nullptr;
			for (i = 0; i<inode.size / User::save_size; i++) {
				user = new User();
				user->setName(data + i * User::save_size, USER_NAME_LENGTH);
				user->setPassword(data + i * User::save_size + User::offset_password, USER_PASSWORD_LENGTH);
				for (j = 0; j <sizeof(user->uid); j++) {
					((char*)&(user->uid))[j] = data[ i * User::save_size + User::offset_uid + j];
				}
				for (j = 0; j <sizeof(user->gid); j++) {
					((char*)&(user->gid))[j] = data[ i * User::save_size + User::offset_gid + j];
				}
				this->users.insert(pair<string, User>(user->name, *user));
			}
		}
	}
}

void SuperBlock::saveGroups(fstream* file)
{
	Dentry *dentry = &root_dentry;
	changeDirectory(file, dentry, DATA_PATH, false);
	if (dentry != nullptr) {
		Dentry *c_dentry = dentry->children.find(GROUP_DATA_FILENAME)->second;
		if (c_dentry != nullptr) {
			unsigned int curr = 0, i;
			char *data = new char[this->groups.size() * Group::save_size];
			char *temp_data;
			map<string, Group>::iterator it = this->groups.begin();
			while(it != this->groups.end()) {
				temp_data = it->second.getSaveData();
				for (i = 0; i<Group::save_size; i++) {
					data[curr] = temp_data[i];
					curr ++;
				}
				delete [] temp_data;
				it ++;
			}
			c_dentry->saveFileContent(file, data, this->groups.size() * Group::save_size);
		}
	}
}

void SuperBlock::readGroups(fstream* file)
{
	Dentry *dentry = &root_dentry;
	dentry->loadChildren(file);
	changeDirectory(file, dentry, DATA_PATH, false);
	if (dentry != nullptr) {
		Dentry *c_dentry = dentry->getChildByName(file, GROUP_DATA_FILENAME);
		if (c_dentry != nullptr) {
			Inode inode;
			inode.number = c_dentry->inode_number;
			inode.load(file);
			char* data = inode.getBinaryContent(file);
			unsigned int i, j;
			Group* group = nullptr;
			for (i = 0; i<inode.size / Group::save_size; i++) {
				group = new Group();
				group->setName(data + i * Group::save_size, GROUP_NAME_LENGHT);
				for (j = 0; j <sizeof(group->gid); j++) {
					((char*)&(group->gid))[j] = data[ i * Group::save_size + Group::offset_gid + j];
				}
				this->groups.insert(pair<string, Group>(group->name, *group));
			}
			delete [] data;
		}
	}
}

string SuperBlock::getUsernameByUid(unsigned int uid)
{
    map<string, User>::iterator it = this->users.begin();
    while(it != this->users.end()){
        if (it->second.uid == uid){
            return it->first;
        }
        it ++;
    }
    return "not found";
}

string SuperBlock::getGroupNameByGid(unsigned int gid)
{
	map<string, Group>::iterator it = this->groups.begin();
    while(it != this->groups.end()){
        if (it->second.gid == gid){
            return it->first;
        }
        it ++;
    }
    return "not found";
}

