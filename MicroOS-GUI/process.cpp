#pragma once
#include "process.h"
#include "misc.h"
#include <windows.h>
int latestPid = -1;			//当前最新的pid
map<int, PCBptr> pidPool;	//进程池
map<int, string> procName;


list<int> begin_queue;		//新建队列
list<int> waiting_queue;	//等待队列
list<int> ready_queue;		//就绪队列
list<int> running_queue;	//仅用于记录哪些进程在运行

QMutex ready_lock;
QMutex bufferLock;
extern QString buffer;  //缓存区


//输出进程信息
void proc_Info(string s,string d,int pid)
{

	bufferLock.lock();
	buffer.append(QString::fromStdString("        process " + procName[pid]+"(pid:"+ to_string(pid)+")" + " had moved from " + s + " queue to " + d + " queue"+"\r\n"));
	bufferLock.unlock();
}
//创建新进程
void init_proc(string filename)
{	
	
	
	int currentPid = latestPid + 1;
	//MicroOSGUI::ProStatus();
	BYTE* mem_address = apply_mem_process(filename, currentPid);
	if (mem_address!=NULL) {

		
		//qDebug() << "lcdlcajdiognaoi!" << endl;
		pidPool[currentPid] = (PCBptr)pcb_apply_mem(currentPid);
		PCBptr p = pidPool[currentPid];
		procName[currentPid] = filename;
		p->memPtr = mem_address;
		p->pid = currentPid;

		proc_Info("memory","begin", p->pid);
		
		//buffer.clear();
		
		latestPid = p->pid;//更新当前最新pid

		p->address = 0x0;
		p->state = BEGIN;
		p->createTime = time(0);
		p->startTime = -1;

		memset(p->instCache, 0, MAX_AR_LEN);
		p->instLength = 3;
		p->isInstFinished = true;
		p->instRemainTime = 0;
		p->isBegin = false;
		start_proc(p->pid);//进入ready队列
		
		//proc_Info("ready", "memory", p->pid);
	}
	else {
		cout << "Space not enough!" << endl;
	}
}

//kill进程
void kill_proc(int pid)
{
	map<int, PCBptr>::iterator it = pidPool.find(pid);
	if (it != pidPool.end()) {
		pidPool[pid]->state = DIED;
		ready_lock.lock();
		ready_queue.remove(pid);
		ready_lock.unlock();
		release_process(pid);//顺序不能变
		pidPool.erase(it);
	}
	else {
		qDebug() << "该进程不存在!" << endl;
	}
}


//展示pidPool中所有进程信息
void show_proc()
{
	map<int,PCBptr>::iterator it;
	for (it = pidPool.begin(); it != pidPool.end(); it++){
		cout << "pid: " << pidPool[it->first]->pid << endl;
		cout << "address: " << pidPool[it->first]->address << endl;
		cout << "state: " << pidPool[it->first]->state << endl;
		cout << "isInstFinished: " << pidPool[it->first]->isInstFinished << endl;
		cout << "createTime: " << pidPool[it->first]->createTime << endl;
		cout << "startTime: " << pidPool[it->first]->startTime << endl;
		cout << "instLength: " << pidPool[it->first]->instLength << endl;
		cout << "instCache: " << pidPool[it->first]->instCache << endl;
		cout << "instRemainTime: " << pidPool[it->first]->instRemainTime << endl << endl;
	}
}
// 发生I/O或事件的等待
void running_to_waiting(int pid)
{
	if (find_in_list(pid, running_queue)){
		proc_Info("running", "waiting", pid);
		running_queue.remove(pid);
		waiting_queue.push_back(pid);
		pidPool[pid]->state = WAITING;

		

	}
	else
		cout << "该进程未在运行中!" << endl;
}
// 发生I/O或事件的完成
void waiting_to_ready(int pid)
{
	if (find_in_list(pid, waiting_queue)) {
		
		proc_Info("waiting", "ready", pid);
		waiting_queue.remove(pid);

		ready_lock.lock();
		ready_queue.push_back(pid);
		ready_lock.unlock();

		pidPool[pid]->state = READY;

		
		
	}
	else
		cout << "该进程未在waiting队列!" << endl;
}
// 发生中断
void running_to_ready(int pid)
{
	if (find_in_list(pid, running_queue)) {

		proc_Info("running", "ready", pid);
		running_queue.remove(pid);

		ready_lock.lock();
		ready_queue.push_back(pid);
		ready_lock.unlock();

		pidPool[pid]->state = READY;

		
	}
	else
		cout << "该进程未在运行中!" << endl;
}
// 发生调度
void ready_to_running(int pid)
{
	if (find_in_list(pid, ready_queue)) {
		proc_Info("ready", "running", pid);
		ready_lock.lock();
		ready_queue.remove(pid);
		ready_lock.unlock();
		pidPool[pid]->state = RUNNING;
		if (pidPool[pid]->startTime == -1) {
			pidPool[pid]->startTime = time(0);
		}
		running_queue.push_back(pid);

		
	}
	else
		cout << "该进程未在ready队列!" << endl;
}
// BEGIN->READY,执行进程
void start_proc(int pid)
{
	if (pidPool[pid]->state == BEGIN) {

		proc_Info("begin", "ready", pid);

		ready_lock.lock();
		ready_queue.push_back(pid);/**/
		ready_lock.unlock();
		pidPool[pid]->state = READY;

		
	}
	else
		proc_Info("begin", "ready", pid);
		cout << "该进程不在BEGIN状态" << endl;
}
// 运行结束退出
void exit_proc(int pid)
{
	if (pidPool[pid]->state == RUNNING) {
		pidPool[pid]->state = FINISHED;
		map<int, PCBptr>::iterator it = pidPool.find(pid);
		pidPool[pid]->state = FINISHED;
		release_process(pid);//顺序不能错
		pidPool.erase(it);

		bufferLock.lock();
		buffer.clear();
		buffer.append(QString::fromStdString("         process (pid:"+to_string(pid)+") " + procName[pid] + " had exited"));
		bufferLock.unlock();
		;
	}
	else
		cout << "该进程未在运行中" << endl;
}
//判断x是否在list l中
bool find_in_list(int x, list<int> l)
{
	list<int>::iterator it = find(l.begin(), l.end(), x);
	if (it != l.end())
		return true;
	else
		return false;
}

//RR轮询调度算法,返回即将运行进程的pid,需配合时间片模拟函数
int sche_proc()
{
	ready_lock.lock();
	//ready队列不空时,取队列的第一个pid
	if (!ready_queue.empty()){
		int curPid = ready_queue.front();
		ready_queue.pop_front();
		ready_queue.push_back(curPid);
		ready_lock.unlock();
		return curPid;
	}
	else {
		cout << "ready队列为空" << endl;
		ready_lock.unlock();
		return -1;
	}
}

void clear_list()
{
	//list<int> begin_queue;		//新建队列
	//list<int> waiting_queue;	//等待队列
	//list<int> ready_queue;		//就绪队列
	//list<int> running_queue;	//仅用于记录哪些进程在运行
	begin_queue.clear();
	waiting_queue.clear();
	ready_queue.clear();
	running_queue.clear();
}