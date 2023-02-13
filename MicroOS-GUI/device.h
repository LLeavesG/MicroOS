#pragma once

#include "def.h"
#include "process.h"

#ifndef DEVICE_H
#define DEVICE_H
#define MAX_DCT_LEN 32
#define MAX_SDT_LEN 32

// PCB指针队列
typedef struct PCBPtrQueue {
	PCBptr pcbPtr;
	struct PCBPtrQueue *next;
}PCBPtrQueue, * PCBPtrQueueptr;

// DCT表项
typedef struct DeviceControlTableItem {
	unsigned char deviceID;
	bool busy;
	PCBPtrQueue* waitingQueueHead; 
	PCBPtrQueue* waitingQueueEnd;

}DCTItem, * DCTItemptr;

// DCT 设备控制表，系统中的每台设备都有一张设备控制表DCT
typedef struct DeviceControlTable {
	unsigned char size;
	DCTItem DCTitem[MAX_DCT_LEN];
}DCT, * DCTptr;

// SDT系统设备表项
typedef struct SystemDeviceTableItem {
	unsigned char deviceID;
	DCTItem* DCTItemPtr;

}SDTItem, * SDTItemptr;

// SDT 系统设备表
typedef struct SystemDeviceTable {
	unsigned char size;
	SDTItem	SDTitem[MAX_SDT_LEN];
}SDT, * SDTptr;


// 初始化设备信息
void init_device();
int registe_device(unsigned char deviceID);
int get_index_by_deviceID(unsigned char deviceID);
int apply_device(int pid, unsigned char deviceID);
int release_device(int pid, unsigned char deviceID);

#endif
