#include "FlyInclude.h"
//#include "../include/fly_soc_iic.h"

void dealExdisplayCommand(BYTE *buf, UINT len)
{
	//int i;
	BYTE p[256];
	if(len > 255)
	{
		return;
	}
	p[0] = 0x03;
	memcpy(&p[1] , buf ,len);
	LPCCombinDataStream(p, len + 1);
}