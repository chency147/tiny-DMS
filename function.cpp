#include "SuperBlock.h"
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>

using namespace std;
extern SuperBlock super_block;//超级块对象
extern Dentry root_dentry;//根目录对象
extern Inode root_inode;//根目录的i节点
extern Dentry *current_dentry;//当前目录
extern map<string, string> help_map;

/**
 * @brief 分配i节点
 * @param file 文件流对象指针
 * @return 新的i节点编号
 */
unsigned int allocateFreeInode(fstream *file)
{
	if(super_block.free_inodes_amount == 0) {//如果空闲i节点数量为O，返回一个已定义好的无效i节点号
		return INODE_AMOUNT;
	}
	unsigned int result_inode_number;
	result_inode_number = super_block.free_inodes_stack[super_block.curr_inode_stack];//从空闲i节点栈取出栈顶
	super_block.free_inodes_stack[super_block.curr_inode_stack] = INODE_AMOUNT;
	if(super_block.curr_inode_stack == 0) {
		Inode inode;
		inode.number = result_inode_number;
		inode.load(file);
		inode.file_type = FILE_TYPE_OTHER;
		inode.save(file);
		super_block.reloadFreeInodeStack(file);//如果取出的是栈底，那么重新载入新的空闲i节点栈
	} else {
		super_block.curr_inode_stack --;//栈指针向栈底移一位
	}
	super_block.free_inodes_amount --;//空闲i节点数减一
	super_block.modify_time = time(0);//设置超级块的修改时间
	super_block.save(file);//保存超级块的内容
	return result_inode_number;//返回获取的空闲i节点编号
}

/**
 * @brief 分配一个空闲盘块
 * @param file 文件流指针
 * @return 新的空闲盘编号
 */
unsigned int allocateFreeBlock(fstream *file)
{
	if(super_block.free_blocks_amount == 0) {//如果空闲盘块数量为0，返回一个规定好的无效盘块号
		return 0;
	}
	unsigned int result_block_number;
	if(super_block.curr_block_stack == 0) {//如果到达栈底
		result_block_number = super_block.free_blocks_stack_bottom;//设置预返回盘块号为栈底
		if(super_block.free_blocks_stack[super_block.curr_block_stack] != BLOCK_NUMBER_OF_SUPER_BLOCK) {//如果下一个空闲盘块组的头一个盘块号不为0
			super_block.loadFreeBlockStack(file, super_block.free_blocks_stack[super_block.curr_block_stack]);
		}
	} else {
		result_block_number = super_block.free_blocks_stack[super_block.curr_block_stack];
		super_block.free_blocks_stack[super_block.curr_block_stack] = BLOCK_NUMBER_OF_SUPER_BLOCK;
		super_block.curr_block_stack --;//栈指针向栈底移动一位
	}
	super_block.free_blocks_amount --;//空闲盘块数量减一
	super_block.modify_time = time(0);//设置超级块的修改时间
	super_block.save(file);//保存超级块内容
	return result_block_number;//返回空闲盘块号
}

/**
 * @brief 归还盘块
 * @param file 文件流对象指针
 * @param block_number 待归还的盘块号
 * @return 归还的结果
 */
bool releaseBlock(fstream *file, unsigned int block_number)
{
	if(block_number == BLOCK_NUMBER_OF_SUPER_BLOCK) {
		return false;
	}
	if(super_block.curr_block_stack < FREE_BLOCK_STACK_SIZE - 1) {//如果空闲盘块栈未满
		super_block.curr_block_stack ++;
		super_block.free_blocks_stack[super_block.curr_block_stack] = block_number;
	} else {//如果空闲盘块栈已满
		char* data;
		int stack_size = super_block.curr_block_stack + 1, i;
		file->seekp(block_number * super_block.per_block_size, std::ios::beg);
		file->write(data, convertToBinary(data, stack_size));//保存空闲栈大小
		releaseBinaryData(data);
		file->write(data, convertToBinary(data, super_block.free_blocks_stack[0]));//保存下一盘块组头一个盘块的盘块号
		releaseBinaryData(data);
		super_block.free_blocks_stack[0] = block_number;
		for(i = 1; i < stack_size; i++) {
			file->write(data, convertToBinary(data, super_block.free_blocks_stack[i]));//写入盘块栈内空闲盘块号
			releaseBinaryData(data);
			super_block.free_blocks_stack[i] = BLOCK_NUMBER_OF_SUPER_BLOCK;
		}
		super_block.curr_block_stack = 0;
	}
	super_block.free_blocks_amount ++;//空闲盘块号加一
	super_block.modify_time = time(0);
	return super_block.save(file);
}

/**
 * @brief 归还i节点
 * @param file 文件流对象指针
 * @param inode_number 待归还的i节点
 * @return 归还的结果
 */
