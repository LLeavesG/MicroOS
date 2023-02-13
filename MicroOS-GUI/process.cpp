#pragma once
#include "process.h"
#include "misc.h"
#include <windows.h>
int latestPid = -1;			//��ǰ���µ�pid
map<int, PCBptr> pidPool;	//���̳�
map<int, string> procName;


list<int> begin_queue;		//�½�����
list<int> waiting_queue;	//�ȴ�����
list<int> ready_queue;		//��������
list<int> running_queue;	//�����ڼ�¼��Щ����������

QMutex ready_lock;
QMutex bufferLock;
extern QString buffer;  //������


//���������Ϣ
void proc_Info(string s,string d,int pid)
{

	bufferLock.lock();
	buffer.append(QString::fromStdString("        process " + procName[pid]+"(pid:"+ to_string(pid)+")" + " had moved from " + s + " queue to " + d + " queue"+"\r\n"));
	bufferLock.unlock();
}
//�����½���
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
		
		latestPid = p->pid;//���µ�ǰ����pid

		p->address = 0x0;
		p->state = BEGIN;
		p->createTime = time(0);
		p->startTime = -1;

		memset(p->instCache, 0, MAX_AR_LEN);
		p->instLength = 3;
		p->isInstFinished = true;
		p->instRemainTime = 0;
		p->isBegin = false;
		start_proc(p->pid);//����ready����
		
		//proc_Info("ready", "memory", p->pid);
	}
	else {
		cout << "Space not enough!" << endl;
	}
}

//kill����
void kill_proc(int pid)
{
	map<int, PCBptr>::iterator it = pidPool.find(pid);
	if (it != pidPool.end()) {
		pidPool[pid]->state = DIED;
		ready_lock.lock();
		ready_queue.remove(pid);
		ready_lock.unlock();
		release_process(pid);//˳���ܱ�
		pidPool.erase(it);
	}
	else {
		qDebug() << "�ý��̲�����!" << endl;
	}
}


//չʾpidPool�����н�����Ϣ
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
// ����I/O���¼��ĵȴ�
void running_to_waiting(int pid)
{
	if (find_in_list(pid, running_queue)){
		proc_Info("running", "waiting", pid);
		running_queue.remove(pid);
		waiting_queue.push_back(pid);
		pidPool[pid]->state = WAITING;

		

	}
	else
		cout << "�ý���δ��������!" << endl;
}
// ����I/O���¼������
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
		cout << "�ý���δ��waiting����!" << endl;
}
// �����ж�
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
		cout << "�ý���δ��������!" << endl;
}
// ��������
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
		cout << "�ý���δ��ready����!" << endl;
}
// BEGIN->READY,ִ�н���
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
		cout << "�ý��̲���BEGIN״̬" << endl;
}
// ���н����˳�
void exit_proc(int pid)
{
	if (pidPool[pid]->state == RUNNING) {
		pidPool[pid]->state = FINISHED;
		map<int, PCBptr>::iterator it = pidPool.find(pid);
		pidPool[pid]->state = FINISHED;
		release_process(pid);//˳���ܴ�
		pidPool.erase(it);

		bufferLock.lock();
		buffer.clear();
		buffer.append(QString::fromStdString("         process (pid:"+to_string(pid)+") " + procName[pid] + " had exited"));
		bufferLock.unlock();
		;
	}
	else
		cout << "�ý���δ��������" << endl;
}
//�ж�x�Ƿ���list l��
bool find_in_list(int x, list<int> l)
{
	list<int>::iterator it = find(l.begin(), l.end(), x);
	if (it != l.end())
		return true;
	else
		return false;
}

//RR��ѯ�����㷨,���ؼ������н��̵�pid,�����ʱ��Ƭģ�⺯��
int sche_proc()
{
	ready_lock.lock();
	//ready���в���ʱ,ȡ���еĵ�һ��pid
	if (!ready_queue.empty()){
		int curPid = ready_queue.front();
		ready_queue.pop_front();
		ready_queue.push_back(curPid);
		ready_lock.unlock();
		return curPid;
	}
	else {
		cout << "ready����Ϊ��" << endl;
		ready_lock.unlock();
		return -1;
	}
}

void clear_list()
{
	//list<int> begin_queue;		//�½�����
	//list<int> waiting_queue;	//�ȴ�����
	//list<int> ready_queue;		//��������
	//list<int> running_queue;	//�����ڼ�¼��Щ����������
	begin_queue.clear();
	waiting_queue.clear();
	ready_queue.clear();
	running_queue.clear();
}