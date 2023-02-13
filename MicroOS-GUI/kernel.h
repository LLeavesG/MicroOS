#include "def.h"
#include "process.h"
#include "qdebug.h"
#include "qthread.h"

#ifndef KERNEL_H
#define KERNEL_H

#define INTERRUPT_TIMER_INTERVAL 200    // ʱ���жϣ�ÿ��ʱ��ƬΪ200ms
#define IO_INTERRUPT_HANLE_TIME 100     // IO�жϴ���ʱ�� 100ms
#define FILE_CPU_TIME 100               // FILE�жϴ���ռ��CPUʱ�� 100ms
#define DEVICE_CPU_TIME 100             // �豸�жϴ���ռ��CPUʱ�� 100ms
#define MAX_IVT_INFO_LEN 10             // �ж�����������Ϣ��󳤶� 10�ֽ�
#define MAX_IVT_ITEM 40                 // �ж�����������������

enum IVTable_TYPE { OOPS, EXCEPTION, IO_INTERRUPT, OTHER };

// �Ĵ�����
typedef struct OSRegister
{
    int PC;
    short int instRemainTime;
    BYTE instLength;
    unsigned char AR[MAX_AR_LEN];

}OSRegister, * OSRegisterPtr;

// �ж���������
typedef struct InterruptVectorTableItem {
    unsigned char id;
    bool accept;
    IVTable_TYPE type;
    unsigned char info[MAX_IVT_INFO_LEN];
    unsigned char* function;
    
}IVTableItem, *IVTableITemptr;

// �ж�������
typedef struct InterruptVectorTable {
    unsigned char size;
    IVTableItem ivtItem[MAX_IVT_ITEM];
}IVTable, *IVTableptr;

// CPU�����߳�
class CPUThread : public QThread {
protected:
    virtual void run();
};

// ģ���첽����IO�߳�
class AsynchronousDISKIO : public QThread {
public:
    short int time;
    string filename;
    int pid;
protected:
    virtual void run();
};


// ģ���첽�豸ʹ���߳�
class AsynchronousDevice : public QThread {
public:
    unsigned char deviceID;
    short int time;
    bool IOType;
    int pid;
protected:
    virtual void run();
};


void init_system();         // ϵͳ��ʼ��
void init_opcode_map();     // ָ���ֽ���ӳ���ʼ��
void init_ivtable();        // �ж��������ʼ��
void init_osRegister();     // ��ʼ���Ĵ�����

void exit_system();         // �˳�����ϵͳ

HANDLE create_timer(unsigned int period, WAITORTIMERCALLBACK task, void* arg);  // ����ʱ���жϼ�ʱ��
VOID CALLBACK timer_interrupt(PVOID lpParam, BOOLEAN TimerOrWaitFired);         // ʱ���ж�
void DISK_interrupt(string filename, int pid);                                  // �����ж�
void IO_interrupt(int pid, bool IOtype);                                        // IO�ж�
void DEVICE_interrupt(int pid, unsigned char deviceID);                         // �豸�ж�

short int get_inst_time(BYTE* AR, BYTE instLength);         // ��ȡָ���ʱ��
short int get_inst_cpu_time(BYTE* AR, BYTE instLength);     // ��ȡCPU��ʱ
short int get_filename_offset(BYTE* AR, BYTE instLength);   // ��ȡ�ļ������ڶ������ļ��е�λ��
unsigned char get_deviceID(BYTE* AR);                       // ��ȡ�豸ID

bool load_inst(PCBptr pcb);                         // CPU����ָ��
void cache_pcb(PCBptr pcb, short int runTime);      // ʱ���жϴ������浱ǰ����ִ����Ϣ
void inst_enter(PCBptr current);                    // ָ�����CPUʱ����
void inst_leave(PCBptr current);                    // ָ���뿪CPUʱ����

void strip(char* buff, int length);                 // ȥ���س�����
int compile(string srcPath, string outPath);        // ����
void proc_Info(string s, string d, int pid);

#endif



