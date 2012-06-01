#ifndef FLY_MCUPROC_H_
#define FLY_MCUPROC_H_

void mcuFirstInit(void);

void LPCPowerOnOK(void);
void LPCControlHowLongToPowerOn(ULONG iTime);
void LPCControlToSleep(void);
void LPCControlReset(void);

void LPCCombinDataStream(BYTE *p, UINT len);

void systemReqRadioAD(void);

#endif
