#pragma once
#include "memory.h"


extern map<int, PCBptr> pidPool;
extern map<int, string> procName;
extern QString memBuffer;  
QMutex memBufferLock;
MemPtr memPtr = NULL; 



int flag[USE_SIZE/PAGE_SIZE];//标记内存块被哪个进程占用  
int timeInMem[USE_SIZE / PAGE_SIZE];
int pcbFlag[PCB_SIZE / SINGLE_PCB];//标记pcb被谁占用


void memInfo(string s)
{

	memBufferLock.lock();
	memBuffer.append(QString::fromStdString("        "+s+"\r\n"));
	memBufferLock.unlock();

}
void create_mem()
{
	memPtr = (MemPtr)malloc(sizeof(struct Mem));
	for (int i = 0;i < USE_SIZE / PAGE_SIZE;i++)
	{
		flag[i] = -1;
		timeInMem[i] = 0;
	}
	for (int i = 0;i < PCB_SIZE/SINGLE_PCB;i++)
		pcbFlag[i] = -1;

	memInfo("Memory initialized"); 

	if (memPtr != NULL) {
		memset(memPtr, -1, MEM_SIZE);
	}
	else {
		system_log("ERROR AT MEMORY MALLOC");
	}
}

BYTE* create_table()
{
	for (int i =0;i < PAGE_TABLE_SIZE;i = i +64 * 3)
	{
		if (memPtr->pageTable[i] == (BYTE)(-1) && (i + 64 * 3 < PAGE_TABLE_SIZE))
		{
			return &memPtr->pageTable[i];
		}
	}
}

void modify_table(BYTE vAdr, BYTE pAdr, BYTE diskAdr, BYTE flag, BYTE* pageTableAdr)
{
	int i = 3 * vAdr;
	pageTableAdr[i] = pAdr;
	pageTableAdr[i + 1] = diskAdr;
	pageTableAdr[i + 2] = flag;
}

void delete_table(BYTE* pageTableAdr)
{
	for (int i = 0;i < 64 * 3;i++)
		pageTableAdr[i] = -1;
}

BYTE* apply_mem_process(string fileName,int pid)
{
	//memInfo("apply meme...");

	string output;
	BYTE* insSize = (BYTE*)malloc(sizeof(BYTE) * 3);//暂存指令，读取申请的空间
	vector<BYTE*> insSizeVec;
	insSizeVec.push_back(insSize);
	read_file(fileName, insSizeVec, 3);
	int moreSize = insSizeVec[0][1] * 256 + insSizeVec[0][2];//指令申请的空间大小
	int size = get_file_size(fileName);
	int pageSize;
	int sumSize = moreSize + size;//总大小
	BYTE* pagetableAdr;
	if (sumSize % PAGE_SIZE == 0)
		pageSize = sumSize / PAGE_SIZE;
	else
		pageSize = sumSize / PAGE_SIZE + 1;
	if (is_enough(pageSize))
	{
		pagetableAdr = enough_alloc(fileName, pageSize, size, pid);
		if (pagetableAdr == 0)
		{
			output = "process " + procName[pid] + "(pid:" + to_string(pid) + ")" + " dont have enough mem";
			memInfo(output);
			return 0;
		}
		else
			return pagetableAdr;
	}
	else
	{
		output = "process " + procName[pid] + "(pid:" + to_string(pid) + ")" + " dont have enough mem";
		memInfo(output);
		return 0;
	}

	
}

bool is_enough(int pageSize)
{
	int count = 0;
	for (int i = 0;i < USE_SIZE/PAGE_SIZE;i++)
	{
		if (flag[i] == -1)
			count++;
	}
	if (pageSize >= 3 && count >= 3)
		return true;
	else if (pageSize < 3 && count >= pageSize)
		return true;
	else
		return false;
}

BYTE* enough_alloc(string fileName,int pageSize,int size,int pid)
{
	string output;
	BYTE* pagetableAdr;
	pagetableAdr = create_table();
	vector<BYTE*> pageVec;//需要加载代码的内存块起始地址
	vector<BYTE> diskPageVec;//加载到磁盘代码对应的磁盘号
	for (int i = 0, j = 0;j < pageSize && j < 3;i++)
	{
		if (flag[i] == -1)
		{
			flag[i] = pid;
			pageVec.push_back(&memPtr->useMem[i * PAGE_SIZE]);
			modify_table((BYTE)j, (BYTE)i, -1, 1, pagetableAdr);
			output = "process " + procName[pid] + "(pid:" + to_string(pid) + ")" + " get the membolok " + to_string(i);
			memInfo(output);
			j++;
		}
	}
	if (pageSize < 3 )
	{
		if (!read_file(fileName, pageVec, size))
		{
			delete_table(pagetableAdr);
			clear_use(pid);
			return 0;
		}
		else
			return pagetableAdr;
	}
	else
	{
		int ret = read_file_vm(fileName, pageVec, 3, pageSize, diskPageVec, pid);
		if (ret == 0)
		{
			delete_table(pagetableAdr);
			clear_use(pid);
			return 0;
		}
		else if(ret==-1)
		{
			delete_table(pagetableAdr);
			clear_use(pid);
			return 0;
		}
		else
		{
			for (int i = 3;i < pageSize;i++)
			{
				modify_table(BYTE(i), -1, diskPageVec[i - 3], 0, pagetableAdr);
			}
			return pagetableAdr;
		}
		
	}
	
}

