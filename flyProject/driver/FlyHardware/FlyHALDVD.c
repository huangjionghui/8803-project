#include "FlyInclude.h"


void dealDVDCommand(BYTE *buf, UINT len)
{
	switch (buf[0])
	{
	case MSG_DVD_CON_RESET_ON:
		ioControlDVDReset(0);
		DBG0("DVD Reset%d\n",0);
		break;
	case MSG_DVD_CON_RESET_OFF:
		ioControlDVDReset(1);
		DBG0("DVD Reset%d\n",1);
		break;

	case MSG_DVD_CON_LIGHT:
		SOC_IO_Output(DVD_LEDC_GROUP,DVD_LEDC_GPIO,buf[1]);
		DBG0("DVD LED....>>>>>>> %d\n",buf[1]);
		break;

	default:
		break;
	}
}