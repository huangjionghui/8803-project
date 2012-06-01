#include "FlyInclude.h"

void write15887ToIIC(void)
{
	UINT i=0;
	BYTE buff[5];
	buff[0] = 0x00;
	memcpy(&buff[1],pGlobalHardwareInfo->sFlyAN15887Info.regData,4);
	
	printk("\n15887 data:");
	for (i=0; i<5; i++)
		printk("%02X ", buff[i]);
	printk("\n");

	actualIICWrite(I2_1_ID,AN15887_ADD_W,buff,5);
}

//AL	1	DVD	2	MP3	3	CDC	4	AUX	 5
void switchMainAudioInput(BYTE iInput)//A_IN2
{
	pGlobalHardwareInfo->sFlyAN15887Info.mainAudioInput = iInput;

	if (MediaMP3 == iInput || BT_RING == iInput)
	{
		SOC_IO_Output(AUDIO_MP3_BT_4052_G,AUDIO_MP3_BT_4052_I,0);
	}
	else if (BT == iInput || A2DP == iInput)
	{
		SOC_IO_Output(AUDIO_MP3_BT_4052_G,AUDIO_MP3_BT_4052_I,1);
	}

#if PCB_8803_DISP_SEL == PCB_8803_DISP_V2
	if (BT == iInput || A2DP == iInput)
	{
		iInput = MediaMP3;
	}
#endif

#if PCB_8803_AMP_SEL == PCB_8803_AMP_V1
	switch (iInput)
	{
	case MediaCD:
		pGlobalHardwareInfo->sFlyAN15887Info.regData[1] &= ~0x07;//切换通道	
		pGlobalHardwareInfo->sFlyAN15887Info.regData[1] |= 2;		
		break;
	case MediaMP3:
	case IPOD:	
	case BT_RING:
		pGlobalHardwareInfo->sFlyAN15887Info.regData[1] &= ~0x07;//切换通道	
		pGlobalHardwareInfo->sFlyAN15887Info.regData[1] |= 3;		
		break;
	case CDC:
	case TV:	
		pGlobalHardwareInfo->sFlyAN15887Info.regData[1] &= ~0x07;//切换通道	
		pGlobalHardwareInfo->sFlyAN15887Info.regData[1] |= 4;		
		break;
	case AUX:
		pGlobalHardwareInfo->sFlyAN15887Info.regData[1] &= ~0x07;//切换通道	
		pGlobalHardwareInfo->sFlyAN15887Info.regData[1] |= 5;		
		break;

	default:
		break;
	}
#endif

#if PCB_8803_AMP_SEL == PCB_8803_AMP_V2
	switch (iInput)
	{
	case MediaCD:
		pGlobalHardwareInfo->sFlyAN15887Info.regData[1] &= ~0x07;//切换通道	
		pGlobalHardwareInfo->sFlyAN15887Info.regData[1] |= 1;		
		break;
	case CDC:
	case TV:
		pGlobalHardwareInfo->sFlyAN15887Info.regData[1] &= ~0x07;//切换通道	
		pGlobalHardwareInfo->sFlyAN15887Info.regData[1] |= 3;		
		break;
	case AUX:
		pGlobalHardwareInfo->sFlyAN15887Info.regData[1] &= ~0x07;//切换通道	
		pGlobalHardwareInfo->sFlyAN15887Info.regData[1] |= 4;		
		break;
	case MediaMP3:
	case IPOD:
	case BT_RING:
	case BT:
	case A2DP:
		pGlobalHardwareInfo->sFlyAN15887Info.regData[1] &= ~0x07;//切换通道	
		pGlobalHardwareInfo->sFlyAN15887Info.regData[1] |= 5;		
		break;
	default:
		break;
	}
#endif

#if PCB_8803_AMP_SEL == PCB_8803_AMP_V3
	switch (iInput)
	{
	case MediaCD:
		pGlobalHardwareInfo->sFlyAN15887Info.regData[1] &= ~0x07;//切换通道	
		pGlobalHardwareInfo->sFlyAN15887Info.regData[1] |= 2;		
		break;
	case MediaMP3:
	case IPOD:	
	case BT_RING:
		pGlobalHardwareInfo->sFlyAN15887Info.regData[1] &= ~0x07;//切换通道	
		pGlobalHardwareInfo->sFlyAN15887Info.regData[1] |= 3;		
		break;
	case CDC:
	case TV:	
		pGlobalHardwareInfo->sFlyAN15887Info.regData[1] &= ~0x07;//切换通道	
		pGlobalHardwareInfo->sFlyAN15887Info.regData[1] |= 4;		
		break;
	case AUX:
		pGlobalHardwareInfo->sFlyAN15887Info.regData[1] &= ~0x07;//切换通道	
		pGlobalHardwareInfo->sFlyAN15887Info.regData[1] |= 5;		
		break;

	default:
		break;
	}
#endif

	write15887ToIIC();
}
void switchSecondAudioInput(BYTE iInput)//A_IN1
{
	pGlobalHardwareInfo->sFlyAN15887Info.secondAudioInput = iInput;
#if PCB_8803_AMP_SEL == PCB_8803_AMP_V1
	switch (iInput)
	{
	case MediaCD:
		pGlobalHardwareInfo->sFlyAN15887Info.regData[0] &= ~0x07;//切换通道	
		pGlobalHardwareInfo->sFlyAN15887Info.regData[0] |= 2;		
		break;
	case MediaMP3:
	case IPOD:
		pGlobalHardwareInfo->sFlyAN15887Info.regData[0] &= ~0x07;//切换通道	
		pGlobalHardwareInfo->sFlyAN15887Info.regData[0] |= 3;		
		break;
	case CDC:
	case TV:		
		pGlobalHardwareInfo->sFlyAN15887Info.regData[0] &= ~0x07;//切换通道	
		pGlobalHardwareInfo->sFlyAN15887Info.regData[0] |= 4;		
		break;
	case AUX:
		pGlobalHardwareInfo->sFlyAN15887Info.regData[0] &= ~0x07;//切换通道	
		pGlobalHardwareInfo->sFlyAN15887Info.regData[0] |= 5;		
		break;
	default:
		break;
	}
#endif

#if PCB_8803_AMP_SEL == PCB_8803_AMP_V2
	switch (iInput)
	{
	case MediaCD:
		pGlobalHardwareInfo->sFlyAN15887Info.regData[0] &= ~0x07;//切换通道	
		pGlobalHardwareInfo->sFlyAN15887Info.regData[0] |= 1;		
		break;
	case CDC:
	case TV:
		pGlobalHardwareInfo->sFlyAN15887Info.regData[0] &= ~0x07;//切换通道	
		pGlobalHardwareInfo->sFlyAN15887Info.regData[0] |= 3;		
		break;
	case AUX:
		pGlobalHardwareInfo->sFlyAN15887Info.regData[0] &= ~0x07;//切换通道	
		pGlobalHardwareInfo->sFlyAN15887Info.regData[0] |= 4;		
		break;
	case MediaMP3:
	case IPOD:
	case BT:
	case A2DP:
		pGlobalHardwareInfo->sFlyAN15887Info.regData[0] &= ~0x07;//切换通道	
		pGlobalHardwareInfo->sFlyAN15887Info.regData[0] |= 5;		
		break;
	default:
		break;
	}
#endif

#if PCB_8803_AMP_SEL == PCB_8803_AMP_V3
	switch (iInput)
	{
	case MediaCD:
		pGlobalHardwareInfo->sFlyAN15887Info.regData[0] &= ~0x07;//切换通道	
		pGlobalHardwareInfo->sFlyAN15887Info.regData[0] |= 2;		
		break;
	case MediaMP3:
	case IPOD:
		pGlobalHardwareInfo->sFlyAN15887Info.regData[0] &= ~0x07;//切换通道	
		pGlobalHardwareInfo->sFlyAN15887Info.regData[0] |= 3;		
		break;
	case CDC:
	case TV:		
		pGlobalHardwareInfo->sFlyAN15887Info.regData[0] &= ~0x07;//切换通道	
		pGlobalHardwareInfo->sFlyAN15887Info.regData[0] |= 4;		
		break;
	case AUX:
		pGlobalHardwareInfo->sFlyAN15887Info.regData[0] &= ~0x07;//切换通道	
		pGlobalHardwareInfo->sFlyAN15887Info.regData[0] |= 5;		
		break;
	default:
		break;
	}
#endif

	write15887ToIIC();
}

