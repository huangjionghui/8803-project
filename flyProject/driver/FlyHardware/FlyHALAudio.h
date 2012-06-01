#ifndef FLY_HALAUDIO_H_
#define FLY_HALAUDIO_H_

void halAudioPowerOn(void);
void halAudioFirstInit(void);

void dealAudioCommand(BYTE *buf, UINT len);

void switchMainAudioInput(BYTE iInput);
void switchSecondAudioInput(BYTE iInput);
void switchMainVideoInput(BYTE iInput);
void switchSecondVideoInput(BYTE iInput);

#endif
