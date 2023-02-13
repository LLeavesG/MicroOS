#pragma once

#include "def.h"
#include "misc.h"
#include "process.h"
#include "file.h"

#ifndef MEMORY_H
#define MEMORY_H
#define MEM_SIZE 32*1024
#define PAGE_SIZE 1024  
#define V_MEM_SIZE 64 * 1024
#define PCB_SIZE 2*1024
#define IVT_SIZE 1024 //中断向量表
#define SEL_SIZE 1024 //系统设备表
#define PAGE_TABLE_SIZE 5 * 1024
#define USE_SIZE 23*1024
#define SINGLE_PCB sizeof(PCB)



typedef struct Mem {
	BYTE pcbMem[PCB_SIZE];
	BYTE ivtMem[IVT_SIZE];
	BYTE selMem[SEL_SIZE];
	BYTE pageTable[PAGE_TABLE_SIZE];
	BYTE useMem[USE_SIZE];
}Mem,*MemPtr;

typedef struct pagetable{
	BYTE pAdr;
	BYTE diskAdr;
	BYTE flag;
}pagetable;

void create_mem();//创建内存
BYTE* create_table();
void modify_table(BYTE vAdr, BYTE pAdr, BYTE diskAdr, BYTE flag, BYTE* pageTableAdr);//对某个进程的页表进行修改;
void delete_table(BYTE* pageTableAdr);
BYTE* apply_mem_process(string fileName,int pid);//进程加载，成功返回页表地址，失败返回0
bool is_enough(int pageSize);//判断当前内存空间对该进程来说是否足够
BYTE* enough_alloc(string fileName,int pageSize, int size, int pid);//内存足够，创建页表，加载进程，对页表修改
void pagefault_manage(int pid, int logAdr);//缺页中断触发，对进程在对应空间实现页面置换
void release_process(int pid);//释放进程的空间和页表
BYTE* find_phAdr(int pid, int vAdr);//根据逻辑地址访问物理地址,在内存返回地址，不在返回0
BYTE* pcb_apply_mem(int pid);


void clear_use(int pid);
void clear_pcb(int pid);
#endif