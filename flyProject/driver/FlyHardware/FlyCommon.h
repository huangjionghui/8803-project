#ifndef FLY_COMMON_H_
#define FLY_COMMON_H_

extern UINT32 GetTickCount(void);
extern UINT32 forU8ToU32LSB(BYTE *p);
extern void forU32TopU8LSB(UINT32 data,BYTE *p);

#endif
