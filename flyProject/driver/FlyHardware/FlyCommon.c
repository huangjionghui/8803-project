#include "FlyInclude.h"

UINT32 GetTickCount(void)
{
	struct timespec t_now;
	do_posix_clock_monotonic_gettime(&t_now);
	monotonic_to_bootbased(&t_now);  
	return t_now.tv_sec * 1000 + t_now.tv_nsec / 1000000;
}

UINT32 forU8ToU32LSB(BYTE *p)
{
	UINT32 iTemp = 0;
	iTemp = (p[3] << 24) + (p[2] << 16) + (p[1] << 8) + p[0];
	return iTemp;
}

void forU32TopU8LSB(UINT32 data,BYTE *p)
{
	p[0] = data;
	data = data >> 8;p[1] = data;
	data = data >> 8;p[2] = data;
	data = data >> 8;p[3] = data;
}