/*
 *
 *
 */
#ifndef MY_FS
#define MY_FS

#include <list>
#include <time.h>
#include <fstream>
#include <cstddef>
#include <string.h>
using std::fstream;
using std::string;

/*长度大小定义*/
#define BLOCK_AMOUNT 317//盘块数量
#define BLOCK_SIZE 512 //块大小设置为512字节
#define FREE_INODE_STACK_SIZE 20 //空闲i节点栈大小
#define FREE_BLOCK_STACK_SIZE 20 //空闲盘块栈大小
#define INODE_AMOUNT 128//i节点数量
#define INODE_DIRECT_ADDRESS_COUNT 4 //i节点中直接地址数量
#define DENTRY_NAME_LENGTH 12 //目录项文件名长度
#define USER_NAME_LENGTH 12 //用户名最大长度
#define USER_PASSWORD_LENGTH 12//密码最大长度
#define GROUP_NAME_LENGHT 12//组名最大长度
/*文件类型定义*/
#define FILE_TYPE_DIRECTORY (short)0 //目录文件
#define FILE_TYPE_NORMAL (short)1 //普通文件
#define FILE_TYPE_INDEX (short)2 //索引文件
#define FILE_TYPE_VOID (short)3 //未分配文件
#define FILE_TYPE_OTHER (short)4 //其他文件
/*修改标志定义*/
#define MODIFYING (short)0 //正在修改
#define MODIFIED (short)1 //修改完毕
/*特殊盘块号*/
#define BLOCK_NUMBER_OF_SUPER_BLOCK 0 //超级块所在盘块号
#define BLOCK_NUMBER_OF_FIRST_INODE 1 //第一个i节点所在盘块号
/*固定字符串定义*/
#define SIMULATED_DISK "disk.data" //模拟磁盘的文件名
#define DATA_PATH "etc" //数据存储所在文件夹
#define USER_DIRECTORY "usr" //用户文件数据所在文件夹
#define USER_DATA_FILENAME "passwd" //用户账户数据文件
#define GROUP_DATA_FILENAME "gdata" //用户组数据文件
#define HOME "home" //用户home文件夹
using std::list;

/**
 * @brief 释放动态字符数组
 * @param data 字符数组头指针
 */
static void releaseBinaryData(char* data)
{
	delete [] data;
}

/**
 * @brief 将非数组类型的基本格式数据转换为二进制
 * @param b_data 字符串头指针
 * @param o_data 原始数据
 * @return 数据的长度
 */
template<class T>
static unsigned int convertToBinary(char* &b_data, T o_data)
{
	unsigned int i;
	b_data = new char[sizeof(o_data)];
	for(i = 0 ; i < sizeof(o_data); i++) {
		b_data[i] = ((char*)&o_data)[i];
	}
	return sizeof(o_data);
}

/**
 * @brief 将非数组类型的基本格式数据转换为二进制（不进行new操作）
 * @param b_data 字符串头指针
 * @param o_data 原始数据
 * @return 数据的长度
 */
template<class T>
static unsigned int convertToBinaryNotNew(char* b_data, T o_data)
{
	unsigned int i;
	for(i = 0 ; i < sizeof(o_data); i++) {
		b_data[i] = ((char*)&o_data)[i];
	}
	return sizeof(o_data);
}

/**
 * @brief 将数组类型的数据转换为二进制
 * @param b_data 字符串头指针
 * @param o_data 原始数据
 * @param size 数组的项目个数
 * @return 数据长度
 */
template<class T>
static unsigned int convertArrayToBinaryNotNew(char* b_data, T* o_data, unsigned int size)
{
	unsigned int i, j, curr = 0;
	for(i = 0 ; i < size; i++) {
		for(j = 0; j < sizeof(o_data[i]); j++) {
			b_data[curr] = ((char*)&o_data[i])[j];
			curr ++;
		}
	}
	return sizeof(o_data);
}



#endif