bool releaseInode(fstream *file, unsigned int inode_number)
{
	if(inode_number == 0) {
		return false;
	}
	if(super_block.curr_inode_stack < FREE_INODE_STACK_SIZE - 1) {//如果空闲i节点栈未满
		super_block.curr_inode_stack ++;
		super_block.free_inodes_stack[super_block.curr_inode_stack] = inode_number;
	}
	char *data = new char[Inode::save_size];
	Inode inode;
	inode.number = inode_number;
	Inode::getSingleResetData(data, inode_number);//获取i节点的初始数据
	file->seekp(inode.getSaveOffset(), std::ios::beg);
	file->write(data, Inode::save_size);//将i节点的初始数据写入模拟磁盘
	releaseBinaryData(data);
	super_block.free_inodes_amount ++;
	super_block.modify_time = time(0);
	return super_block.save(file);
}

/**
 * @brief 检查文件名是否规范
 * @param 文件名
 * @return true 规范， false 不规范
 */
bool checkName(const char* name)
{
	if(strlen(name) > DENTRY_NAME_LENGTH) {//检查目录名字长度是否规范
		cout << "failed! the lenght of the name must be less than " << DENTRY_NAME_LENGTH << "." << endl;
		return false;
	}
	unsigned int i;
	for(i = 0; i < DENTRY_NAME_LENGTH && i<strlen(name); i++) {//检查目录名称中是否有特殊字符
		if(name[i] == '\\' || name[i] == '/' || name[i] == ':' || name[i] == '*' || name[i] == '?' || name[i] == '<' || name[i] == '>' || name[i] == '|' || name[i] == '\"') {
			break;
		}
	}
	if(i < DENTRY_NAME_LENGTH && i< strlen(name)) {
		cout << "failed! the directory's name can not contain these characters: \\,/,:,*,?,<,>,|,\"" << endl;
		return false;
	}
	return true;
}

/**
 * @brief 创建子目录
 * @param file 文件流指针
 * @param dentry 目录项
 * @param dir_name 新目录名称
 * @return 是否创建成功
 */
bool makeDirectory(fstream *file, Dentry* dentry, const char* dir_name)
{
	Inode p_inode;
	p_inode.number = dentry->inode_number;
	p_inode.load(file);
	if(p_inode.file_type != FILE_TYPE_DIRECTORY) {//检查dentry是否是个目录
		cout << "failed! the dentry is not a directory." << endl;
		return false;
	}
	if (!p_inode.canWrite()) {
		cout << "permission denied." << endl;
		return false;
	}
	if (!checkName(dir_name)) {
		return false;
	}
	if(!dentry->loadChildren(file)) {//刷新当前目录的子目录项
		cout << "failed! get error when get children of the current directory." << endl;
		return false;
	}
	if(dentry->hasSameName(dir_name)) {//检查是否已存在同名目录项
		cout << "failed! there is a dentry with the same name." << endl;
		return false;
	}
	Inode n_inode;
	unsigned int block_number;
	n_inode.number = allocateFreeInode(file);//分配一个i节点
	if(n_inode.number == INODE_AMOUNT) {
		cout << "failed! there are no free inodes." << endl;
		return false;
	}
	block_number = allocateFreeBlock(file);//分配一个盘块
	if(block_number == 0) {
		releaseInode(file, n_inode.number);
		cout << "failed! there are no free blocks." << endl;
		return false;
	}
	n_inode.authority = super_block.umask;
	n_inode.address[0] = block_number;
	n_inode.file_type = FILE_TYPE_DIRECTORY;
	n_inode.create_time = time(0);
	n_inode.modify_time = time(0);
	n_inode.user_id = super_block.current_user->uid;
	n_inode.group_id = super_block.current_user->gid;//设置新的i节点的信息
	Dentry* c_dentry = new Dentry();
	c_dentry->inode_number = n_inode.number;
	c_dentry->setName(dir_name, DENTRY_NAME_LENGTH);
	c_dentry->parent = dentry;
	dentry->children.insert(pair<string, Dentry*>(dir_name, c_dentry));//将新的目录项加入到父目录中
	p_inode.size += Dentry::save_size;
	p_inode.modify_time = time(0);
	p_inode.save(file);//保存父目录的i节点
	if(!n_inode.save(file)) {
		return false;
	}
	c_dentry->saveDir(file);//保存新的目录项
	return dentry->saveDir(file);
}

/**
 * @brief 删除目录
 * @param file 文件流对象指针
 * @param dentry 父目录项指针
 * @param dir_name 待删除的子目录项的名称
 * @return 删除的结果
 */
bool removeDirectory(fstream *file, Dentry* dentry, const char* dir_name)
{
	Inode p_inode;
	p_inode.number = dentry->inode_number;
	p_inode.load(file);//载入父目录项的i节点
	if(p_inode.file_type != FILE_TYPE_DIRECTORY) {//检查父目录项是否为目录
		cout << "failed! the dentry is not a directory." << endl;
		return false;
	}
	if(!dentry->loadChildren(file)) {//载入父目录的子目录项
		cout << "failed! get error when get children of the current directory." << endl;
		return false;
	}
	Dentry* c_dentry = dentry->getChildByName(file, dir_name);//通过名字获取子目录项
	if(c_dentry == nullptr) {
		cout << "failed! no such directory." << endl;
		return false;
	}
	Inode inode;
	inode.number = c_dentry->inode_number;
	inode.load(file);//载入子目录项的i节点
	if(inode.file_type != FILE_TYPE_DIRECTORY) {//检查子目录项是否是一个目录
		cout << "failed! it's not a directory." << endl;
		return false;
	}
	if (!inode.canDelete()) {
		cout << "permission denied." << endl;
		return false;
	}
	if(inode.size > 2 * Dentry::save_size) {//检查这个子目录是否再含有子目录
		cout << "failed! the directory contains child dentries." << endl;
		return false;
	}
	return Dentry::remove(c_dentry, file);
}

