#include "Dentry.h"
#include "SuperBlock.h"
#include <vector>
#include "iostream"
using namespace std;
extern SuperBlock super_block;//超级块对象
extern Dentry root_dentry;//根目录对象
unsigned int allocateFreeBlock(fstream *file);
unsigned int allocateFreeInode(fstream *file);
bool releaseBlock(fstream* file, unsigned int block_number);
bool releaseInode(fstream* file, unsigned int inode_number);
bool checkName(const char* name);
vector<string> split(string str, const char* spl);
bool changeDirectory(fstream* file, Dentry* &dentry, const char* dir_name, bool isTip = true);

Dentry::Dentry()
{
}

Dentry::~Dentry()
{
}

/**
 * @brief 设置文件名
 * @param p_name 文件名
 * @param length 文件名长度
 */
void Dentry::setName(const char* p_name, int length)
{
	int i;
	for(i = 0; i < DENTRY_NAME_LENGTH; i++) {
		this->name[i] = i<length ? p_name[i] : 0;
	}
}

/**
 * @brief 加载子目录项
 * @param file 文件流指针
 * @return  是否获取成功
 */
bool Dentry::loadChildren(fstream* file)
{
	Inode inode;
	inode.number = this->inode_number;
	inode.load(file);//加载目录项对应的i节点
	if(inode.file_type != FILE_TYPE_DIRECTORY) {
		return false;
	}
	unsigned int block_number, i, j;
	char* temp_name = new char[DENTRY_NAME_LENGTH];
	unsigned int temp_inode_number;
	this->clearChildren();//清空子目录项
	Dentry* c_dentry = nullptr;
	unsigned int count = inode.size / this->save_size;
	for(i = 0; i < INODE_DIRECT_ADDRESS_COUNT; i++) {//从文件读入子目录项，首先遍历4个直接地址
		if(inode.address[i] == BLOCK_NUMBER_OF_SUPER_BLOCK) {
			break;
		}
		if(count == 0) {
			break;
		}
		block_number = inode.address[i];
		file->seekp(block_number * super_block.per_block_size, std::ios::beg);
		for(j = 0; j < super_block.per_block_size / this->save_size; j++) {//从盘块中读入
			file->read(temp_name, DENTRY_NAME_LENGTH);
			file->read((char*)&temp_inode_number, sizeof(temp_inode_number));
			if(temp_inode_number >= INODE_AMOUNT) {
				break;
			} else {
				c_dentry = new Dentry();
				c_dentry->setName(temp_name, DENTRY_NAME_LENGTH);
				c_dentry->inode_number = temp_inode_number;
				c_dentry->parent = this;
				this->children.insert(pair<string, Dentry*>(c_dentry->name, c_dentry));
				count --;
			}
			if(count == 0) {
				break;
			}
		}//暂时只做到直接地址
	}
	if(this->children.empty()) {
		Dentry *self_dentry = new Dentry(), *parent_dentry = new Dentry();
		self_dentry->setName(".", 1);
		parent_dentry->setName("..", 2);
		self_dentry->inode_number = this->inode_number;
		parent_dentry->inode_number = this->parent == nullptr ? this->inode_number : this->parent->inode_number;
		this->children.insert(pair<string, Dentry*>(".", self_dentry));
		this->children.insert(pair<string, Dentry*>("..", parent_dentry));
	}
	return true;
}

/**
 * @brief 清空子目录项
 * @return 清除的结果
 */
bool Dentry::clearChildren()
{
	map<string, Dentry*> empty_map;
	empty_map.swap(this->children);
	return true;
}

/**
 * @brief 检测是否存在相同名字的子目录项
 * @param newName
 * @return true 存在，false 不存在
 */
bool Dentry::hasSameName(const char * newName)
{
	if(!this->children.empty()) {
		map<string, Dentry*>::iterator it;
		it = this->children.find(newName);
		return it != this->children.end(); //如果迭代器到达map末尾，说明没有同名文件
	} else {
		return false;
	}
}

/**
 * @brief 获取本目录保存的数据
 * @param data 字符指针
 * @return 数据长度
 */
unsigned int Dentry::getDirSavaData(char*& data)
{
	if(this->children.empty()) {
		Dentry *self_dentry = new Dentry(), *parent_dentry = new Dentry();
		self_dentry->setName(".", 1);
		parent_dentry->setName("..", 2);
		self_dentry->inode_number = this->inode_number;
		parent_dentry->inode_number = this->parent == nullptr ? this->inode_number : this->parent->inode_number;
		this->children.insert(pair<string, Dentry*>(".", self_dentry));
		this->children.insert(pair<string, Dentry*>("..", parent_dentry));
	}
	unsigned int size = this->children.size() * this->save_size;
	data = new char[size];
	map<string, Dentry*>::iterator it;
	unsigned int i = 0, j;
	for(it = this->children.begin(); it != this->children.end(); it++) {
		for(j = 0; j < DENTRY_NAME_LENGTH; j++) {
			data[save_size * i + this->offset_name + j] = it->second->name[j];
		}
		convertToBinaryNotNew(data + save_size * i + this->offset_inode_number, it->second->inode_number);
		i++;
	}
	return size;
}

