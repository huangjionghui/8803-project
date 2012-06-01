#ifndef COMMONFUNC_H_
#define COMMONFUNC_H_

#define INFINITE 0xFFFFFFFF

extern void Sleep(UINT32 sTime);
extern ULONG GetTickCount(void);

extern void PostSignal(pthread_mutex_t *pMutex,pthread_cond_t *pCond,BOOL *bRunAgain);
extern int WaitSignedTimeOut(pthread_mutex_t *pMutex,pthread_cond_t *pCond,BOOL *bRunAgain,UINT32 iTimeOutMs);

#endif