/**
 * @brief 删除文件
 * @param file 文件流对象指针
 * @param dentry 父目录项指针
 * @param dir_name 待删除的子目录项的名称
 * @return 删除的结果
 */
bool remove(fstream *file, Dentry* dentry, const char* dir_name)
{
	Inode p_inode;
	p_inode.number = dentry->inode_number;
	p_inode.load(file);//载入父目录项的i节点
	if(p_inode.file_type != FILE_TYPE_DIRECTORY) {//检查父目录项是否为目录
		cout << "failed! the dentry is not a directory." << endl;
		return false;
	}
	if(!dentry->loadChildren(file)) {//载入父目录的子目录项
		cout << "failed! get error when get children of the current directory." << endl;
		return false;
	}
	Dentry* c_dentry = dentry->getChildByName(file, dir_name);//通过名字获取子目录项
	if(c_dentry == nullptr) {
		cout << "failed! no such directory." << endl;
		return false;
	}
	Inode inode;
	inode.number = c_dentry->inode_number;
	inode.load(file);//载入子目录项的i节点
	if (!inode.canDelete()) {
		cout << "permission denied." << endl;
		return false;
	}
	return Dentry::remove(c_dentry, file);
}

/**
 * @brief 输出当前目录下的目录项
 * @param file 文件流指针
 * @param dentry 目录项
 * @return 是否输出成功
 */
bool listDentryChildren(fstream *file, Dentry *dentry)
{
	static char str_time[100];
	struct tm *show_time = NULL;//用于时间格式的转换
	dentry->loadChildren(file);
	Inode inode;
	map<string, Dentry*>::iterator it;
	for(it = dentry->children.begin(); it != dentry->children.end(); it ++) {
		inode.number = it->second->inode_number;
		inode.load(file);
		cout << it->second->name <<'\t'<<it->second->inode_number<<'\t'<<inode.size<<'\t'
		     << inode.getCharFileType()<<inode.getStringAuthority()<<'\t'
		     << super_block.getUsernameByUid(inode.user_id) <<'\t'
		     << super_block.getGroupNameByGid(inode.group_id)<<'\t';
		show_time = localtime(&inode.modify_time);//显示文件的基本信息
		strftime(str_time, sizeof(str_time), "%Y-%m-%d %H:%M:%S", show_time);
		cout << str_time <<'\t';
		show_time = localtime(&inode.create_time);
		strftime(str_time, sizeof(str_time), "%Y-%m-%d %H:%M:%S", show_time);
		cout << str_time <<'\t'<<endl;
	}
	return true;
}

/**
 * @brief 打印当前超级块内的情况
 */
void showSysState()
{
	unsigned int i;
	cout << "Amount of all inodes:" << super_block.inode_amount << endl;//i节点数量
	cout << "Amount of free inodes :" << super_block.free_inodes_amount << endl;//空闲i节点数量
	cout << "Amount of all blocks:" << super_block.block_amount << endl;//盘块数量
	cout << "Amount of free blocks :" << super_block.free_blocks_amount << endl;//空闲盘块数量
	cout << "free inode stack :" << endl;
	for(i = 0; i < FREE_INODE_STACK_SIZE; i++) {//查看空闲盘块栈信息
		cout << " ";
		if(super_block.free_inodes_stack[i] == INODE_AMOUNT) {
			cout << "N/A";
		} else {
			cout << super_block.free_inodes_stack[i];
		}
	}
	cout << endl << "curr: " << super_block.free_inodes_stack[super_block.curr_inode_stack] << endl;
	cout << "free block stack :" << endl;
	cout << " " << super_block.free_blocks_stack_bottom;
	for(i = 1; i < FREE_INODE_STACK_SIZE; i++) {//查看空闲i节点栈信息
		cout << " ";
		if(super_block.free_blocks_stack[i] == 0) {
			cout << "N/A";
		} else {
			cout << super_block.free_blocks_stack[i];
		}
	}
	cout << endl << "curr: " << (super_block.curr_block_stack == 0 ?
	                             super_block.free_blocks_stack_bottom :
	                             super_block.free_blocks_stack[super_block.curr_block_stack]) << endl;

	cout << "the first of next series: " << super_block.free_blocks_stack[0] << endl;
}

/**
 * @brief 字符串拆分
 * @param str 欲拆分的字符串
 * @param spl 拆分标记字符集
 * @return 拆分结果构成的向量
 */
vector<string> split(string str, const char* spl)
{
	char* name = new char[str.size()];
	char* p;
	strcpy(name, str.c_str());
	p = strtok(name, spl);
	vector<string> tokens;
	while(p) {
		tokens.push_back(p);
		p = strtok(0, spl);
	}
	delete [] name;
	return tokens;
}

