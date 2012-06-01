#ifndef FLY_HALSYSTEM_H_
#define FLY_HALSYSTEM_H_

void systemControlAccOn(void);

UINT noBlockSystemMessage(BYTE *pData,UINT length);
void dealSystemCommand(BYTE *buf, UINT len);

void systemLCDPWMProc(void);

#endif