//CVBS_F	1	AUX	2	CDC	3	DVD	4	AV	5	BACK	6
void switchMainVideoInput(BYTE iInput)//V_IN1
{
	pGlobalHardwareInfo->sFlyAN15887Info.mainVideoInput = iInput;

	if (BACK == iInput)
	{
		SOC_IO_Output(VIDEO_IPOD_AUX_BACK_4052_G,VIDEO_IPOD_AUX_BACK_4052_I,0);
	}
	else if (AUX == iInput || IPOD == iInput)
	{
		SOC_IO_Output(VIDEO_IPOD_AUX_BACK_4052_G,VIDEO_IPOD_AUX_BACK_4052_I,1);
	}

#if PCB_8803_AMP_SEL == PCB_8803_AMP_V1
	switch (iInput)
	{
	case AUX:	
	case IPOD:
		pGlobalHardwareInfo->sFlyAN15887Info.regData[0] &= ~(0x07<<3);
		pGlobalHardwareInfo->sFlyAN15887Info.regData[0] |= 2<<3;
		break;
	case CDC:
	case TV:
		pGlobalHardwareInfo->sFlyAN15887Info.regData[0] &= ~(0x07<<3);
		pGlobalHardwareInfo->sFlyAN15887Info.regData[0] |= 3<<3;
		break;
	case MediaCD:
		pGlobalHardwareInfo->sFlyAN15887Info.regData[0] &= ~(0x07<<3);
		pGlobalHardwareInfo->sFlyAN15887Info.regData[0] |= 4<<3;
		break;
	case BACK:
		pGlobalHardwareInfo->sFlyAN15887Info.regData[0] &= ~(0x07<<3);
		pGlobalHardwareInfo->sFlyAN15887Info.regData[0] |= 6<<3;
		break;
	default:
		break;
	}
#endif

#if PCB_8803_AMP_SEL == PCB_8803_AMP_V2
	switch (iInput)
	{
	case AUX:
	case IPOD:	
		pGlobalHardwareInfo->sFlyAN15887Info.regData[0] &= ~(0x07<<3);
		pGlobalHardwareInfo->sFlyAN15887Info.regData[0] |= 2<<3;
		break;
	case CDC:
	case TV:
		pGlobalHardwareInfo->sFlyAN15887Info.regData[0] &= ~(0x07<<3);
		pGlobalHardwareInfo->sFlyAN15887Info.regData[0] |= 5<<3;
		break;
	case MediaCD:
		pGlobalHardwareInfo->sFlyAN15887Info.regData[0] &= ~(0x07<<3);
		pGlobalHardwareInfo->sFlyAN15887Info.regData[0] |= 6<<3;
		break;
	case BACK:
		pGlobalHardwareInfo->sFlyAN15887Info.regData[0] &= ~(0x07<<3);
		pGlobalHardwareInfo->sFlyAN15887Info.regData[0] |= 4<<3;
		break;
	default:
		break;
	}
#endif

#if PCB_8803_AMP_SEL == PCB_8803_AMP_V3
	switch (iInput)
	{
	case AUX:	
	case IPOD:
		pGlobalHardwareInfo->sFlyAN15887Info.regData[0] &= ~(0x07<<3);
		pGlobalHardwareInfo->sFlyAN15887Info.regData[0] |= 1<<3;
		break;
	case CDC:
	case TV:
		pGlobalHardwareInfo->sFlyAN15887Info.regData[0] &= ~(0x07<<3);
		pGlobalHardwareInfo->sFlyAN15887Info.regData[0] |= 3<<3;
		break;
	case MediaCD:
		pGlobalHardwareInfo->sFlyAN15887Info.regData[0] &= ~(0x07<<3);
		pGlobalHardwareInfo->sFlyAN15887Info.regData[0] |= 4<<3;
		break;
	case BACK:
		pGlobalHardwareInfo->sFlyAN15887Info.regData[0] &= ~(0x07<<3);
		pGlobalHardwareInfo->sFlyAN15887Info.regData[0] |= 2<<3;
		break;
	default:
		break;
	}
#endif

	pGlobalHardwareInfo->sFlyAN15887Info.regData[3] |= (1 << 5);
	pGlobalHardwareInfo->sFlyAN15887Info.regData[3] |= (1 << 7);
	write15887ToIIC();
}