/**
 * @brief 改变当前目录
 * @param file 文件流对象指针
 * @param dentry 当前目录
 * @param dir_name 目标目录
 * @return 更改的结果
 */
bool changeDirectory(fstream* file, Dentry* &dentry, const char* dir_name, bool isTip = true)
{
	vector<string> tokens = split(dir_name, "/");//根据斜杠拆分路径
	vector<string>::iterator it = tokens.begin();
	bool flag = false;
	Dentry *temp_dentry = dir_name[0] == '/'? &root_dentry:dentry;//判断路径第一位是否是/，如是则从根目录开始搜寻
	Dentry *c_dentry = nullptr;
	Inode inode;
	while(it != tokens.end()) {//一级一级查找子目录
		if (temp_dentry->children.empty()) {
			temp_dentry->loadChildren(file);
		}
		if ((c_dentry = temp_dentry->getChildByName(file, it->c_str())) == nullptr) {//如果不存在该目录
			if (isTip) {
				cout << "faild! the directory does not exist." <<endl;
			}
			break;
		} else {
			temp_dentry = c_dentry;
			inode.number = temp_dentry->inode_number;
			inode.load(file);
			if (inode.file_type != FILE_TYPE_DIRECTORY) {//如果该文件不是目录文件
				if (isTip) {
					cout << "faild! it is nor a directory." <<endl;
				}
				break;
			}
		}
		it ++;
	}
	flag = it==tokens.end();
	if (flag) {
		dentry = temp_dentry;//更改当前工作目录
	}
	return flag;
}

/**
 * @brief 获取当前工作目录的全名
 * @param file 文件流对象指针
 * @param dentry 当前工作目录的指针
 * @return 目录全名字符串
 */
string getWorkingDirecory(fstream* file, Dentry* dentry)
{
	string result = "";
	if (dentry->children.empty()) {//检查子目录项是否已经加载
		dentry->loadChildren(file);
	}
	Dentry *p_dentry = dentry->getChildByName(file, "..");
	if (p_dentry->inode_number == dentry->inode_number) {//如果父目录和本目录相同，说明是根目录
		return "/";
	}
	if (p_dentry->children.empty()) {//检查父目录的子目录项是否已经加载
		p_dentry->loadChildren(file);
	}
	map<string, Dentry*>::iterator it = p_dentry->children.begin();
	while(it != p_dentry->children.end()) {//遍历找出父目录中子目录项和自己相同的目录项
		if (it->second->inode_number == dentry->inode_number) {
			result = it->first + "/";//将目录名称作为结果保存
			break;
		}
		it ++;
	}
	return getWorkingDirecory(file, p_dentry) + result;//递归调用本身
}



/**
 * @brief 编辑文件
 * @param file 文件流对象指针
 * @param dentry 当前工作目录的指针
 * @param filename 目标文件名称
 */
void edit(fstream *file, Dentry* dentry, const char* filename)
{
	vector<string> tokens = split(filename, "/");//拆分路径
	vector<string>::iterator it = tokens.end();
	it --;
	string last = *it;
	tokens.pop_back();
	string dir_name = filename[0] == '/' ? "/" : "./";//如果以斜杠开头，从根目录开始搜寻
	it = tokens.begin();
	while (it != tokens.end()) {
		dir_name += *it;
		dir_name += "/";
		it ++;
	}
	vector<string> empty_vec;
	empty_vec.swap(tokens);
	Dentry* temp_dentry = dentry, *target_dentry;
	Inode inode;
	if (!changeDirectory(file, temp_dentry, dir_name.c_str())) {//cd是否成功
		return;
	}
	if ((target_dentry = temp_dentry->getChildByName(file, last.c_str())) == nullptr) {//是否存在该文件
		inode.number = temp_dentry->inode_number;
		inode.load(file);
		if (!inode.canWrite()) {//查询在该目录下是否拥有写权限
			cout << "permission denied." << endl;
			return;
		}
		target_dentry = Dentry::createEmptyFile(file, temp_dentry, last.c_str());//新建空文件
		if (target_dentry == nullptr) {
			return;
		}
	}
	inode.number = target_dentry->inode_number;
	inode.load(file);
	if (inode.file_type != FILE_TYPE_NORMAL) {//如果不是文本文件
		cout << "failed! this type of file cannot be opened." <<endl;
		return;
	}
	if (!inode.canWrite()) {//查看是否拥有该文件的写权限
		cout << "permission denied." << endl;
		return;
	}
	cout << target_dentry->name << ':' <<endl;
	cout << inode.getContent(file) << endl << endl;
	cout << "input new content here:"<<endl;
	string content;
	cin >> content;//输入新内容
	if (target_dentry->saveFileContent(file, content.c_str(), content.size())) {//保存文件内容
		cout << "save successfully!" <<endl;
		return;
	} else {
		cout << "failed to save!" <<endl;
		return;
	}
}

/**
 * @brief 连接显示文件内容
 * @param file 文件流对象指针
 * @param dentry 当前工作目录的指针
 * @param filenames 要显示的文件的文件名
 * @return 显示是否成功
 */
