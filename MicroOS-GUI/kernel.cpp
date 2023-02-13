#pragma once

#include "kernel.h"
#include "misc.h"
#include "memory.h"
#include "device.h"
#include "file.h"

#pragma comment(lib,"user32")
#pragma warning(disable:4996)

extern MemPtr memPtr;
extern map<int, PCBptr> pidPool;
extern SDT* SDTPtr;
extern DCT* DCTPtr;

// �Ĵ�����
OSRegister osRegister;
// ָ��ֽ���ӳ��
map<string, int> opcode_map;
// ģ�������ź�
BYTE pulse = 0;
// ������CPU�߳�
CPUThread *cpuThread;
// �ж�������
IVTable* ivTable;

// ��ָͬ���ֽ��볤��
int instLengthArray[0x10] = { 0,0,3,3,3,5,5,4,5,0,0,0,0,0,0,1 }; 



// ʱ���жϣ�ģ������������
VOID CALLBACK timer_interrupt(PVOID lpParam, BOOLEAN TimerOrWaitFired)
{
    int* arg = (int*)lpParam;
    pulse = 1;
}

void init_system()
{
    //Process
    clear_list();

    // �Ĵ������ʼ��
    init_osRegister();
        
    // ��������ʼ����
    init_opcode_map();

    // Timer�жϳ�ʼ��
    int arg = 1;
    HANDLE h1 = create_timer(INTERRUPT_TIMER_INTERVAL, timer_interrupt, &arg);

    // �ڴ�ģ���ʼ��
    create_mem();

    // �ж��������ʼ��
    init_ivtable();

    // �ļ�ϵͳ��ʼ��
    start_fileSystem();

    // TODO���豸ģ���ʼ��
    init_device();

    // CPU�߳�����
    cpuThread = new CPUThread();
    cpuThread->start();
}


HANDLE create_timer(unsigned int period, WAITORTIMERCALLBACK task, void* arg)
{

    HANDLE HTimer = NULL;

    if (!CreateTimerQueueTimer(&HTimer, NULL, (WAITORTIMERCALLBACK)task, (PVOID)arg, 0, period , 0))
    {
        qDebug() << "CreateTimerQueueTimer failed " << GetLastError() << endl;
        return NULL;
    }

    return HTimer;
}

void CPUThread::run() {

    clock_t start = 0, end = 0;

    long long int intervel = 0;
    PCBptr current;
    int currentPid = 0;

    bool isExit = false;


    while (1) {

        // ȡһ��׼��ִ�еĽ���(��Ready״̬)
        currentPid = sche_proc();
        if (currentPid != -1) {
            current = pidPool[currentPid];

            // ȡ�����ΪRunning״̬
            if( current != NULL) ready_to_running(current->pid);
        }
        else {
            continue;
        }

        if (current == NULL) continue;

        // ȡָ
        if (!load_inst(current)) {
            exit_proc(current->pid);
            continue;
        }
        else {
            pulse = 0;
        }

        while (!pulse) {

            intervel = 0;
            start = clock();
            // �ж�ʱ��Ƭ������ָ���Ƿ����ִ��
            while (!pulse && intervel < osRegister.instRemainTime) {
                // ִ��
                if (!current->isBegin) {
                    inst_enter(current);
                }
                current->isBegin = true;
                end = clock();
                // ����CPU��ʱ
                intervel = end - start;
            }

            // ָ��������
            if (intervel >= osRegister.instRemainTime) {
                inst_leave(current);
                current->isBegin = false;
                current->isInstFinished = true;
                current->instLength = osRegister.instLength;
                if (current->state != RUNNING) break;
                if (!load_inst(current)) {
                    exit_proc(current->pid);
                    break;
                }
                intervel = 0;
            }
        }

        // ���������RUNNING״̬������õ�ready����
        if( current->state == RUNNING ) running_to_ready(current->pid);

        // �޸�PCB���л���
        cache_pcb(current, intervel);

        // �����½���
        pulse = 0;
    }
}



