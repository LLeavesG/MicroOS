#pragma once

#include "def.h"

#ifndef FILE_H
#define FILE_H

/*文件系统提供功能：
 *文件系统级操作
 *文件分配表FAT及物理块相关操作
 *系统打开文件表的操作
 *进程打开文件表的操作
 *对目录的操作
 *对目录项的操作
 *对普通文件的操作
 *交互操作
 */

 //文件系统的物理块大小为1KB,磁盘分为引导区(暂定占1块)，保留块（1块），FAT1（占128块）,FAT2(备份，占128块)，根目录区（暂定占2块）,数据区（大小可变）
#define BLOCKSIZE 1024     //磁盘块大小是1KB
#define DISKSIZE 67108864  //磁盘空间是64MB
#define ENDBLOCK 65535     //最后一块磁盘块号
#define FREEBLOCK 0        //磁盘块空闲标志
#define FIRSTROOTBLOCK 258 //根目录起始块号
#define FIRSTDATABLOCK 260 //数据磁盘起始块号
#define FATSIZE 128        // FAT大小
#define FIRSTFATBLOCK 2    // FAT起始块号
#define FIRSTFAT2BLOCK 130 // FAT2起始块号
#define REBLOCK 1          //保留块号

#define DISKNAME "disk"   //磁盘文件名
#define FENDFLAG 1        //将保留块的块号作为结束块标记
#define DIR 0             // fcb属性
#define COMMONFILE 1      // fcb属性
#define FATENTRYNUM 65536 // FAT表项数
#define NRW 0             //不可读写
#define RO 1              //只读
#define RW 2              //可写
#define FCBSIZE 32        // FCB大小

#define MKSOFTENTRY 1 //创建soft表项
#define RMSOFTENTRY 2 //删除soft表项
#define UPDATEENTRY 3 //修改soft表项

#define MKPOFTTABLE 1     //创建poft表
#define RMPOFTTABLE 2     //删除poft表
#define MKPOFTENTRY 1     //创建poft表项
#define RMPOFTENTRY 2     //删除poft表项
#define UPDATEPOFTENTRY 3 //修改soft表项

#define VMSIZE 32
/*文件控制块，每个目录项占32字节(14+1+1+2+2+4+4+4)*/
typedef struct FCB
{
    char fileName[14];         //文件名&扩展名
    char rights;               //文件权限：0不可读写，1只读，2可写
    unsigned char attribute;   //属性，暂定0表示目录，1表示普通文件
    unsigned short startBlock; //文件开始的磁盘块号
    unsigned short length;     //文件的长度，指占多少个磁盘块
    unsigned int size;         //文件大小，单位字节
    unsigned int createTime;   //文件创建时间
    unsigned int updateTime;   //文件最后一次修改时间
} fcb;
//除了根目录，其他目录只允许占1块



//系统打开文件表项
typedef struct softEntry
{
    fcb fileFcb;     //文件fcb
    string path;     //路径名,以防有重名文件
    int pn;          //表明当前使用该文件的进程数
    vector<int> pid; //进程标识符数组，用于表明哪些进程在使用该文件
    int mode;        //文件打开模式，标明各个进程打开文件的方式
} softEntry;

//进程打开文件表项
typedef struct poftEntry
{
    fcb fileFcb; //目录项
    string path; //路径名,以防有重名文件
    // int fd;      //文件描述符，定位系统打开文件表
    int mode; //文件打开模式
} poftEntry;

//单个进程文件打开表
typedef struct poft
{
    int pid;                   //表明进程id
    vector<poftEntry> entries; //进程打开文件表项数组
} poft;

//虚拟内存表项
typedef struct vmEntry
{
    int logicPageNum; // 0表示该块空闲
    int pid;
    string fileName;
} vmEntry;


//磁盘级操作
void init_disk(); //初始化磁盘

//文件系统级操作
void start_fileSystem(); //进入文件系统 启动引导区 默认进入根目录

void exit_fileSystem(); //退出文件系统 保存文件分配表