bool concatenate(fstream *file, Dentry* dentry, vector<string> filenames)
{
	Dentry* temp_dentry, *target_dentry;
	vector<string>::iterator it = filenames.begin();
	string result = "";
	while(it != filenames.end()) {
		temp_dentry = dentry;
		vector<string> tokens = split(*it, "/");//拆分路径
		vector<string>::iterator c_it = tokens.end();
		c_it --;
		string last = *c_it;
		tokens.pop_back();
		string dir_name = (*it)[0] == '/' ? "/" : "";
		c_it = tokens.begin();
		while (c_it != tokens.end()) {
			dir_name += *c_it;
			dir_name += "/";
			c_it ++;
		}
		Inode inode;
		if (!changeDirectory(file, temp_dentry, dir_name.c_str())) {//cd是否成功
			return false;
		}
		if ((target_dentry = temp_dentry->getChildByName(file, last.c_str())) == nullptr) {
			cout << "failed! file '"<<last<<"' not found!" <<endl;
			return false;
		}
		inode.number = target_dentry->inode_number;
		inode.load(file);
		if (inode.file_type != FILE_TYPE_NORMAL) {//如果不是文本文件
			cout << "failed! this type of file cannot be opened." <<endl;
			return false;
		}
		if (!inode.canRead()) {//查看读权限
			cout << "permission denied." << endl;
			return false;
		}
		result += inode.getContent(file);
		it ++;
		vector<string> empty_vec;
		empty_vec.swap(tokens);
	}
	cout << result << endl;
	return true;
}
/**
 * @brief 移动或者重命名文件
 * @param file 文件流对象指针
 * @param dentry 当前工作目录的指针
 * @param name1 源文件名
 * @param name2 新文件名或者新父目录
 * @return 移动或者重命名的结果
 */
bool move(fstream *file, Dentry* dentry, string name1, string name2)
{
	Dentry *o_dentry = Dentry::getDentryByPathName(file, dentry, name1);
	if (o_dentry == nullptr) {//检查源文件是否存在
		cout << "failed! can not find file or directory named '"<<name1<<"'."<<endl;
		return false;
	}
	Inode o_inode;
	o_inode.number = o_dentry->inode_number;
	o_inode.load(file);
	if (!o_inode.canWrite()) {//检查源文件的写权限
		cout << "permission denied. no authority to move or rename this file." << endl;
		return false;
	}
	Dentry *o_p_dentry = o_dentry->parent;
	Dentry *n_dentry = Dentry::getDentryByPathName(file, dentry, name2);
	if (n_dentry != nullptr) {//如果目标文件存在
		Inode inode;
		inode.number = n_dentry->inode_number;
		inode.load(file);
		if (inode.file_type == FILE_TYPE_NORMAL) {//如果目标文件是个普通文件
			if (o_dentry->inode_number == n_dentry->inode_number) {//如果目标文件和源文件是同一个
				cout << "cancel! the new name must be different." <<endl;
			} else {
				cout << "failed! there is a file with the same name."<<endl;//如果存在一个同名文件
			}
			return false;
		}
		if (inode.file_type == FILE_TYPE_DIRECTORY) {//如果目标文件是个目录
			if (o_dentry->parent->inode_number == n_dentry->inode_number) {
				cout << "cancel! you must move the file to a new directory."<<endl;
				return false;
			}
			if (!inode.canWrite()) {//检查该目录的写权限
				cout << "permission denied. can not move to here." << endl;
				return false;
			}
			n_dentry->loadChildren(file);
			if (n_dentry->hasSameName(o_dentry->name)) {//检查是否存在同名文件
				cout << "failed! there is a file with the same name."<<endl;
				return false;
			}
			if (o_inode.file_type == FILE_TYPE_DIRECTORY) {//如果源文件是个目录
				string pwd1 = getWorkingDirecory(file, o_dentry);
				string pwd2 = getWorkingDirecory(file, n_dentry);
				unsigned int i;
				for (i = 0; i<pwd1.size() && i<pwd2.size(); i++) {//检查是否是把父目录移动到子目录的情况
					if (pwd1[i] != pwd2[i]) {
						break;
					}
				}
				if (i == pwd1.size()) {
					cout << "failed! can not move to sub directory."<<endl;
					return false;
				}
			}
			o_dentry->parent = n_dentry;
			o_p_dentry->loadChildren(file);
			n_dentry->children.insert(pair<string, Dentry*>(o_dentry->name, o_dentry));
			o_p_dentry->children.erase(o_dentry->name);
			o_p_dentry->saveDir(file);
			n_dentry->saveDir(file);//移动
			return true;
		} else {
			cout << "failed! you can not move to here."<<endl;
			return false;
		}
	} else {//如果目标文件不存在
		unsigned int i;
		for (i = 0; i<name2.size(); i++) {
			if (name2[i] == '/') {
				break;
			}
		}
		if (i == name2.size()) {
			if (checkName(name2.c_str())) {//检查文件名是否规范
				o_p_dentry->children.erase(o_dentry->name);
				o_dentry->setName(name2.c_str(), name2.size());
				o_p_dentry->children.insert(pair<string, Dentry*>(name2, o_dentry));
				o_p_dentry->saveDir(file);
				return true;
			} else {
				return false;
			}
		}
		vector<string> tokens = split(name2, "/");
		vector<string>::iterator it = tokens.end();
		it --;
		string last = *it;
		tokens.erase(it);
		string temp_dir_name = name2[0]=='/'? "/":"./";
		it = tokens.begin();
		while(it != tokens.end()) {
			temp_dir_name += *it + "/";
			it ++;
		}
		Dentry *n_p_dentry = current_dentry;
		if (!changeDirectory(file, n_p_dentry, temp_dir_name.c_str(), false) || n_p_dentry == nullptr) {//检查目标文件是否存在父目录
			cout << "faild! the directory '"<<temp_dir_name<<"' does not exists."<<endl;
			return false;
		}
		o_dentry->parent = n_p_dentry;
		o_p_dentry->children.erase(o_dentry->name);
		o_dentry->setName(last.c_str(), last.size());
		n_p_dentry->children.insert(pair<string, Dentry*>(o_dentry->name, o_dentry));
		o_p_dentry->saveDir(file);
		n_p_dentry->saveDir(file);
		return true;
	}
}
/**
 * @brief 复制文件
 * @param file 文件流对象指针
 * @param dentry 当前工作目录的指针
 * @param name1 源文件名
 * @param name2 新文件名或者目录
 * @return 复制的结果
 */