void inst_enter(PCBptr current) {
    // ָ�����ʱ����
    int logAddr = 0;
    BYTE* ret = 0;
    unsigned char tmpBuffer[40] = {0};
    string tmpFilename;

    void (*PageFaultfunctionPtr)(int, int);

    AsynchronousDISKIO* newDISKIOThread;
    AsynchronousDevice* newDeviceThread;

    switch (osRegister.AR[0])
    {
    case 0x02:
        // calu����ָ��
        qDebug() << "Pid: " << current->pid << "calu begin" << endl;
        break;
    case 0x03:
        // keyb��������ָ��

        qDebug() << "Pid: " << current->pid << "keyboard begin" << endl;
        // �����̷���
        running_to_waiting(current->pid);
        running_to_waiting(current->pid);
        qDebug() << "Pid: " << current->pid << "device begin" << endl;
        newDeviceThread = new AsynchronousDevice;
        newDeviceThread->time = get_inst_time(osRegister.AR, osRegister.instLength);
        newDeviceThread->pid = current->pid;
        newDeviceThread->deviceID = 1;
        newDeviceThread->start();
        break;
    case 0x04:
        // prin���ָ��

        qDebug() << "Pid: " << current->pid << "print begin" << endl;
        running_to_waiting(current->pid);
        qDebug() << "Pid: " << current->pid << "device begin" << endl;
        newDeviceThread = new AsynchronousDevice;
        newDeviceThread->time = get_inst_time(osRegister.AR, osRegister.instLength);
        newDeviceThread->pid = current->pid;
        newDeviceThread->deviceID = 0;
        newDeviceThread->start();
        break;
    case 0x05:
        // read���ļ�ָ��
        qDebug() << "Pid: " << current->pid << "read begin" << endl;
        running_to_waiting(current->pid);

        strcpy((char *)tmpBuffer, (const char*)find_phAdr(current->pid, get_filename_offset(osRegister.AR, osRegister.instLength)));
        tmpFilename = (char*)tmpBuffer;
        open_file(tmpFilename, RO, current->pid);
        newDISKIOThread = new AsynchronousDISKIO;
        newDISKIOThread->time = get_inst_time(osRegister.AR, osRegister.instLength);
        newDISKIOThread->pid = current->pid;
        newDISKIOThread->filename = tmpFilename;
        newDISKIOThread->start();
        break;
    case 0x06:
        // writд�ļ�ָ��
        running_to_waiting(current->pid);
        qDebug() << "Pid: " << current->pid << "write begin" << endl;
        strcpy((char*)tmpBuffer, (const char*)find_phAdr(current->pid, get_filename_offset(osRegister.AR, osRegister.instLength)));
        tmpFilename = (const char*)tmpBuffer;
        open_file(tmpFilename, RW, current->pid);
        newDISKIOThread = new AsynchronousDISKIO;
        newDISKIOThread->time = get_inst_time(osRegister.AR, osRegister.instLength);
        newDISKIOThread->pid = current->pid;
        newDISKIOThread->filename = tmpFilename;
        newDISKIOThread->start();
        break;
    case 0x07:
        // deviʹ���豸ָ��
        running_to_waiting(current->pid);
        qDebug() << "Pid: " << current->pid << "device begin" << endl;
        newDeviceThread = new AsynchronousDevice;
        newDeviceThread->time = get_inst_time(osRegister.AR, osRegister.instLength);
        newDeviceThread->pid = current->pid;
        newDeviceThread->deviceID = get_deviceID(osRegister.AR);
        newDeviceThread->start();
        break;
    case 0x08:
        // addr�ô�ָ��
        qDebug() << "Pid: " << current->pid << "addr begin" << endl;
        logAddr = (osRegister.AR[osRegister.instLength - 3] & 0x00ff) + ((osRegister.AR[osRegister.instLength - 4] << 8) & 0xff00);
        ret = find_phAdr(current->pid, logAddr);
        if ((int)ret != -1) {
            if ((int)ret == 0) {
                // ����ȱҳ�ж�
                PageFaultfunctionPtr = (void (*)(int, int))ivTable->ivtItem[4].function;
                PageFaultfunctionPtr(current->pid, logAddr);
            }
        }
        else {
            qDebug() << "Pid: " << current->pid << "illegal memory use" << endl;
        }
        break;
    default:
        break;
    }
}

