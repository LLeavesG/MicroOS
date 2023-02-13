#pragma once

#include "def.h"
#include "process.h"

#ifndef DEVICE_H
#define DEVICE_H
#define MAX_DCT_LEN 32
#define MAX_SDT_LEN 32

// PCBָ�����
typedef struct PCBPtrQueue {
	PCBptr pcbPtr;
	struct PCBPtrQueue *next;
}PCBPtrQueue, * PCBPtrQueueptr;

// DCT����
typedef struct DeviceControlTableItem {
	unsigned char deviceID;
	bool busy;
	PCBPtrQueue* waitingQueueHead; 
	PCBPtrQueue* waitingQueueEnd;

}DCTItem, * DCTItemptr;

// DCT �豸���Ʊ�ϵͳ�е�ÿ̨�豸����һ���豸���Ʊ�DCT
typedef struct DeviceControlTable {
	unsigned char size;
	DCTItem DCTitem[MAX_DCT_LEN];
}DCT, * DCTptr;

// SDTϵͳ�豸����
typedef struct SystemDeviceTableItem {
	unsigned char deviceID;
	DCTItem* DCTItemPtr;

}SDTItem, * SDTItemptr;

// SDT ϵͳ�豸��
typedef struct SystemDeviceTable {
	unsigned char size;
	SDTItem	SDTitem[MAX_SDT_LEN];
}SDT, * SDTptr;


// ��ʼ���豸��Ϣ
void init_device();
int registe_device(unsigned char deviceID);
int get_index_by_deviceID(unsigned char deviceID);
int apply_device(int pid, unsigned char deviceID);
int release_device(int pid, unsigned char deviceID);

#endif