bool copy(fstream *file, Dentry* dentry, string name1, string name2)
{
	Dentry *o_dentry = Dentry::getDentryByPathName(file, dentry, name1);
	if (o_dentry == nullptr) {//检查源文件是否存在
		cout << "failed! can not find file or directory named '"<<name1<<"'."<<endl;
		return false;
	}
	Dentry *n_dentry = Dentry::getDentryByPathName(file, dentry, name2);
	if (n_dentry != nullptr) {//如果目标文件不存在
		Inode inode;
		inode.number = n_dentry->inode_number;
		inode.load(file);
		if (inode.file_type == FILE_TYPE_NORMAL) {//如果是个文件
			if (o_dentry->inode_number == n_dentry->inode_number) {
				cout << "cancel! the new file name must be different." <<endl;
			} else {
				cout << "failed! there is a file with the same name."<<endl;
			}
			return false;
		}
		if (inode.file_type == FILE_TYPE_DIRECTORY) {//如果是个目录
			if (o_dentry->parent->inode_number == n_dentry->inode_number) {
				cout << "cancel! you must copy the file to a new directory."<<endl;
				return false;
			}
			if (!inode.canWrite()) {
				cout << "permission denied. can not move to here." << endl;
				return false;
			}
			n_dentry->loadChildren(file);
			if (n_dentry->hasSameName(o_dentry->name)) {
				cout << "failed! there is a file with the same name."<<endl;
				return false;
			}
			Inode o_inode;
			o_inode.number = o_dentry->inode_number;
			o_inode.load(file);
			if (o_inode.file_type == FILE_TYPE_DIRECTORY) {//检查是否是父目录复制到子目录的情况
				string pwd1 = getWorkingDirecory(file, o_dentry);
				string pwd2 = getWorkingDirecory(file, n_dentry);
				unsigned int i;
				for (i = 0; i<pwd1.size() && i<pwd2.size(); i++) {
					if (pwd1[i] != pwd2[i]) {
						break;
					}
				}
				if (i == pwd1.size()) {
					cout << "failed! can not copy to sub directory."<<endl;
					return false;
				}
			}
			//-----------------------------
			return Dentry::copy(file, o_dentry, o_dentry, n_dentry);//复制到目标目录
			//-----------------------------
		} else {
			cout << "failed! you can not move to here."<<endl;
			return false;
		}
	} else {//如果目标文件不存在
		unsigned int i;
		for (i = 0; i<name2.size(); i++) {
			if (name2[i] == '/') {
				break;
			}
		}
		if (i == name2.size()) {
			if (checkName(name2.c_str())) {//检查文件名规范
				//--------------
				Inode o_p_inode;
				o_p_inode.number = o_dentry->parent->inode_number;
				o_p_inode.load(file);
				if (!o_p_inode.canWrite()) {//检查源文件父目录是否可写
					cout << "permission denied. can not copy to here." << endl;
					return false;
				}
				Dentry tar_dentry;
				tar_dentry.setName(name2.c_str(), name2.size());
				return Dentry::copy(file, o_dentry, &tar_dentry, current_dentry);
				//--------------
			} else {
				return false;
			}
		}
		vector<string> tokens = split(name2, "/");
		vector<string>::iterator it = tokens.end();
		it --;
		string last = *it;
		tokens.erase(it);
		string temp_dir_name = name2[0]=='/'? "/":"./";//判断是否从根目录开始
		it = tokens.begin();
		while(it != tokens.end()) {
			temp_dir_name += *it + "/";
			it ++;
		}
		Dentry *n_p_dentry = current_dentry;
		if (!changeDirectory(file, n_p_dentry, temp_dir_name.c_str(), false) || n_p_dentry == nullptr) {//目标文件的父目录不存在
			cout << "faild! the directory '"<<temp_dir_name<<"' does not exists."<<endl;
			return false;
		}
		if (n_p_dentry->inode_number != o_dentry->parent->inode_number) {//如果不是复制到本目录，且目标目录不存在
			cout << "faild! the target directory does not exist."<<endl;
			return false;
		}
		Inode o_p_inode;
		o_p_inode.number = o_dentry->parent->inode_number;
		o_p_inode.load(file);
		if (!o_p_inode.canWrite()) {//检查源文件父目录是否可写
			cout << "permission denied. can not copy to here." << endl;
			return false;
		}
		if (!checkName(last.c_str())) {//检查文件名
			return false;
		}
		Dentry tar_dentry;
		tar_dentry.setName(last.c_str(), last.size());
		return Dentry::copy(file, o_dentry, &tar_dentry, n_p_dentry);
	}
}
/**
 * @brief 改变文件的权限
 * @param file 文件流对象指针
 * @param dentry 当前目录项
 * @param auth 新的权限对应的指针
 * @param filename 欲修改权限的文件文件名称
 */
