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
#define IVT_SIZE 1024 //�ж�������
#define SEL_SIZE 1024 //ϵͳ�豸��
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

void create_mem();//�����ڴ�
BYTE* create_table();
void modify_table(BYTE vAdr, BYTE pAdr, BYTE diskAdr, BYTE flag, BYTE* pageTableAdr);//��ĳ�����̵�ҳ������޸�;
void delete_table(BYTE* pageTableAdr);
BYTE* apply_mem_process(string fileName,int pid);//���̼��أ��ɹ�����ҳ���ַ��ʧ�ܷ���0
bool is_enough(int pageSize);//�жϵ�ǰ�ڴ�ռ�Ըý�����˵�Ƿ��㹻
BYTE* enough_alloc(string fileName,int pageSize, int size, int pid);//�ڴ��㹻������ҳ�����ؽ��̣���ҳ���޸�
void pagefault_manage(int pid, int logAdr);//ȱҳ�жϴ������Խ����ڶ�Ӧ�ռ�ʵ��ҳ���û�
void release_process(int pid);//�ͷŽ��̵Ŀռ��ҳ��
BYTE* find_phAdr(int pid, int vAdr);//�����߼���ַ���������ַ,���ڴ淵�ص�ַ�����ڷ���0
BYTE* pcb_apply_mem(int pid);


void clear_use(int pid);
void clear_pcb(int pid);
#endif