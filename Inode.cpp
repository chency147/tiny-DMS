#include "Inode.h"
#include "SuperBlock.h"
#include <iostream>
extern SuperBlock super_block;//超级块对象

bool releaseBlock(fstream* file, unsigned int block_number);
template<class T>
unsigned int convertToBinaryNotNew(char* b_data, T o_data);
template<class T>
unsigned int convertArrayToBinaryNotNew(char* b_data, T* o_data, unsigned int size);

Inode::Inode()
{
}

Inode::~Inode()
{
}

/**
 * @brief 获取单个i节点的初始化数据
 * @param data 字符串头指针
 * @param i_number i节点编号
 * @return 数据的长度
 */
unsigned int Inode::getSingleResetData(char* data, unsigned int i_number)
{
	Inode inode_temp;
	unsigned int i = 0;
	inode_temp.number = i_number;
	inode_temp.size = 0;
	inode_temp.file_type = FILE_TYPE_VOID;
	inode_temp.authority = 0x000000000;
	for(i = 0 ; i < INODE_DIRECT_ADDRESS_COUNT; i++) {
		inode_temp.address[i] = BLOCK_NUMBER_OF_SUPER_BLOCK;
	}
	inode_temp.address_1 = BLOCK_NUMBER_OF_SUPER_BLOCK;
	inode_temp.address_2 = BLOCK_NUMBER_OF_SUPER_BLOCK;
	inode_temp.create_time = 0;
	inode_temp.modify_time = 0;
	inode_temp.link_count = 0;
	inode_temp.user_id = 0;
	inode_temp.group_id = 0;
	char *temp_data;
	inode_temp.getSaveData(temp_data);
	for(i = 0; i < Inode::save_size; i++) {
		*(data + i) = temp_data[i];
	}
	delete [] temp_data;
	return Inode::save_size;
}

/**
 * @brief 获取当前i节点保存的数据
 * @param data 字符串头指针
 * @return 数据的长度
 */
unsigned int Inode::getSaveData(char*& data)
{
	data = new char[this->save_size];
	convertToBinaryNotNew(data + offset_number, this->number);
	convertToBinaryNotNew(data + offset_size, this->size);
	convertToBinaryNotNew(data + offset_file_type, this->file_type);
	convertToBinaryNotNew(data + offset_authority, this->authority);
	convertArrayToBinaryNotNew(data + offset_address, this->address, INODE_DIRECT_ADDRESS_COUNT);
	convertToBinaryNotNew(data + offset_address_1, this->address_1);
	convertToBinaryNotNew(data + offset_address_2, this->address_2);
	convertToBinaryNotNew(data + offset_create_time, this->create_time);
	convertToBinaryNotNew(data + offset_modify_time, this->modify_time);
	convertToBinaryNotNew(data + offset_link_count, this->link_count);
	convertToBinaryNotNew(data + offset_user_id, this->user_id);
	convertToBinaryNotNew(data + offset_group_id, this->group_id);
	return this->save_size;
}

/**
 * @brief 将当前i节点的数据保存到文件
 * @param file 文件流对象指针
 * @return 保存的结果
 */
bool Inode::save(fstream* file)
{
	char *data_p;
	this->getSaveData(data_p);
	file->seekp(this->getSaveOffset(), std::ios::beg);
	file->write(data_p, Inode::save_size);
	return true;
}

/**
 * @brief 获取当前i节点在文件中的偏移值
 * @return 偏移值
 */
unsigned long Inode::getSaveOffset()
{
	unsigned int per_count = BLOCK_SIZE / this->save_size;
	return (BLOCK_NUMBER_OF_FIRST_INODE + this->number / per_count) * BLOCK_SIZE + (this->number % per_count) * save_size;
}

/**
 * @brief 从文件载入i节点的内容
 * @param file 文件流对象指针
 * @return 是否获取成功
 */
bool Inode::load(fstream* file)
{
	if(this->number >= INODE_AMOUNT) {
		return false;
	}
	unsigned int i;
	file->seekp(this->getSaveOffset(), std::ios::beg);
	file->read((char*)&this->number, sizeof(this->number));
	file->read((char*)&this->size, sizeof(this->size));
	file->read((char*)&this->file_type, sizeof(this->file_type));
	file->read((char*)&this->authority, sizeof(this->authority));
	for(i = 0; i < INODE_DIRECT_ADDRESS_COUNT; i++) {
		file->read((char*)&this->address[i], sizeof(this->address[i]));
	}
	file->read((char*)&this->address_1, sizeof(this->address_1));
	file->read((char*)&this->address_2, sizeof(this->address_2));
	file->read((char*)&this->create_time, sizeof(this->create_time));
	file->read((char*)&this->modify_time, sizeof(this->modify_time));
	file->read((char*)&this->link_count, sizeof(this->link_count));
	file->read((char*)&this->user_id, sizeof(this->user_id));
	file->read((char*)&this->group_id, sizeof(this->group_id));
	return true;
}

/**
 * @brief 释放多余的盘块
 * @param file 文件流对象指针
 * @param new_size 新内容所占用的空间
 * @return 实际保存的大小
 */
