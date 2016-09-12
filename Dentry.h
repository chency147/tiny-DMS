#ifndef DENTRY_H
#define DENTRY_H
#include "Inode.h"
#include <map>
#include <string.h>
#include <utility>
using std::map;
using std::string;
using std::pair;
/**
 * @class Dentry
 * @author Rick
 * @date 12/01/2016
 * @file Dentry.h
 * @brief 目录项
 */
class Dentry // (16 bytes)
{
public:
	Dentry();
	~Dentry();
	volatile int count;//引用计数器 0:unuserd, 正数:inuse, 负数:negative
	char name[DENTRY_NAME_LENGTH];//文件名
	unsigned int inode_number;//目录项对应的i节点号
	Dentry* parent = nullptr;//父目录的目录项对象


	map<string, Dentry*> children;//如果是目录，那么表示为该目录下的目录文件，否则无意义
	short modify_flag = MODIFIED;//修改标志
	const static unsigned int offset_name = 0;//文件名称
	const static unsigned int offset_inode_number = sizeof(name);//对应的i节点

	const static unsigned int save_size = offset_inode_number + sizeof(inode_number);//目录项在磁盘中保存的大小

	void setName(const char* p_name, int lenght);//设置文件名

	bool loadChildren(fstream* file);//读入目录项

	bool clearChildren();//清空子目录项目列表

	bool hasSameName(const char* newName);//检索在当前子目录项，是否已有名为newName这个子目录项

	unsigned int getDirSavaData(char* &data);//获取目录数据

	bool saveDir(fstream* file);//保存目录数据

	Dentry* getChildByName(fstream *file, const char* c_name);//通过名称获得子目录项

	static bool remove(Dentry *dentry, fstream *file);//删除目录项（递归）

	static Dentry* createEmptyFile(fstream *file, Dentry* parent_dentry, const char* name);//创建一个空文件

    bool saveFileContent(fstream *file, const char* content, unsigned int size);//保存文件内容

    static Dentry* getDentryByPathName(fstream *file, Dentry* p_dentry, string name);//通过路径获得目录项

    static bool copy(fstream* file, Dentry* o_dentry, Dentry* tar_dentry, Dentry* p_tar_dentry);//复制
};

#endif // DENTRY_H
