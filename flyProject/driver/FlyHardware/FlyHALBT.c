#include "FlyInclude.h"

void dealBTCommand(BYTE *buf, UINT len)
{
	switch (buf[0])
	{
	case MSG_BT_CON_POWER: 
		SOC_IO_Output(BT_POWER_GROUP,BT_POWER_GPIO,buf[1]);
		break;

	case MSG_BT_CON_RESET_ON: 
		SOC_IO_Output(BT_RESET_GROUP,BT_RESET_GPIO,0);
		break;

	case MSG_BT_CON_RESET_OFF: 
		SOC_IO_Output(BT_RESET_GROUP,BT_RESET_GPIO,1);
		break;

	case MSG_BT_CON_CE_NORMAL: 
		//SOC_IO_Output(BT_CE_GROUP,BT_CE_GPIO,0);
		DBG0("BT CE....>>>>>>> xxxxxxxxxxxxNot Used\n");
		break;

	case MSG_BT_CON_CE_UPDATE: 
		//SOC_IO_Output(BT_CE_GROUP,BT_CE_GPIO,0);
		DBG0("BT CE....>>>>>>> xxxxxxxxxxxxNot Used\n");
		break;

	default:
		break;
	}
}