unsigned int Inode::freeBlocksBySize(fstream* file, unsigned int new_size)
{
	unsigned int old_count = this->size / super_block.per_block_size + 1;
	unsigned int new_count = new_size / super_block.per_block_size + 1;
	if(old_count <= new_count) {
		this->size = new_size;
		this->save(file);
		return 0;
	}
	unsigned int start = new_count, end = old_count - 1, i;
	for(i = start; i <= end && i < INODE_DIRECT_ADDRESS_COUNT; i++) {
		releaseBlock(file, this->address[i]);//调用归还盘块的函数
		this->address[i] = BLOCK_NUMBER_OF_SUPER_BLOCK;
	}//只做到直接地址
	this->size = new_size;
	this->save(file);
	return old_count - new_count;
}
/**
 * @brief 获取i节点对应文件的内容。字符串形式
 * @param file 文件流对象指针
 * @return 文件的内容
 */
string Inode::getContent(fstream* file)
{
	if (this->size == 0) {
		return "";
	}
	string result;
	char *data = new char[this->size + 1];
	unsigned int i, curr = 0, in_size;
	for (i = 0; i < INODE_DIRECT_ADDRESS_COUNT; i++) {
		if (this->size > i * super_block.per_block_size) {
			file->seekp(super_block.per_block_size * this->address[i], std::ios::beg);
			in_size = this->size - i * super_block.per_block_size > super_block.per_block_size?
			          super_block.per_block_size : this->size - i * super_block.per_block_size;
			file->read(data+curr, in_size);
			curr += in_size;
		} else {
			break;
		}
	}//只做到直接地址
	data[this->size] = 0;
	result = data;
	releaseBinaryData(data);
	return result;
}
/**
 * @brief 获取i节点对应文件的内容。二进制形式
 * @param file 文件流对象指针
 * @return 文件的内容
 */
char* Inode::getBinaryContent(fstream* file)
{
	if (this->size == 0) {
		char *a= new char[1];
		a[0] = '\0';
		return a;
	}
	char *data = new char[this->size];
	unsigned int i, curr = 0, in_size;
	for (i = 0; i < INODE_DIRECT_ADDRESS_COUNT; i++) {
		if (this->size > i * super_block.per_block_size) {
			file->seekp(super_block.per_block_size * this->address[i], std::ios::beg);
			in_size = this->size - i * super_block.per_block_size > super_block.per_block_size?
			          super_block.per_block_size : this->size - i * super_block.per_block_size;
			file->read(data+curr, in_size);
			curr += in_size;
		} else {
			break;
		}
	}//只做到直接地址
	return data;
}
/**
 * @brief 获取权限对应的字符串
 * @return 权限的字符串形式
 */
string Inode::getStringAuthority()
{
    char as[3] = {'r', 'w', 'x'};
    string result = "";
    short standerd = 0b100000000;
    unsigned int i;
    for (i = 0; i<9;i++){
        if((this->authority&(standerd>>i)) == standerd>>i){
			result += as[i%3];
        }else{
			result += '-';
        }
    }
    return result;
}
/**
 * @brief 获取文件类型对应的字母
 * @return rt
 */
char Inode::getCharFileType()
{
	if (file_type == FILE_TYPE_DIRECTORY){
		return 'd';
	}
	return '-';
}
/**
 * @brief 判断是否可以更改权限
 * @return true 可以，false 不可以
 */
bool Inode::canChmod()
{
	if (this->user_id == super_block.current_user->uid){
		return true;
	}
	map<string, User>::iterator it = super_block.users.find("root");
	return super_block.current_user->uid == it->second.uid;
}
/**
 * @brief 判断是否可读
 * @return true 可以，false 不可以
 */
bool Inode::canRead()
{
    if (this->user_id == super_block.current_user->uid){
        if (0b100000000&this->authority){
            return true;
        }
    }
    if (this->group_id == super_block.current_user->gid){
		if (0b000100000&this->authority){
            return true;
        }
    }
    if (0b000000100&this->authority){
		return true;
    }else{
		return false;
    }
}
/**
 * @brief 判断是否可写
 * @return true 可以，false 不可以
 */
bool Inode::canWrite()
{
	if (this->user_id == super_block.current_user->uid){
        if (0b010000000&this->authority){
            return true;
        }
    }
    if (this->group_id == super_block.current_user->gid){
		if (0b000010000&this->authority){
            return true;
        }
    }
    if (0b000000010&this->authority){
		return true;
    }else{
		return false;
    }
}
/**
 * @brief 判断是否可执行
 * @return true 可以，false 不可以
 */
bool Inode::canExecute()
{
	if (this->user_id == super_block.current_user->uid){
        if (0b001000000&this->authority){
            return true;
        }
    }
    if (this->group_id == super_block.current_user->gid){
		if (0b000001000&this->authority){
            return true;
        }
    }
    if (0b000000001&this->authority){
		return true;
    }else{
		return false;
    }
}
/**
 * @brief 判断是否可以删除
 * @return true 可以，false 不可以
 */
bool Inode::canDelete()
{
	return this->user_id == super_block.current_user->uid;
}
/**
 * @brief 判断是否可以更改用户id或者组id
 * @return true 可以，false 不可以
 */
bool Inode::canChangeUidOrGid()
{
	map<string, User>::iterator it = super_block.users.find("root");
	return super_block.current_user->uid == it->second.uid;
}


