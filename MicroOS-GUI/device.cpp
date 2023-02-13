#include "device.h"

SDT* SDTPtr;
DCT* DCTPtr;

extern MemPtr memPtr;
extern map<int, PCBptr> pidPool;

void init_device() {
	int i;
	SDTPtr = (SDT *)memPtr->selMem;
	SDTPtr->size = 0;
	DCTPtr = (DCT*)(memPtr->selMem + sizeof(SDT));
	DCTPtr->size = 0;
	for (i = 0; i < MAX_SDT_LEN; i++) {
		registe_device(i);
	}

}

int registe_device(unsigned char deviceID) {
	SDTItem* tmpSDTItemPtr;
	tmpSDTItemPtr = &(SDTPtr->SDTitem[SDTPtr->size]);
	tmpSDTItemPtr->deviceID = deviceID;
	tmpSDTItemPtr->DCTItemPtr = &(DCTPtr->DCTitem[DCTPtr->size]);

	tmpSDTItemPtr->DCTItemPtr->busy = false;
	tmpSDTItemPtr->DCTItemPtr->deviceID = deviceID;
	tmpSDTItemPtr->DCTItemPtr->waitingQueueHead = (PCBPtrQueue*)malloc(sizeof(PCBPtrQueue)); 
	if (tmpSDTItemPtr->DCTItemPtr->waitingQueueHead != NULL) {
		tmpSDTItemPtr->DCTItemPtr->waitingQueueHead->next = NULL;
	}
	
	tmpSDTItemPtr->DCTItemPtr->waitingQueueEnd = tmpSDTItemPtr->DCTItemPtr->waitingQueueHead;
	SDTPtr->size += 1;
	DCTPtr->size += 1;
	return 1;
}

int get_index_by_deviceID(unsigned char deviceID) {
	int i;
	for (i = 0; i < SDTPtr->size; i++) {
		if (SDTPtr->SDTitem[i].deviceID == deviceID)
			return i;
	}
	return -1;
}

int apply_device(int pid, unsigned char deviceID) {
	int index = get_index_by_deviceID(deviceID);
	DCTItem* tmpDCTPtr;
	PCBPtrQueue* tmpPCBPtr;

	if (index == -1) {
		qDebug() << "UNKNOW DEVICE : " << deviceID << endl;
		return 0;
	}
	else {
		tmpDCTPtr = SDTPtr->SDTitem[index].DCTItemPtr;
		tmpPCBPtr = (PCBPtrQueue*)malloc(sizeof(PCBPtrQueue));

		tmpDCTPtr->waitingQueueEnd->next = tmpPCBPtr;
		if (tmpPCBPtr != NULL) {
			tmpPCBPtr->pcbPtr = pidPool[pid];
			tmpPCBPtr->next = NULL;
			tmpDCTPtr->waitingQueueEnd = tmpPCBPtr;
			qDebug() << "pid: " << pid << "APPLY" << endl;
			return 1;
		}
		else {
			return 0;
		}
		
	}
}

int release_device(int pid, unsigned char deviceID) {
	int index = get_index_by_deviceID(deviceID);
	DCTItem* tmpDCTPtr;
	PCBPtrQueue* tmpPCBPtr;

	if (index == -1) {
		qDebug() << "UNKNOW DEVICE : " << deviceID << endl;
		return 0;
	}
	else {
		tmpDCTPtr = SDTPtr->SDTitem[index].DCTItemPtr;
		if (tmpDCTPtr->waitingQueueHead->next->pcbPtr->pid == pid) {
			tmpDCTPtr->waitingQueueHead->next = tmpDCTPtr->waitingQueueHead->next->next;
			if (tmpDCTPtr->waitingQueueHead->next == NULL) {
				tmpDCTPtr->waitingQueueEnd = tmpDCTPtr->waitingQueueHead;
				tmpDCTPtr->busy = false;
			}
			
			qDebug() << "pid: " << pid << "RELEASE" << endl;
			return 1;
		}
		else {
			return 0;
		}
	}

}