/**
 * @brief 保存目录信息
 * @param file 文件流指针
 * @return 是否保存成功
 */
bool Dentry::saveDir(fstream* file)
{
	char* data;
	unsigned int size = this->getDirSavaData(data);
	Inode inode;
	inode.number = this->inode_number;
	inode.load(file);
	inode.freeBlocksBySize(file, size);
	if(size != 0) {
		unsigned int i;
		for(i = 0; i < INODE_DIRECT_ADDRESS_COUNT; i++) {
			if(size >= i * super_block.per_block_size) {
				if(inode.address[i] == BLOCK_NUMBER_OF_SUPER_BLOCK) {
					inode.address[i] = allocateFreeBlock(file);
				}
				file->seekp(inode.address[i] * super_block.per_block_size, std::ios::beg);
				file->write(data + i * super_block.per_block_size, size - i * super_block.per_block_size >= super_block.per_block_size ?
				            super_block.per_block_size : size - i * super_block.per_block_size);
			} else {
				break;
			}
		}//先只做直接地址部分
	}
	inode.save(file);
	if(size != 0) {
		releaseBinaryData(data);
	}
	return true;
}
/**
 * @brief 通过名字获取子目录项
 * @param c_name 子目录项的名称
 * @return 指向子目录项的指针
 */
Dentry* Dentry::getChildByName(fstream *file, const char* c_name)
{
	if(this->children.size()<=2) {
		this->loadChildren(file);
	}
	map<string, Dentry*>::iterator it;
	it = this->children.find(c_name);
	if(it == this->children.end()) { //如果迭代器到达map末尾，说明没有同名文件
		return nullptr;
	}
	return it->second;
}

/**
 * @brief 删除目录项以及其子目录项
 * @param dentry 指向待删除的子目录项的指针
 * @param file 文件流对象指针
 * @return 删除的结果
 */
bool Dentry::remove(Dentry *dentry, fstream* file)
{
	char self[DENTRY_NAME_LENGTH] = {'.'};
	char parent[DENTRY_NAME_LENGTH] = {'.','.'};
	if (!strcmp(dentry->name, self) || !strcmp(dentry->name, parent)) {//检查文件名称是不是.和..，如果是则取消递归操作
		return true;
	}
	dentry->loadChildren(file);
	map<string, Dentry*>::iterator it;
	for(it = dentry->children.begin(); it != dentry->children.end();) {//遍历子目录项
		remove((it++)->second, file);//递归调用自身
		if (it == dentry->children.end()) {
			break;
		}
	}
	Inode *inode = new Inode();
	inode->number = dentry->inode_number;
	inode->load(file);//读取目录项i节点信息
	unsigned int i;
	for(i = 0; i < INODE_DIRECT_ADDRESS_COUNT && i <= inode->size / super_block.per_block_size; i++) {
		if(inode->address[i] == BLOCK_NUMBER_OF_SUPER_BLOCK) {
			break;
		}
		releaseBlock(file, inode->address[i]);//释放i节点所指向的盘块
	}
	releaseInode(file, inode->number);//释放i节点
	delete inode;
	if(dentry->parent != nullptr) {
		dentry->parent->children.erase(dentry->name);//父目录删除子目录
		dentry->parent->saveDir(file);//保存父目录
	}
	return true;
}

/**
 * @brief 创建一个空的文件
 * @param file 文件流对象指针
 * @param parent_dentry 父目录对象指针
 * @param 新文件的名称
 * @return 新文件的目录项指针，如果创建失败，返回的是nullptr
 */
Dentry* Dentry::createEmptyFile(fstream* file, Dentry* parent_dentry, const char* name)
{
	if (!checkName(name)) {//检查文件名是否合法
		return nullptr;
	}
	Dentry *c_dentry = new Dentry();
	Inode inode;
	inode.number = allocateFreeInode(file);//分配i节点
	if (inode.number == INODE_AMOUNT) {
		delete c_dentry;
		cout << "failed! there are no free inodes."<<endl;
		return nullptr;
	}
	inode.address[0] = allocateFreeBlock(file);//分配盘块
	if (inode.address[0] == 0) {
		delete c_dentry;
		allocateFreeInode(file);
		cout << "failed! there are no free blocks."<<endl;
		return nullptr;
	}
	inode.size = 0;
	inode.authority = super_block.umask;
	inode.file_type = FILE_TYPE_NORMAL;
	inode.modify_time = time(0);
	inode.create_time = time(0);
	inode.user_id = super_block.current_user->uid;
	inode.group_id = super_block.current_user->gid;
	c_dentry->setName(name, strlen(name));
	c_dentry->inode_number = inode.number;
	if (parent_dentry->children.empty()) {
		parent_dentry->loadChildren(file);
	}
	parent_dentry->children.insert(pair<string, Dentry*>(name, c_dentry));
	inode.save(file);
	parent_dentry->saveDir(file);
	return c_dentry;
}

/**
 * @brief 保存文件内容
 * @param file 文件流对象指针
 * @param content 文件的内容
 * @param size 文件的大小
 * @return 保存是否成功
 */