void inst_leave(PCBptr current) {
    // ָ�����ʱ����
    switch (osRegister.AR[0])
    {
    case 0x02:
        // calu����ָ��
        qDebug() << "Pid: " << current->pid << "calu over" << endl;
        break;
    case 0x03:
        // keyb��������ָ��
        qDebug() << "Pid: " << current->pid << "keyboard over" << endl;
        break;
    case 0x04:
        // prin���ָ��
        qDebug() << "Pid: " << current->pid << "print over" << endl;
        break;
    case 0x05:
        // read���ļ�ָ��
        qDebug() << "Pid: " << current->pid << "read over" << endl;
        break;
    case 0x06:
        // writд�ļ�ָ��
        qDebug() << "Pid: " << current->pid << "write over" << endl;
        break;
    case 0x07:
        // deviʹ���豸ָ��
        qDebug() << "Pid: " << current->pid << "device over" << endl;
        break;
    case 0x08:
        // addr�ô�ָ��
        qDebug() << "Pid: " << current->pid << "addr over" << endl;
        break;
    default:
        break;
    }
}


short int get_inst_time(BYTE* AR, BYTE instLength) {
    return (AR[instLength - 1] & 0x00ff) + ((AR[instLength - 2] << 8) & 0xff00);
}

short int get_filename_offset(BYTE* AR, BYTE instLength) {
    return (AR[instLength - 3] & 0x00ff) + ((AR[instLength - 4] << 8) & 0xff00);
}

unsigned char get_deviceID(BYTE* AR) {
    return AR[1];
}

void exit_system()
{
    exit_fileSystem();
    cpuThread->terminate();
    exit(0);
}

short int get_inst_cpu_time(BYTE* AR, BYTE instLength) {
    short int time = 0;
    if (AR[0] == 0x02 || AR[0] == 0x08) {
        time = get_inst_time(AR, instLength);
    }
    else if (AR[0] == 0x05 || AR[0] == 0x06) {
        time = FILE_CPU_TIME;
    }
    else {
        time = DEVICE_CPU_TIME;
    }
    return time;
}

bool load_inst(PCBptr pcb) {
    
    if (pcb->isInstFinished == true) {
        // �����ǰָ���Ѿ�����ȡ��һ��
        pcb->address += pcb->instLength;
        memcpy(osRegister.AR, find_phAdr(pcb->pid, pcb->address), MAX_AR_LEN * sizeof(BYTE));

        if (osRegister.AR[0] == 0x10) {
            // EXITָ������˳�
            return false;
        }
        osRegister.instLength = instLengthArray[osRegister.AR[0]];
        osRegister.instRemainTime = get_inst_cpu_time(osRegister.AR, osRegister.instLength);
        pcb->instRemainTime = osRegister.instRemainTime;
        pcb->isInstFinished = false;
    }
    else {
        // ���û���������ִ��
        memcpy(osRegister.AR, pcb->instCache, MAX_AR_LEN * sizeof(BYTE));
        osRegister.instLength = pcb->instLength;
        osRegister.instRemainTime = pcb->instRemainTime;
    }

    qDebug() << "Pid:" << pcb->pid << "opcode:" << (int)osRegister.AR[0] << "remainTime:" << pcb->instRemainTime << endl;
    return true;

}

void cache_pcb(PCBptr pcb, short int runTime) {
    BYTE instLength = osRegister.instLength;
    memcpy(pcb->instCache, osRegister.AR, sizeof(BYTE) * instLength);
    pcb->instLength = instLength;
    pcb->instRemainTime -= runTime;
}


void AsynchronousDISKIO::run() {
    Sleep(time);
    void (*functionPtr)(string, int);
    functionPtr = (void (*)(string, int))ivTable->ivtItem[0].function;
    functionPtr(filename, pid);
    qDebug() << "Pid: " << pid << "DISK IO Thread END " << time << filename.c_str();
}

void AsynchronousDevice::run() {
    if (!apply_device(pid, deviceID)) {
        qDebug() << "Pid: " << pid << "APPLY DEVICE "<< deviceID << " ERROR";
    }
    
    DCTItem* tmpDCTItem = SDTPtr->SDTitem[get_index_by_deviceID(deviceID)].DCTItemPtr;
    // �ȴ��豸�������ǰ����
    while (tmpDCTItem->busy == false && tmpDCTItem->waitingQueueHead->next != NULL) {
        if (tmpDCTItem->waitingQueueHead->next->pcbPtr->pid == pid) {
            tmpDCTItem->busy = true;
            break;
        }
    }

    // ͨ��Sleepģ��ʹ��ʱ��
    Sleep(time);

    // �����жϸ�֪�����
    void (*functionPtr)(int, unsigned char);
    functionPtr = (void (*)(int, unsigned char))ivTable->ivtItem[3].function;
    functionPtr(pid, deviceID);
    qDebug() << "Pid: " << pid << "Device Thread END " << time;
}

