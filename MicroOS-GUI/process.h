#pragma once
#include "def.h"
#include "memory.h"
#include "MicroOSGUI.h"
#ifndef PROCESS_H
#define PROCESS_H

enum proc_state {BEGIN,READY,RUNNING,WAITING,FINISHED,DIED};
/*进程状态
* 新建进程状态一律为BEGIN
* 当进程第一次进入内存，BEGIN->READY
* 进程第一次运行，READY->RUNNING
* 进程被调度，RUNNING->WAITING  or  WAITING->READY
* 进程完成，RUNNING->FINISHED;非正常退出为DIED
* 进程超时,RUNNING->READY
*/


typedef struct PCB
{
    int pid;
    int address;      // PC计数器(当前指令)
    proc_state state; //进程状态

    BYTE* memPtr;      //内存指针
    BYTE* filePtr;     //文件指针
    time_t createTime; //创建时间
    time_t startTime;  //开始运行时间

    BYTE instLength;                    //当前指令长度
    BYTE instCache[MAX_AR_LEN];         //当前指令字节码缓存
    short int instRemainTime;           //当前指令剩余运行时间
    bool isInstFinished;                //标记当前指令是否完成,0代表未完成,1代表已完成
    bool isBegin;                       //标记当前指令是否开始执行
} PCB, * PCBptr;

void show_proc();                       //展示进程相关信息
void kill_proc(int pid);                //kill进程
void init_proc(string filename);        //创建进程及其初始化
bool find_in_list(int x, list<int> l);  //判断x是否在list l中

void running_to_waiting(int pid);       //running-->waiting
void waiting_to_ready(int pid);         //waiting-->ready
void running_to_ready(int pid);         //running-->ready
void ready_to_running(int pid);         //ready-->running
void start_proc(int pid);               //进程执行
void exit_proc(int pid);                //进程运行结束、退出
int sche_proc();                        //进程调度，向kernel返回将运行进程的pid
void clear_list();


#endif