#ifndef FLY_HALVIDEO_H_
#define FLY_HALVIDEO_H_

void halVideoPowerOn(void);
void halVideoFirstInit(void);

void TVP5150StatusCheck(void);

UINT noBlockVideoMessage(BYTE *pData,UINT length);
void dealVideoCommand(BYTE *buf, UINT len);

#endif