void pagefault_manage(int pid, int logAdr)
{
	string output;
	int pageNum = logAdr / PAGE_SIZE;
	output = " process " + procName[pid] + "(pid:" + to_string(pid) + ")" + " meet a pagefault in logic pageNumber " + to_string(pageNum);
	memInfo(output);
	BYTE* pagetableAdr = pidPool[pid]->memPtr;
	BYTE diskNum = pagetableAdr[pageNum * 3 + 1];//需要换入的磁盘号
	int maxTime = 0;
	int outPage = 0;
	for (int i = 0;i < USE_SIZE / PAGE_SIZE;i++)//根据LRU找到需要换出的内存块
	{
		if (flag[i] == pid && timeInMem[i] > maxTime)
		{
			maxTime = timeInMem[i];
			outPage = i;
		}
	}
	vm_swap(&memPtr->useMem[outPage * PAGE_SIZE], diskNum);
	output = " process " + procName[pid] + "(pid:" + to_string(pid) + ")" + " have a pagechage,the outpage number is " + to_string(outPage);
	memInfo(output);
	for (int i = 0;i < 64 * 3;i++)
	{
		if (pagetableAdr[i] == outPage)
		{
			pagetableAdr[i + 2] =0;
			pagetableAdr[i + 1] = diskNum;
			break;
		}
	}
	modify_table((BYTE)pageNum, (BYTE)outPage, 0, 1, pagetableAdr);
}

void release_process(int pid)
{
	string output = " process " + procName[pid] + "(pid:" + to_string(pid) + ") released";
	clear_use(pid);
	delete_table(pidPool[pid]->memPtr);
	clear_pcb(pid);
	free_vm(pid);

	
	memInfo(output);
}

BYTE* find_phAdr(int pid, int vAdr)
{
	string output;
	PCBptr ptr;
	ptr = pidPool[pid];
	int vPageNum = vAdr / PAGE_SIZE;
	int offset = vAdr % PAGE_SIZE;
	BYTE* pagetableAdr = &ptr->memPtr[3 * vPageNum];//对应页表项的起始地址
	int pPageNum;
	int pageflag = pagetableAdr[2];
	if (pagetableAdr[0] == (BYTE)(-1) && pagetableAdr[2] == (BYTE)(-1) && pagetableAdr[1] == (BYTE)(-1))
	{
		output = " process " + procName[pid] + "(pid:" + to_string(pid) + ") addr " + to_string(vAdr) + " out the range it apply";
		memInfo(output);
		return (BYTE*)(-1);
	}
	else if (pageflag == 0)
		return 0;
	else
	{
		pPageNum = pagetableAdr[0];//找到对应内存块号
		for (int i =0; i < USE_SIZE / PAGE_SIZE;i++)
		{
			if (flag[i] == pid)
			{
				if (i == pPageNum)
					timeInMem[i] = 0;
				else
					timeInMem[i]++;
			}
		}
		return &memPtr->useMem[pPageNum * PAGE_SIZE + offset];
	}
}

BYTE* pcb_apply_mem(int pid)
{
	for (int i = 0;i < PCB_SIZE / SINGLE_PCB;i++)
	{
		if (pcbFlag[i] == -1)
		{
			pcbFlag[i] = pid;
			return &memPtr->pcbMem[i * SINGLE_PCB];
		}
	}
}



void clear_use(int pid)
{
	string output;
	for (int i = 0;i < USE_SIZE / PAGE_SIZE;i++)
	{
		if (flag[i] == pid)
		{
			output = " process " + procName[pid] + "(pid:" + to_string(pid) + ")" + " relaese the memblock " + to_string(i);
			memInfo(output);
			flag[i] = -1;
			timeInMem[i] = 0;
		}
	}
}

void clear_pcb(int pid)
{
	for (int i = 0;i < PCB_SIZE / SINGLE_PCB ;i++)
	{
		if (pcbFlag[i] == pid)
		{
			pcbFlag[i] = -1;
		}
	}
}
