#ifndef FLY_HALPROC_H_
#define FLY_HALPROC_H_

void messageToRadioHAL(BYTE *p,UINT length);

void messageMCUToSystemHAL(BYTE *p,UINT length);
void messageNormalToSystemHAL(BYTE *p,UINT length);

void messageToKeyHAL(BYTE *p,UINT length);
void messageToServiceHAL(BYTE *p,UINT length);

void dealDataFromUser(BYTE *buf, UINT len);
UINT32 noBlockMessageToHAL(BYTE currentHAL,BYTE *pData, UINT32 len);
UINT blockMessageToHAL(BYTE *pData,UINT length);

void halProcFirstInit(void);

#endif