void changeMod(fstream *file, Dentry *dentry, string auth, string filename)
{
	if (auth.size() != 9) {//检查长度是否规范
		cout << "failed! wrong authority format." <<endl;
		return;
	}
	unsigned int i;
	short a = 0b000000000;
	short st = 0b100000000;
	for (i=0; i<9; i++) {//将字符串翻译成二进制
		if (auth[i]=='1') {
			a = a^(st>>i);
			continue;
		}
		if (auth[i]=='0') {
			continue;
		} else {
			break;
		}
	}
	if (i < 9) {
		cout << "failed! wrong authority format." <<endl;
		return;
	}
	Dentry *c_dentry = Dentry::getDentryByPathName(file, dentry, filename);
	if (c_dentry == nullptr) {//检查文件是否存在
		cout << "failed! file not find." <<endl;
		return;
	}
	Inode inode;
	inode.number = c_dentry->inode_number;
	inode.load(file);
	if (!inode.canChmod()) {//检查权限
		cout << "permission denied." <<endl;
		return;
	}
	inode.authority = a;
	inode.save(file);
}
/**
 * @brief 改变文件掩码
 * @param auth 新掩码对应的字符串
 */
void changeUmask(string auth)
{
	if (auth.size() != 9) {
		cout << "failed! wrong authority format." <<endl;
		return;
	}
	unsigned int i;
	short a = 0b000000000;
	short st = 0b100000000;
	for (i=0; i<9; i++) {
		if (auth[i]=='0') {
			a = a^(st>>i);
			continue;
		}
		if (auth[i]=='1') {
			continue;
		} else {
			break;
		}
	}
	if (i < 9) {
		cout << "failed! wrong authority format." <<endl;
		return;
	}
	super_block.umask = a;
}
/**
 * @brief 改变文件的拥有者
 * @param file 文件流对象指针
 * @param dentry 当前目录项
 * @param username 用户名
 * @param filename 欲修改拥有者的文件文件名称
 */
void changeUser(fstream *file, Dentry *dentry, string username, string filename)
{

	map<string, User>::iterator it = super_block.users.find(username);
	if (it == super_block.users.end()) {
		cout << "failed! user not found." <<endl;
		return;
	}
	Dentry *c_dentry = Dentry::getDentryByPathName(file, dentry, filename);
	if (c_dentry == nullptr) {
		cout << "failed! file not found." << endl;
		return;
	}
	Inode inode;
	inode.number = c_dentry->inode_number;
	inode.load(file);
	if (!inode.canChangeUidOrGid()) {
		cout << "permission denied." << endl;
		return;
	}
	inode.user_id = it->second.uid;
	inode.modify_time = time(0);
	inode.save(file);
}
/**
 * @brief 改变文件的组
 * @param file 文件流对象指针
 * @param dentry 当前目录项
 * @param groupname 组名称
 * @param filename 欲修改组的文件文件名称
 */
void changeGroup(fstream *file, Dentry *dentry, string groupname, string filename)
{

	map<string, Group>::iterator it = super_block.groups.find(groupname);
	if (it == super_block.groups.end()) {//查看组是否存在
		cout << "failed! group not found." <<endl;
		return;
	}
	Dentry *c_dentry = Dentry::getDentryByPathName(file, dentry, filename);
	if (c_dentry == nullptr) {//查看文件是否存在
		cout << "failed! file not found." << endl;
		return;
	}
	Inode inode;
	inode.number = c_dentry->inode_number;
	inode.load(file);
	if (!inode.canChangeUidOrGid()) {//判断权限
		cout << "permission denied." << endl;
		return;
	}
	inode.group_id = it->second.gid;
	inode.modify_time = time(0);
	inode.save(file);
}

/**
 * @brief 更改用户密码
 * @param file 文件流对象指针
 * @param password 旧密码
 */
