/*
 * @author Rick
 * @date 13/01/2016
 * @file init.cpp
 * @brief 和文件系统初始化相关的函数
 */
#include <fstream>
#include <iostream>
#include <unistd.h>
#include "SuperBlock.h"
#include <map>
using namespace std;

SuperBlock super_block;//超级块对象
Dentry root_dentry;//根目录对象
Inode root_inode;//根目录的i节点
Dentry *current_dentry = &root_dentry;
map<string, string> help_map;

unsigned int allocateFreeInode(fstream *file);
unsigned int allocateFreeBlock(fstream *file);
bool makeDirectory(fstream *file, Dentry* dentry, const char* dir_name);
bool changeDirectory(fstream* file, Dentry* &dentry, const char* dir_name, bool isTip = true);
/**
 * @brief 初始化数据盘块区
 * @param file 文件流对象指针
 */
void resetBlockData(fstream* file)
{
	char *data;
	int i;
	int number = INODE_AMOUNT / (super_block.per_block_size / Inode::save_size) + 1;
	int stack_size, next_block_number;
	for(; number < (int)super_block.block_amount; number += FREE_BLOCK_STACK_SIZE) {
		if(super_block.block_amount - number > FREE_BLOCK_STACK_SIZE) {//确定空闲盘块栈区大小以及下一空闲盘块组的第一个盘块号
			stack_size = FREE_BLOCK_STACK_SIZE;
			next_block_number = number + FREE_BLOCK_STACK_SIZE;
		} else {
			stack_size = super_block.block_amount - number;
			next_block_number = BLOCK_NUMBER_OF_SUPER_BLOCK;
		}
		/*保存空闲盘块组头一个盘块*/
		file->seekp(number * super_block.per_block_size, ios::beg);
		file->write(data, convertToBinary(data, stack_size));//保存空闲栈大小
		releaseBinaryData(data);
		file->write(data, convertToBinary(data, next_block_number));//保存下一盘块组头一个盘块的盘块号
		releaseBinaryData(data);
		for(i = 1; i < stack_size; i++) {
			file->write(data, convertToBinary(data, number + i));//写入初始每一个盘块栈内空闲盘块号
			releaseBinaryData(data);
		}
	}
}


/**
 * @brief 创建一个指定大小的空文件
 * @param filename 文件名称
 * @param size 指定的文件大小，单位：字节
 */
int createEmptyFile(const char* filename, long size)
{
	int ret = -1;
	FILE *fp = NULL;
	fp = fopen(filename, "a+b");
	ret = truncate(filename, size);
	fclose(fp);
	return ret;
}

/**
 * @brief 重置所有的i节点数据
 * @param file 文件流对象指针
 */
void resetInodeData(fstream* file)
{
	file->seekp(super_block.per_block_size, ios::beg);
	int i;
	char data[Inode::save_size];
	for(i = 0; i < INODE_AMOUNT; i++) {
		Inode::getSingleResetData(data, i);
		file->write(data, Inode::save_size);
	}
}

void createRootDirectory(fstream *file)
{
	Inode inode;
	inode.number = allocateFreeInode(file);
	unsigned int block_number = allocateFreeBlock(file);
	inode.file_type = FILE_TYPE_DIRECTORY;
	inode.authority = (short)0b111100100;
	inode.size = 0;
	inode.address[0] = block_number;
	inode.user_id = 0;
	inode.group_id = 0;
	inode.modify_time = time(0);
	inode.create_time = time(0);
	inode.link_count = 1;
	root_dentry.inode_number = inode.number;
	super_block.root_inode_number = inode.number;
	inode.save(file);
	root_dentry.saveDir(file);
	super_block.save(file);
}

/**
 * @brief 重置数据
 * @param filename 保存数据的文件名称
 * @param size 数据的大小
 * @return 重置的结果
 */
