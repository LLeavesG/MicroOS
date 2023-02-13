#pragma once
#include "def.h"
#include "memory.h"
#include "MicroOSGUI.h"
#ifndef PROCESS_H
#define PROCESS_H

enum proc_state {BEGIN,READY,RUNNING,WAITING,FINISHED,DIED};
/*����״̬
* �½�����״̬һ��ΪBEGIN
* �����̵�һ�ν����ڴ棬BEGIN->READY
* ���̵�һ�����У�READY->RUNNING
* ���̱����ȣ�RUNNING->WAITING  or  WAITING->READY
* ������ɣ�RUNNING->FINISHED;�������˳�ΪDIED
* ���̳�ʱ,RUNNING->READY
*/


typedef struct PCB
{
    int pid;
    int address;      // PC������(��ǰָ��)
    proc_state state; //����״̬

    BYTE* memPtr;      //�ڴ�ָ��
    BYTE* filePtr;     //�ļ�ָ��
    time_t createTime; //����ʱ��
    time_t startTime;  //��ʼ����ʱ��

    BYTE instLength;                    //��ǰָ���
    BYTE instCache[MAX_AR_LEN];         //��ǰָ���ֽ��뻺��
    short int instRemainTime;           //��ǰָ��ʣ������ʱ��
    bool isInstFinished;                //��ǵ�ǰָ���Ƿ����,0����δ���,1���������
    bool isBegin;                       //��ǵ�ǰָ���Ƿ�ʼִ��
} PCB, * PCBptr;

void show_proc();                       //չʾ���������Ϣ
void kill_proc(int pid);                //kill����
void init_proc(string filename);        //�������̼����ʼ��
bool find_in_list(int x, list<int> l);  //�ж�x�Ƿ���list l��

void running_to_waiting(int pid);       //running-->waiting
void waiting_to_ready(int pid);         //waiting-->ready
void running_to_ready(int pid);         //running-->ready
void ready_to_running(int pid);         //ready-->running
void start_proc(int pid);               //����ִ��
void exit_proc(int pid);                //�������н������˳�
int sche_proc();                        //���̵��ȣ���kernel���ؽ����н��̵�pid
void clear_list();


#endif