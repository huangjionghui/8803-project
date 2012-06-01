#ifndef FLY_HALRADIO_H_
#define FLY_HALRADIO_H_

void halRadioFirstInit(void);

UINT noBlockRadioMessage(BYTE *pData,UINT length);
void dealRadioCommand(BYTE *buf, UINT len);

#endif