void switchSecondVideoInput(BYTE iInput)//V_IN2
{
	pGlobalHardwareInfo->sFlyAN15887Info.secondVideoInput = iInput;
#if PCB_8803_AMP_SEL == PCB_8803_AMP_V1
	switch (iInput)
	{
	case AUX:
	case IPOD:
		pGlobalHardwareInfo->sFlyAN15887Info.regData[1] &= ~(0x07<<3);
		pGlobalHardwareInfo->sFlyAN15887Info.regData[1] |= 2<<3;
		break;
	case TV:
	case CDC:
		pGlobalHardwareInfo->sFlyAN15887Info.regData[1] &= ~(0x07<<3);
		pGlobalHardwareInfo->sFlyAN15887Info.regData[1] |= 3<<3;
		break;
	case MediaCD:
		pGlobalHardwareInfo->sFlyAN15887Info.regData[1] &= ~(0x07<<3);
		pGlobalHardwareInfo->sFlyAN15887Info.regData[1] |= 4<<3;
		break;
	case BACK:
		pGlobalHardwareInfo->sFlyAN15887Info.regData[1] &= ~(0x07<<3);
		pGlobalHardwareInfo->sFlyAN15887Info.regData[1] |= 6<<3;
		break;
	}
#endif

#if PCB_8803_AMP_SEL == PCB_8803_AMP_V2
	switch (iInput)
	{
	case AUX:
	case IPOD:	
		pGlobalHardwareInfo->sFlyAN15887Info.regData[1] &= ~(0x07<<3);
		pGlobalHardwareInfo->sFlyAN15887Info.regData[1] |= 2<<3;
		break;
	case CDC:
	case TV:
		pGlobalHardwareInfo->sFlyAN15887Info.regData[1] &= ~(0x07<<3);
		pGlobalHardwareInfo->sFlyAN15887Info.regData[1] |= 5<<3;
		break;
	case MediaCD:
		pGlobalHardwareInfo->sFlyAN15887Info.regData[1] &= ~(0x07<<3);
		pGlobalHardwareInfo->sFlyAN15887Info.regData[1] |= 6<<3;
		break;
	case BACK:
		pGlobalHardwareInfo->sFlyAN15887Info.regData[1] &= ~(0x07<<3);
		pGlobalHardwareInfo->sFlyAN15887Info.regData[1] |= 4<<3;
		break;
	default:
		break;
	}
#endif

#if PCB_8803_AMP_SEL == PCB_8803_AMP_V3
	switch (iInput)
	{
	case AUX:	
	case IPOD:
		pGlobalHardwareInfo->sFlyAN15887Info.regData[1] &= ~(0x07<<3);
		pGlobalHardwareInfo->sFlyAN15887Info.regData[1] |= 1<<3;
		break;
	case CDC:
	case TV:
		pGlobalHardwareInfo->sFlyAN15887Info.regData[1] &= ~(0x07<<3);
		pGlobalHardwareInfo->sFlyAN15887Info.regData[1] |= 3<<3;
		break;
	case MediaCD:
		pGlobalHardwareInfo->sFlyAN15887Info.regData[1] &= ~(0x07<<3);
		pGlobalHardwareInfo->sFlyAN15887Info.regData[1] |= 4<<3;
		break;
	case BACK:
		pGlobalHardwareInfo->sFlyAN15887Info.regData[1] &= ~(0x07<<3);
		pGlobalHardwareInfo->sFlyAN15887Info.regData[1] |= 2<<3;
		break;
	default:
		break;
	}
#endif

	write15887ToIIC();
}