void strip(char *buff, int length)
{
    if (buff[length - 1] == '\r' || buff[length - 1] == '\n')
    {
        buff[length - 1] = 0;
    }
    if (buff[length - 2] == '\r' || buff[length - 2] == '\n')
    {
        buff[length - 2] = 0;
    }
}

int compile(string srcPath, string outPath) {

    char buff[MAXLINELEN] = { 0 };

    char opcode[5] = { 0 };
    char outBuff[1024] = { 0 };

    int dataCount = 0;
    char dataBuff[20][40] = { 0 };
    char tmpNumber[6] = { 0 };

    bool isExit = false;
    stringstream srcStream;
    map<int, int> fillBack;
    vector<unsigned char*> tmpBufferVector;

    char* tmp;

    int i = 0;
    int imm = 0;
    int length = 0;

    fcb srcFcb = get_file_fcb(srcPath);
    fcb outFcb = get_file_fcb(outPath);

    unsigned char* buffer = (unsigned char*)malloc(sizeof(unsigned char) * srcFcb.size);
    if (buffer != NULL) {
        memset(buffer, 0, srcFcb.size);
        for (i = 0; i < srcFcb.length; i++) {
            tmpBufferVector.push_back(buffer + i * BLOCKSIZE);
        }
        
        read_file(srcPath, tmpBufferVector, srcFcb.size);
    }

    for (i = 0; i < srcFcb.length; i++) {
        srcStream << tmpBufferVector[i];
    }

    free(buffer);
    if (srcStream.peek() != EOF) {
        srcStream.getline((char*)buff, MAXLINELEN);
        strip(buff, strlen((const char*)buff));
        strncpy(opcode, buff, 4);
        opcode[4] = 0;

        if (strcmp(opcode, "umem") == 0 && *(buff + 4) == ' ') {
            tmp = buff + 5;
            imm = atoi((const char*)tmp);
            if (imm % PAGE_SIZE != 0) {
                return -1;
            }

            outBuff[0] = 0x1;
            outBuff[1] = (((short)imm & 0xff00) >> 8) & 0xff;
            outBuff[2] = ((short)imm & 0x00ff) & 0xff;
            length += 3;
        }
    }

    while (srcStream.peek() != EOF) {
        srcStream.getline((char*)buff, MAXLINELEN);
        strip(buff, strlen((const char*)buff));
        strncpy(opcode, buff, 4);

        opcode[4] = 0;
        map<string, int>::iterator it = opcode_map.find(string((const char*)opcode));

        if (it != opcode_map.end()) {

            outBuff[length] = it->second;
            length += 1;

            tmp = buff + 5;
            if (it->second == 5 || it->second == 6) {

                int nameLength = atoi((const char*)tmp);

                if (nameLength < 10)
                    tmp += 2;
                else if (nameLength >= 10)
                    tmp += 3;
                else {
                    return -1;
                }

                memcpy(dataBuff[dataCount], tmp, nameLength);
                fillBack[length] = dataCount;
                dataCount++;

                outBuff[length] = 0;
                outBuff[length + 1] = 0;

                tmp = tmp + nameLength + 1;

                imm = atoi((const char*)tmp);
                outBuff[length + 2] = (((short)imm & 0xff00) >> 8) & 0xff;
                outBuff[length + 3] = ((short)imm & 0x00ff) & 0xff;

                length = length + 4;
            }
            else if (it->second == 7) {
                int deviceID = atoi((const char*)tmp);
                outBuff[length++] = deviceID & 0xff;

                if(deviceID == 0 || deviceID == 1){
                    return -1;
                }

                if (deviceID < 10)
                    tmp += 2;
                else if (deviceID >= 10 && deviceID <= 32)
                    tmp += 3;
                else {
                    return -1;
                }

                imm = atoi((const char*)tmp);

                outBuff[length++] = (((short)imm & 0xff00) >> 8) & 0xff;
                outBuff[length++] = ((short)imm & 0x00ff) & 0xff;

            }
            else if (it->second == 0x08) {
                int address = atoi((const char*)tmp);
                outBuff[length++] = (((short)address & 0xff00) >> 8) & 0xff;
                outBuff[length++] = ((short)address & 0x00ff) & 0xff;

                memset(tmpNumber, 0, 6);
                sprintf(tmpNumber, "%d", address);
                tmp = tmp + strlen(tmpNumber) + 1;

                imm = atoi((const char*)tmp);
                outBuff[length++] = (((short)imm & 0xff00) >> 8) & 0xff;
                outBuff[length++] = ((short)imm & 0x00ff) & 0xff;
            }
            else if (it->second != 0x10) {

                imm = atoi((const char*)tmp);
                outBuff[length++] = (((short)imm & 0xff00) >> 8) & 0xff;
                outBuff[length++] = ((short)imm & 0x00ff) & 0xff;

            }
            else if (it->second == 0x10) {
                isExit = true;
            }
        }
        else
        {
            return -1;
        }
    }

    if (!isExit) {
        return -1;
    }
    map<int, int>::iterator fillBackIt;

    // ʹ�û�����������ݶε�ַ
    for (fillBackIt = fillBack.begin(); fillBackIt != fillBack.end(); fillBackIt++) {

        memcpy(outBuff + length, dataBuff[fillBackIt->second], sizeof(char) * strlen(dataBuff[fillBackIt->second]));
        outBuff[fillBackIt->first] = ((length & 0xff00) >> 8) & 0xff;
        outBuff[fillBackIt->first + 1] = (length & 0x00ff) & 0xff;
        length += strlen(dataBuff[fillBackIt->second]);
        outBuff[length++] = 0x0;
    }

    tmpBufferVector.clear();
    int blockSize = length / BLOCKSIZE == 0 ? 1: length / BLOCKSIZE;
    if (length / BLOCKSIZE != 0 && length % blockSize != 0) {
        blockSize += 1;
    }
    if (outBuff != NULL) {
        for (i = 0; i < blockSize; i++) {
            tmpBufferVector.push_back((unsigned char*)outBuff + i * BLOCKSIZE);
        }
        
        // ���������ļ�д��
        write_file(outPath, tmpBufferVector, length);
    }
    return 0;
}