bool Dentry::saveFileContent(fstream* file, const char* content, unsigned int size)
{
	Inode inode;
	inode.number = this->inode_number;
	inode.load(file);
	if (inode.file_type != FILE_TYPE_NORMAL) {
		return false;
	}
	inode.freeBlocksBySize(file, size);
	if(size != 0) {
		unsigned int i;
		for(i = 0; i < INODE_DIRECT_ADDRESS_COUNT; i++) {
			if(size >= i * super_block.per_block_size) {
				if(inode.address[i] == BLOCK_NUMBER_OF_SUPER_BLOCK) {
					inode.address[i] = allocateFreeBlock(file);
				}
				file->seekp(inode.address[i] * super_block.per_block_size, std::ios::beg);
				file->write(content + i * super_block.per_block_size, size - i * super_block.per_block_size >= super_block.per_block_size ?
				            super_block.per_block_size : size - i * super_block.per_block_size);
			} else {
				break;
			}
		}//先只做直接地址部分
	}
	inode.modify_time = time(0);
	inode.save(file);
	return true;
}

/**
 * @brief 通过路径获取目录项
 * @param file 文件流对象指针
 * @param p_dentry 父目录
 * @param name 目录的路径
 * @return 目标目录项
 */

Dentry* Dentry::getDentryByPathName(fstream* file, Dentry* p_dentry, string name)
{
	unsigned int i;
	for(i = 0; i<name.size(); i++) {
		if (name[i] != '/') {
			break;
		}
	}
	if (i == name.size()) {
		return &root_dentry;
	}
	vector<string> tokens = split(name, "/");
	vector<string>::iterator it = tokens.end();
	it --;
	string last = *it;
	tokens.pop_back();
	string dir_name = name[0] == '/' ? "/" : "./";
	it = tokens.begin();
	while (it != tokens.end()) {
		dir_name += *it;
		dir_name += "/";
		it ++;
	}
	vector<string> empty_vec;
	empty_vec.swap(tokens);
	Dentry* temp_dentry = p_dentry, *target_dentry;
	if (!changeDirectory(file, temp_dentry, dir_name.c_str(), false)) {
		return nullptr;
	}
	return target_dentry = temp_dentry->getChildByName(file, last.c_str());
}

/**
 * @brief 复制目录项
 * @param file 文件流对象指针
 * @param o_dentry 源目录项
 * @param tar_dentry 目标目录项
 * @param p_tar_dentry 目标目录项的父目录
 * @return 复制的结果
 */
bool Dentry::copy(fstream* file, Dentry* o_dentry, Dentry* tar_dentry, Dentry* p_tar_dentry)
{
	Inode inode, c_inode;
	inode.number = o_dentry->inode_number;
	inode.load(file);
	if (inode.file_type == FILE_TYPE_NORMAL) {//如果源目录项是一个文件
		c_inode.number = allocateFreeInode(file);
		if (c_inode.number == INODE_AMOUNT) {
			cout << "failed! there are no free inodes" <<endl;
			return false;
		}
		c_inode.file_type = FILE_TYPE_NORMAL;
		c_inode.size = 0;
		c_inode.authority = inode.authority;
		c_inode.user_id = inode.user_id;
		c_inode.group_id = inode.group_id;
		c_inode.create_time = time(0);
		c_inode.modify_time = time(0);
		c_inode.save(file);
		tar_dentry->inode_number = c_inode.number;
		tar_dentry->saveFileContent(file, inode.getContent(file).c_str(), inode.size);
		p_tar_dentry->children.insert(pair<string, Dentry*>(tar_dentry->name, tar_dentry));
		p_tar_dentry->saveDir(file);
		return true;
	}
	if (inode.file_type == FILE_TYPE_DIRECTORY) {//如果是个目录
		if (o_dentry->children.empty()) {
			o_dentry->loadChildren(file);
		}
		if (p_tar_dentry->children.empty()) {
			p_tar_dentry->loadChildren(file);
		}
		Dentry* temp_dentry;
		temp_dentry = new Dentry();
		temp_dentry->setName(tar_dentry->name, strlen(tar_dentry->name));
		Inode c_inode;
		c_inode.number = allocateFreeInode(file);
		c_inode.file_type = FILE_TYPE_DIRECTORY;
		c_inode.size = 0;
		c_inode.authority = inode.authority;
		c_inode.user_id = inode.user_id;
		c_inode.group_id = inode.group_id;
		c_inode.modify_time = time(0);
		c_inode.create_time = time(0);
		c_inode.save(file);
		temp_dentry->inode_number = c_inode.number;
		temp_dentry->parent = p_tar_dentry;
		temp_dentry->saveDir(file);
		p_tar_dentry->children.insert(pair<string, Dentry*>(temp_dentry->name, temp_dentry));
		p_tar_dentry->saveDir(file);
		map<string, Dentry*>::iterator it = o_dentry->children.begin();
		for(; it != o_dentry->children.end(); it++) {
			if (it->first == "." || it->first == "..") {
				continue;
			}
			copy(file, it->second, it->second, temp_dentry);//递归调用自身
		}
		return it == o_dentry->children.end();
	}
	return false;
}