//文件分配表FAT及物理块相关操作
void init_fat(); //初始化文件分配表，从FAT1起始块读入整个文件分配表

void update_fat(vector<unsigned short> addr, vector<unsigned short> nextBlock); //更新文件分配表

void get_blocks(int blockSize, vector<unsigned short>& getBlocks); //申请物理块，返回值是物理块号的数组,申请成功，需要更新新文件分配表

int get_fats(unsigned short firstBlock, vector<unsigned short>& addr, unsigned short blockSize); //获取文件所有的块号

// int free_blocks(int *addr, int *blockNum); //简化到更新文件分配表

//系统打开文件表的操作
int get_soft_position(string path, string name); //通过文件名+路径来定位在系统打开文件表的位置
int soft_entry_op(softEntry entry, int op);      //表项操作,包括创建，删除，更新，传入需要操作的表项和操作类型
void print_soft(stringstream& output);                               //打印文件打开表内容

//进程打开文件表的操作
int get_poft_position(int pid);                                     //通过pid获得进程打开文件表
int get_poft_entry_location(int pid, string path, string filename); //通过pid路径和文件名定位表项位置
int poft_op(poft table, int op);                                    //对单张文件打开表的操作，分为创建表和删除表
int poft_entry_op(poftEntry entry, int pid, int op);                //对单张表中表项操作，分为创建表项，删除表项，修改表项
void print_pofts(stringstream &output);                                                 //打印文件打开表内容
// int create_poft_entry(fcb dirEntry, int mode, int block, int offset); //创建表项,仅当文件被此进程首次使用时
// int delete_poft_entry(int fd); //删除表项，仅当文件不再被此进程使用时
// int update_poft_entry(char **ways); //修改表项
// int create_poft(int pid, fcb dirEntry, int mode, int block, int offset); //新建表，仅当新的进程打开首个文件
// int delete_poft_entry(int pid); //删除表，仅当进程关闭所有使用的文件
//对表的修改简化为对表项的修改

//对目录的操作
void ls(string dirName, stringstream &output); //显示目录:

int cd(string dirName); //改变当前工作目录,成功返回1，失败返回0，下同

int mkdir(string dirName); //创建目录

int rmdir(string dirName); //删除目录

int load_dirEntry(string dirName); //加载某个目录下的所有目录项

//获取当前文件夹名称
string get_temp_dir_name();

//对目录项的操作
int create_dir_entry(string dirName, fcb dirEntry); //创建目录项

int update_dir_entry(string dirName, fcb dirEntry); //更新某目录项

int delete_dir_entry(string entryName); //删除某一目录项

//对普通文件的操作
int create_file(string fileName); //创建文件
int rm(string fileName);          //删除文件

int open_file(string fileName, int mode, int pid); //打开文件 返回文件描述符fd

int close_file(string fileName, int pid); //关闭文件

int read_file(string fileName, vector<unsigned char*>& mem, int size);

int write_file(string fileName, vector<unsigned char*>& mem, int size);

//交互操作:虚拟内存

int init_vm(); //初始化虚拟内存：将FAT2的物理块分出来作为虚拟内存

int read_file_vm(string fileName, vector<unsigned char*>& mem, int realSize, int logicSize, vector<unsigned char>& addr, int pid); // 文件名，读入地址 ， 真实大小（内存分给了几块），逻辑大小（包含虚拟内存的大小）虚拟内存页号

int vm_swap(unsigned char* mem, unsigned char pageNum); //文件名 需要换入的地址，虚拟页号

int free_vm(int pid); // 当某个进程结束，释放虚拟内存对应的空间

//调试和辅助函数

int exist_file(string fileName);//判断是否存在此文件 绝对路径

void splitStr(const string& str, vector<string>& arr, const string& cut);//拆分路径

void print_fcb(fcb dirEntry, stringstream& output);

int get_dir_entry_location(string name);

void get_path_and_name(string fullName, string& path, string& filename); //获取路径和文件名

fcb get_file_fcb(string fileName);

unsigned int get_file_size(string fileName); //获取文件大小

#endif