void AN15887EnableISOLATEVideo(BYTE iChannel)
{
	if (1 == iChannel)
	{
		pGlobalHardwareInfo->sFlyAN15887Info.regData[0] |= 1<<6;
	}
	else if (2 == iChannel)
	{
		pGlobalHardwareInfo->sFlyAN15887Info.regData[1] |= 1<<6;
	}
	else if (3 == iChannel)
	{
		pGlobalHardwareInfo->sFlyAN15887Info.regData[1] |= 1<<7;
	}
	else if (4 == iChannel)
	{
		pGlobalHardwareInfo->sFlyAN15887Info.regData[2] |= 1<<0;
	}
	else if (5 == iChannel)
	{
		pGlobalHardwareInfo->sFlyAN15887Info.regData[2] |= 1<<1;
	}
	else if (6 == iChannel)
	{
		pGlobalHardwareInfo->sFlyAN15887Info.regData[2] |= 1<<2;
	}
}

void AN15887EnableISOLATEAudio(BYTE iChannel)
{
	if (1 == iChannel)
	{
		pGlobalHardwareInfo->sFlyAN15887Info.regData[2] |= 1<<3;
	}
	else if (2 == iChannel)
	{
		pGlobalHardwareInfo->sFlyAN15887Info.regData[2] |= 1<<4;
	}
	else if (3 == iChannel)
	{
		pGlobalHardwareInfo->sFlyAN15887Info.regData[2] |= 1<<5;
	}
	else if (4 == iChannel)
	{
		pGlobalHardwareInfo->sFlyAN15887Info.regData[2] |= 1<<6;
	}
	else if (5 == iChannel)
	{
		pGlobalHardwareInfo->sFlyAN15887Info.regData[2] |= 1<<7;
	}
}