bool resetData(const char* filename, long size)
{
	createEmptyFile(filename, size);//重新创建一个文件
	/*初始化super_blcok区域数据*/
	fstream file(filename, ios::in | ios::out | ios::binary);
	if(!file) {
		cout << "reset error!" << endl;
		return false;
	} else {
		file.seekp(BLOCK_NUMBER_OF_SUPER_BLOCK * super_block.per_block_size);
		file.write(super_block.getResetData(), SuperBlock::save_size);
		resetBlockData(&file);
		resetInodeData(&file);
		super_block.modify_time = time(0);
		super_block.loadFreeBlockStack(&file, INODE_AMOUNT / (super_block.per_block_size / Inode::save_size) + 1);
		super_block.reloadFreeInodeStack(&file);
		createRootDirectory(&file);

		User root_user;
		root_user.setName("root", 4);
		root_user.setPassword("123456", 6);
		root_user.uid = 0;
		root_user.gid = 0;

		Group group;
		group.gid = 0;
		group.setName("root_g", 6);

		Group group_n;
		group_n.gid = 1;
		group_n.setName("normal", 6);

		super_block.current_user = &root_user;

		Dentry* temp_dentry = &root_dentry;
        super_block.umask = 0b111100110;
		makeDirectory(&file, temp_dentry, DATA_PATH);
		makeDirectory(&file, temp_dentry, USER_DIRECTORY);
		changeDirectory(&file, temp_dentry, DATA_PATH);
		super_block.umask = 0b000000000;
		Dentry::createEmptyFile(&file, temp_dentry, GROUP_DATA_FILENAME);
		Dentry::createEmptyFile(&file, temp_dentry, USER_DATA_FILENAME);

		super_block.users.insert(pair<string, User>(root_user.name, root_user));
		super_block.groups.insert(pair<string, Group>(group.name, group));
		super_block.groups.insert(pair<string, Group>(group_n.name, group_n));

		super_block.saveUsers(&file);
		super_block.saveGroups(&file);

		string path = "/";
		path += USER_DIRECTORY;
		changeDirectory(&file, temp_dentry, path.c_str());
		super_block.umask = 0b111000000;
		makeDirectory(&file, temp_dentry, "root");
		path += "/root";
		changeDirectory(&file, temp_dentry, path.c_str());
		makeDirectory(&file, temp_dentry, HOME);
		file.close();
		return true;
	}
}

/**
 * @brief 系统加载函数
 * @param filename 模拟磁盘的文件名称
 */
void initFileSystem(const char* filename)
{
	fstream file(filename, ios::in | ios::out | ios::binary);
	if(!file) {
		cout << "it's the first time to execute the system, and resetting data now..." << endl;
		resetData(filename, BLOCK_AMOUNT * BLOCK_SIZE);
	}
	super_block.load(&file);
	root_dentry.inode_number = super_block.root_inode_number;
	root_inode.number = super_block.root_inode_number;
	root_inode.load(&file);
	char root_dentry_name[4] = {'r', 'o', 'o', 't'};
	root_dentry.setName(root_dentry_name, 4);
	root_dentry.loadChildren(&file);
	super_block.readUsers(&file);
	super_block.readGroups(&file);
	file.close();
}

void pressKeyToContinue()
{
	cout << "prerss any key to continue...";
	cin.sync();
	getchar();
}

bool login()
{
	string access, password;
	system("cls");
	cout << "********login********" <<endl;
	cout << "access: ";
	cin.sync();
	cin>>access;
	cout << "password: ";
	cin.sync();
	cin>>password;
	map<string, User>::iterator it = super_block.users.find(access);
	if (it == super_block.users.end()) {
		cout << "there is wrong in your access or password"<<endl;
		pressKeyToContinue();
		return false;
	}
	if (password != it->second.password) {
		cout << "there is wrong in your access or password"<<endl;
		pressKeyToContinue();
		return false;
	}
	system("cls");
	super_block.current_user = &it->second;
	cout << "login success!"<<endl;
	pressKeyToContinue();
	system("cls");
	return true;
}

