#include <fcntl.h>  
#include <errno.h>  
#include <termios.h>
#include <stdio.h>
#include <pthread.h>
#include <assert.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/select.h>
#include <sys/types.h> 
#include <cutils/atomic.h>
#include <hardware/hardware.h>  
#include <unistd.h>

#include "commonFunc.h"

void Sleep(UINT32 sTime)
{
	usleep(sTime*1000);
}

ULONG GetTickCount(void)
{
/*	struct timespec usr_timer;
	//clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&usr_timer);
	clock_gettime(CLOCK_REALTIME,&usr_timer);

	return usr_timer.tv_sec*1000 + usr_timer.tv_nsec / 1000000;
*/
	return pFlyAllInOneInfo->pMemory_Share_Common->iOSRunTime;
}

void PostSignal(pthread_mutex_t *pMutex,pthread_cond_t *pCond,BOOL *pbRunAgain)
{
	pthread_mutex_lock(pMutex);
	*pbRunAgain = TRUE;
	pthread_cond_signal(pCond);
	pthread_mutex_unlock(pMutex);
}

int WaitSignedTimeOut(pthread_mutex_t *pMutex,pthread_cond_t *pCond,BOOL *pbRunAgain,UINT32 iTimeOutMs)
{
	int ret = 0;
	struct timeval timenow;
	struct timespec timeout;
	UINT32 iSecnod,iMSecnod;

	pthread_mutex_lock(pMutex);

	if (*pbRunAgain)
	{
	}
	else if (INFINITE == iTimeOutMs || 0 == iTimeOutMs)
	{
		pthread_cond_wait(pCond,pMutex);
	}
	else
	{
		gettimeofday(&timenow, NULL);

		iSecnod = iTimeOutMs / 1000;
		iMSecnod = iTimeOutMs % 1000;

		timeout.tv_sec = timenow.tv_sec + iSecnod;
		timeout.tv_nsec = (timenow.tv_usec + iMSecnod*1000)*1000;

		while (timeout.tv_nsec >= 1000000000)
		{
			timeout.tv_nsec -= 1000000000;
			timeout.tv_sec++;
		}

		ret = pthread_cond_timedwait(pCond,pMutex,&timeout); 
	}

	*pbRunAgain = FALSE;

	pthread_mutex_unlock(pMutex);

	return ret;
}