void DISK_interrupt(string filename, int pid) {
    close_file(filename, pid);
    waiting_to_ready(pid);
}

void IO_interrupt(int pid, bool IOtype) {
    waiting_to_ready(pid);
}


void DEVICE_interrupt(int pid, unsigned char deviceID) {
    release_device(pid, deviceID);
    waiting_to_ready(pid);
}

void init_opcode_map()
{
    opcode_map["calu"] = 0x02;
    opcode_map["keyb"] = 0x03;
    opcode_map["prin"] = 0x04;
    opcode_map["read"] = 0x05;
    opcode_map["writ"] = 0x06;
    opcode_map["devi"] = 0x07;
    opcode_map["addr"] = 0x08;
    opcode_map["exit"] = 0x10;
}

void init_osRegister() {
    osRegister.PC = 0;
    osRegister.instLength = 0;
    osRegister.instRemainTime = 0;
    memset(osRegister.AR, 0, MAX_AR_LEN);

}

void init_ivtable() {
    int i = 0;
    ivTable = (IVTableptr)memPtr->ivtMem;
    ivTable->size = 3;

    // �첽IO�ж�   ������ɺ��֪CPU
    ivTable->ivtItem[0].id = 0x0;
    ivTable->ivtItem[0].accept = true; 
    ivTable->ivtItem[0].type = IO_INTERRUPT;
    memcpy(ivTable->ivtItem[0].info, "IO", 2);
    ivTable->ivtItem[0].function = (unsigned char *)DISK_interrupt;

    // 1 2 �ж�����

    // �豸�ж�     �豸ʹ�����ͨ���ж�֪ͨCPU
    ivTable->ivtItem[3].id = 0x3;
    ivTable->ivtItem[3].accept = true;
    ivTable->ivtItem[3].type = IO_INTERRUPT;
    memcpy(ivTable->ivtItem[3].info, "DEVICE", 6);
    ivTable->ivtItem[3].function = (unsigned char*)DEVICE_interrupt;

    // ȱҳ�ж�     ���ʵ������ڴ��е�ַʱ��Ҫ����ȱҳ�жϵ���ҳ��
    ivTable->ivtItem[4].id = 0x4;
    ivTable->ivtItem[4].accept = true;
    ivTable->ivtItem[4].type = EXCEPTION;
    memcpy(ivTable->ivtItem[4].info, "Page", 4);
    ivTable->ivtItem[4].function = (unsigned char*)pagefault_manage;

}