void AN15887Init(BOOL bInitAll)
{
	if (bInitAll)
	{
		memset(pGlobalHardwareInfo->sFlyAN15887Info.regData,0,4);

		AN15887EnableISOLATEVideo(1);//AUX
		AN15887EnableISOLATEVideo(2);//BACK_VF
		AN15887EnableISOLATEVideo(3);//CDC
		AN15887EnableISOLATEVideo(4);//BACKV
		AN15887EnableISOLATEVideo(5);
		AN15887EnableISOLATEVideo(6);

		AN15887EnableISOLATEAudio(1);
		AN15887EnableISOLATEAudio(2);
		AN15887EnableISOLATEAudio(3);
		AN15887EnableISOLATEAudio(4);//CDC
		AN15887EnableISOLATEAudio(5);//AUX
		
		if (pGlobalHardwareInfo->sFlyAN15887Info.mainAudioInput)
		{
			switchMainAudioInput(pGlobalHardwareInfo->sFlyAN15887Info.mainAudioInput);
		}
		else
		{
			switchMainAudioInput(MediaCD);
		}

		if (pGlobalHardwareInfo->sFlyAN15887Info.secondAudioInput)
		{
			switchSecondAudioInput(pGlobalHardwareInfo->sFlyAN15887Info.secondAudioInput);
		}
		else
		{
			switchSecondAudioInput(MediaCD);
		}

		if (pGlobalHardwareInfo->sFlyAN15887Info.mainVideoInput)
		{
			switchMainVideoInput(pGlobalHardwareInfo->sFlyAN15887Info.mainVideoInput);
		}
		else
		{
			switchMainVideoInput(MediaCD);
		}

		if (pGlobalHardwareInfo->sFlyAN15887Info.secondVideoInput)
		{
			switchSecondVideoInput(pGlobalHardwareInfo->sFlyAN15887Info.secondVideoInput);
		}
		else
		{
			switchSecondVideoInput(MediaCD);
		}
	}
	write15887ToIIC();
}

void dealAudioCommand(BYTE *buf, UINT len)
{
	switch (buf[0])
	{
		default:
			break;
	}
}

void halAudioPowerOn(void)
{
	AN15887Init(FALSE);
}

void halAudioFirstInit(void)
{
	AN15887Init(TRUE);
}
