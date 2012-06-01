#ifndef DRIVER_DEF_H_
#define DRIVER_DEF_H_

#include "../../include/ShareMemoryStruct.h"

typedef struct fly_global_mmap_struct_info{

	volatile  FLY_SHARE_MEMORY_COMMON_DATA *pShareMemoryCommonData;

}FLY_GLOBAL_MMAP_STRUCT_INFO;

extern FLY_GLOBAL_MMAP_STRUCT_INFO GlobalShareMmapInfo;

typedef void (*_t_ipcDriver)(BYTE enumWhat);

struct __global_fops{
	_t_ipcDriver _p_ipcDriver[IPC_DRIVER_MAX];
};
extern struct __global_fops global_fops;

#if CONSOLE_DEBUG
extern int FLY_CONSOLE;
#endif

extern UINT32 GetTickCount(void);
extern void ipcStartEvent(UINT32 sourceEvent);
extern BOOL ipcWhatEventOn(UINT32 sourceEvent);
extern void ipcClearEvent(UINT32 sourceEvent);

void ipcDriverStart(BYTE enumWhatDriver,BYTE enumWhatEvent);

extern void ADC_Gpio_Init(void);
extern void ADC_Get_Values(u32 channel , u32 *value);

#endif