bool reg()
{
    fstream file(SIMULATED_DISK, ios::in | ios::out | ios::binary);
	string access, password, repassword;
	system("cls");
	cout << "********register********" <<endl;
	cout << "access: ";
	cin.sync();
	cin>>access;
	cout << "password: ";
	cin.sync();
	cin>>password;
	cout << "re-password: ";
	cin.sync();
	cin>>repassword;
	if (access.size()<4 || access.size()>12) {
		cout << "failed! the length of the access must be less than 12 and more than 4."<<endl;
		file.close();
		pressKeyToContinue();
		return false;
	}
	unsigned int i;
	for (i = 0; i <access.size(); i++) {
		if(access[i] == '\\' || access[i] == '/' || access[i] == ':' || access[i] == '*' || access[i] == '?' || access[i] == '<' || access[i] == '>' || access[i] == '|' || access[i] == '\"') {
			break;
		}
	}
	if (i != access.size()) {
		cout <<"failed! the access can not contain these characters: \\,/,:,*,?,<,>,|,\""<<endl;
		file.close();
		pressKeyToContinue();
		return false;
	}
	map<string, User>::iterator it = super_block.users.find(access);
	if (it != super_block.users.end()) {
		cout << "failed! there is a user with the same access"<<endl;
		file.close();
		pressKeyToContinue();
		return false;
	}
	if (password != repassword) {
		cout << "failed! the password and re-password are not equal."<<endl;
		file.close();
		pressKeyToContinue();
		return false;
	}
	if (password.size()<4 || password.size()>12) {
		cout << "failed! the length of the password must be less than 12 and more than 4."<<endl;
		file.close();
		pressKeyToContinue();
		return false;
	}
    User *user = new User();
    user->setName(access.c_str(), access.size());
    user->setPassword(password.c_str(), password.size());
    user->uid = super_block.users.size();
    user->gid = super_block.groups.find("normal")->second.gid;
    super_block.current_user = user;
    super_block.umask = 0b111000000;
    Dentry* temp_dentry = current_dentry;
	string path = "/";
	path += USER_DIRECTORY;
	if (!changeDirectory(&file, temp_dentry, path.c_str())){
		file.close();
		pressKeyToContinue();
		return false;
	}
	if (!makeDirectory(&file, temp_dentry, access.c_str())){
		file.close();
		pressKeyToContinue();
		return false;
	}
	path += "/" + access;
	if (!changeDirectory(&file, temp_dentry, path.c_str())){
		file.close();
		pressKeyToContinue();
		return false;
	}
	if (!makeDirectory(&file, temp_dentry, HOME)){
		file.close();
		pressKeyToContinue();
		return false;
	}
	super_block.users.insert(pair<string, User>(access, *user));
	super_block.saveUsers(&file);
	system("cls");
	cout << "register success!"<<endl;
	pressKeyToContinue();
	system("cls");
	file.close();
	return true;
}

int choose()
{
	bool isAccess = false;
	string choose;
	while(!isAccess) {
		system("cls");
		cout << "welcome! please choose the function." <<endl
		     << "1---login"<<endl
		     << "2---register"<<endl
		     << "0---exit" <<endl<<endl
		     << "choose:";
		cin.sync();
		cin>>choose;
		if (choose.size() == 1) {
			switch(choose[0]) {
			case '0':
				return 0;
			case '1':
				isAccess = login();
				break;
			case '2':
				reg();
				break;
			}
		}
	}
	return 1;
}

void init_help(){
	string help;
	help = "state: show system state.";
	help_map.insert(pair<string, string>("state", help));
	help = "pwd: show the path of the current working diretory.";
	help_map.insert(pair<string, string>("pwd", help));
	help = "clear: to clean the screen.";
	help_map.insert(pair<string, string>("clear", help));
	help = "help: show all helps.";
	help_map.insert(pair<string, string>("help", help));
	help = "mkdir: create a new directory in current directory.\n example $: mkdir dir_name";
	help_map.insert(pair<string, string>("mkdir", help));
	help = "rmdir: delete the directory.\n example $: rmdir dir_name";
	help_map.insert(pair<string, string>("rmdir", help));
	help = "rm: delete the directory or file.\n example $: rm filename";
	help_map.insert(pair<string, string>("rm", help));
	help = "cd: change the current working directory.\n example $: cd dir_name";
	help_map.insert(pair<string, string>("cd", help));
	help = "vi: create a file or write a file.\n example $: vi filename";
	help_map.insert(pair<string, string>("vi", help));
	help = "cat: link the conten of files and show them.\n example $: cat filename1 [filename2...]";
	help_map.insert(pair<string, string>("cat", help));
	help = "umask: set the umask code.\n example $: umask 111111111";
	help_map.insert(pair<string, string>("umask", help));
	help = "passwd: change the password.\n example $: passwd your_password";
	help_map.insert(pair<string, string>("passwd", help));
	help = "mv: rename a file or move a file to another directory.\n example $: mv filename new_filename";
	help_map.insert(pair<string, string>("mv", help));
	help = "cp: copy file.\n example $: cp filename1 filename2(or path)";
	help_map.insert(pair<string, string>("cp", help));
	help = "chmod: change a file's authority.\n example $: chmod 111111111 filename";
	help_map.insert(pair<string, string>("chmod", help));
	help = "chown: change a file's owner\n example $: chown username filename";
	help_map.insert(pair<string, string>("chown", help));
	help = "chgrp: change a file's group.\n example $: chgrp groupname filename";
	help_map.insert(pair<string, string>("chgrp", help));
	help = "man: to get the help of a command.\n example $: man command_name";
	help_map.insert(pair<string, string>("man", help));
	help = "ls: show dentries of the current working directory.\n example $: ls";
	help_map.insert(pair<string, string>("ls", help));
}
