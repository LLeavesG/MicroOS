#include "def.h"
#include "process.h"
#include "qdebug.h"
#include "qthread.h"

#ifndef KERNEL_H
#define KERNEL_H

#define INTERRUPT_TIMER_INTERVAL 200    // 时间中断，每个时间片为200ms
#define IO_INTERRUPT_HANLE_TIME 100     // IO中断处理时间 100ms
#define FILE_CPU_TIME 100               // FILE中断处理占用CPU时间 100ms
#define DEVICE_CPU_TIME 100             // 设备中断处理占用CPU时间 100ms
#define MAX_IVT_INFO_LEN 10             // 中断向量表项信息最大长度 10字节
#define MAX_IVT_ITEM 40                 // 中断向量表最大表项数量

enum IVTable_TYPE { OOPS, EXCEPTION, IO_INTERRUPT, OTHER };

// 寄存器组
typedef struct OSRegister
{
    int PC;
    short int instRemainTime;
    BYTE instLength;
    unsigned char AR[MAX_AR_LEN];

}OSRegister, * OSRegisterPtr;

// 中断向量表项
typedef struct InterruptVectorTableItem {
    unsigned char id;
    bool accept;
    IVTable_TYPE type;
    unsigned char info[MAX_IVT_INFO_LEN];
    unsigned char* function;
    
}IVTableItem, *IVTableITemptr;

// 中断向量表
typedef struct InterruptVectorTable {
    unsigned char size;
    IVTableItem ivtItem[MAX_IVT_ITEM];
}IVTable, *IVTableptr;

// CPU独立线程
class CPUThread : public QThread {
protected:
    virtual void run();
};

// 模拟异步磁盘IO线程
class AsynchronousDISKIO : public QThread {
public:
    short int time;
    string filename;
    int pid;
protected:
    virtual void run();
};


// 模拟异步设备使用线程
class AsynchronousDevice : public QThread {
public:
    unsigned char deviceID;
    short int time;
    bool IOType;
    int pid;
protected:
    virtual void run();
};


void init_system();         // 系统初始化
void init_opcode_map();     // 指令字节码映射初始化
void init_ivtable();        // 中断向量表初始化
void init_osRegister();     // 初始化寄存器组

void exit_system();         // 退出操作系统

HANDLE create_timer(unsigned int period, WAITORTIMERCALLBACK task, void* arg);  // 创建时间中断计时器
VOID CALLBACK timer_interrupt(PVOID lpParam, BOOLEAN TimerOrWaitFired);         // 时间中断
void DISK_interrupt(string filename, int pid);                                  // 磁盘中断
void IO_interrupt(int pid, bool IOtype);                                        // IO中断
void DEVICE_interrupt(int pid, unsigned char deviceID);                         // 设备中断

short int get_inst_time(BYTE* AR, BYTE instLength);         // 获取指令的时间
short int get_inst_cpu_time(BYTE* AR, BYTE instLength);     // 获取CPU耗时
short int get_filename_offset(BYTE* AR, BYTE instLength);   // 获取文件名称在二进制文件中的位置
unsigned char get_deviceID(BYTE* AR);                       // 获取设备ID

bool load_inst(PCBptr pcb);                         // CPU加载指令
void cache_pcb(PCBptr pcb, short int runTime);      // 时间中断触发保存当前进程执行信息
void inst_enter(PCBptr current);                    // 指令进入CPU时操作
void inst_leave(PCBptr current);                    // 指令离开CPU时操作

void strip(char* buff, int length);                 // 去除回车符号
int compile(string srcPath, string outPath);        // 编译
void proc_Info(string s, string d, int pid);

#endif