void changePassword(fstream* file, string password)
{
	if (password != super_block.current_user->password) { //判断密码是否正确
		cout << "your password is wrong." <<endl;
		return;
	}
	string n_password, repassword;
	cout << "[ new password ] : ";
	cin.sync();
	cin>>n_password;
	cout << "[ comfirm password ] : ";
	cin.sync();
	cin>>repassword;
	if (n_password != repassword) {//判断两次输入密码是否一致
		cout << "failed! the password and re-password are not equal."<<endl;
		return;
	}
	if (n_password.size()<4 || password.size()>12) {//判断两次密码输入是否一致
		cout << "failed! the length of the password must be less than 12 and more than 4."<<endl;
		return;
	}
	super_block.current_user->setPassword(n_password.c_str(), n_password.size());//保存新密码
	super_block.saveUsers(file);//保存用户信息
}
/**
 * @brief 显示帮助
 * @param cmd 命令
 */
void showHelp(string cmd)
{
	map<string, string>::iterator it;
	if (cmd == "all") { //如果cmd为“all”，那么显示所有的帮助信息
		it = help_map.begin();
		while(it != help_map.end()) {
			cout << it->second <<endl<<endl;
			it ++;
		}
		return;
	}
	it = help_map.find(cmd);
	if (it == help_map.end()) {
		cout << "can not found the command '"<<cmd<<"'."<<endl;
		return;
	}
	cout << it->second <<endl;//显示某一条指令的帮助信息
}


/**
 * @brief 命令读取
 */
void commandListener(const char* filename)
{
	super_block.umask = 0b111111111;
	string cmd;
	stringstream ss;
	vector<string> tokens;
	string buf;
	fstream file(filename, ios::in | ios::out | ios::binary);
	string path = "/";
	path += USER_DIRECTORY;
	path += "/";
	path += super_block.current_user->name;
	path += "/";
	path += HOME;
	changeDirectory(&file, current_dentry, path.c_str());
	bool flag;
	while(true) {
		flag = true;
		ss.str("");
		ss.clear();
		cout << super_block.current_user->name <<"@localhost "<< getWorkingDirecory(&file, current_dentry)<<" $:";
		cin.sync();
		getline(cin, cmd);
		ss << cmd;
		while(ss >> buf) {
			tokens.push_back(buf);
		}
		if(cmd == "exit") {
			break;
		}
		if (tokens.size() == 1) {
			if(tokens.at(0) == "ls") {//查看目录
				listDentryChildren(&file, current_dentry);
			} else if(tokens.at(0) == "state") {//查看空闲i节点栈、空闲盘块栈情况
				showSysState();
			} else if(tokens.at(0) == "pwd") {//查看当前路径
				cout << getWorkingDirecory(&file, current_dentry) << endl;
			} else if(tokens.at(0) == "clear") {//清屏
				system("cls");
			} else if(tokens.at(0) == "help") {//清屏
				showHelp("all");
			} else {
				flag = false;
			}
		} else if (tokens.size() == 2) {
			if(tokens.at(0) == "mkdir") {//创建目录
				const char* name = tokens.at(1).c_str();
				makeDirectory(&file, current_dentry, name);
			} else if(tokens.at(0) == "rmdir") {//删除目录
				const char* name = tokens.at(1).c_str();
				removeDirectory(&file, current_dentry, name);
			} else if(tokens.at(0) == "rm") {
				const char* name = tokens.at(1).c_str();
				remove(&file, current_dentry, name);
			} else if(tokens.at(0) == "cd") {//更改当前目录
				changeDirectory(&file, current_dentry, tokens.at(1).c_str());
			} else if(tokens.at(0) == "vi") {
				edit(&file, current_dentry, tokens.at(1).c_str());
			} else if(tokens.at(0) == "cat") {
				tokens.erase(tokens.begin());
				concatenate(&file, current_dentry, tokens);
			} else if (tokens.at(0) == "umask") {
				changeUmask(tokens.at(1));
			} else if (tokens.at(0) == "passwd") {
				changePassword(&file, tokens.at(1));
			} else if (tokens.at(0) == "man") {
				showHelp(tokens.at(1));
			} else {
				flag = false;
			}
		} else if (tokens.size()>2) {
			if (tokens.at(0) == "cat") {
				tokens.erase(tokens.begin());
				concatenate(&file, current_dentry, tokens);
			} else {
				flag = false;
			}
		}
		if(tokens.size() == 3) {
			if (tokens.at(0) == "mv") {
				move(&file, current_dentry, tokens.at(1), tokens.at(2));
				flag = true;
			} else if(tokens.at(0) == "cp") {
				copy(&file, current_dentry, tokens.at(1), tokens.at(2));
				flag = true;
			} else if(tokens.at(0) == "chmod") {
				changeMod(&file, current_dentry, tokens.at(1), tokens.at(2));
				flag = true;
			} else if(tokens.at(0) == "chown") {
				changeUser(&file, current_dentry, tokens.at(1), tokens.at(2));
				flag = true;
			} else if(tokens.at(0) == "chgrp") {
				changeGroup(&file, current_dentry, tokens.at(1), tokens.at(2));
				flag = true;
			} else {
				flag = false;
			}
		}
		if(!flag) {
			cout << "no such command." <<endl;
		}
		vector<string> empty_vec;
		empty_vec.swap(tokens);
	}
	file.close();
}
