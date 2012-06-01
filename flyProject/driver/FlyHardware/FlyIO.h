#ifndef FLY_IO_H_
#define FLY_IO_H_

void ioPowerOff(void);
void ioPowerOn(void);
void ioFirstInit(void);

void ioControlAMPOn(BOOL bOn);

void controlLCDPWM(BYTE iPWM);
void ioControlLCDIdleStatus(BOOL bStatus);
void ioControl3GPowerEn(BOOL bPowerOn);
void ioControlUSBHostReset(BOOL bReset);

void ioControlDVDReset(BYTE iPara);
void ioPowerOff(void);
void ioControlRadioANT(BYTE iPara);
void ioControlRadioAFMUTE(BYTE iPara);
void ioControlAMPMute(BOOL bMute);
void ioControl7741Reset(BOOL bOn);

#endif
