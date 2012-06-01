#include "FlyInclude.h"

#define nCOLOR_L_N	((BYTE)(0x00))//0x00
#define nCOLOR_M_N	((BYTE)(0xE6))//0xE9
#define nCOLOR_H_N	((BYTE)(0xFF))//0xFF

#define nCONTRAST_L_N	((BYTE)(0x50))//0x64
#define nCONTRAST_M_N	((BYTE)(0x80))//((BYTE)(0x84))
#define nCONTRAST_H_N	((BYTE)(0xB0))//((BYTE)(0xA4))

#define nBRIGHTNESS_L_N	((BYTE)(0x48))//0x48
#define nBRIGHTNESS_M_N	((BYTE)(0x62))//((BYTE)(0x5E))
#define nBRIGHTNESS_H_N	((BYTE)(0x88))//0x88

#define nHUE_L_N	((BYTE)(0x30))
#define nHUE_M_N	((BYTE)(0x00))	
#define nHUE_H_N	((BYTE)(0xD0))

BYTE sColor_N[COLOR_STEP_COUNT] = 																											
{
#ifdef LDH
 nCOLOR_L_N
,nCOLOR_L_N+(nCOLOR_M_N-nCOLOR_L_N)/5*1,nCOLOR_L_N+(nCOLOR_M_N-nCOLOR_L_N)/5*2
,nCOLOR_L_N+(nCOLOR_M_N-nCOLOR_L_N)/5*3,nCOLOR_L_N+(nCOLOR_M_N-nCOLOR_L_N)/5*4
,nCOLOR_M_N
,nCOLOR_M_N+(nCOLOR_H_N-nCOLOR_M_N)/5*1,nCOLOR_M_N+(nCOLOR_H_N-nCOLOR_M_N)/5*2
,nCOLOR_M_N+(nCOLOR_H_N-nCOLOR_M_N)/5*3,nCOLOR_M_N+(nCOLOR_H_N-nCOLOR_M_N)/5*4
,nCOLOR_H_N
#else
 nCOLOR_M_N - 9,
 nCOLOR_M_N - 6,nCOLOR_M_N - 4,
 nCOLOR_M_N - 2,nCOLOR_M_N - 1,
 nCOLOR_M_N,
 nCOLOR_M_N + 1,
 nCOLOR_M_N + 2,nCOLOR_M_N + 4,
 nCOLOR_M_N + 6,nCOLOR_M_N + 9
#endif
};



BYTE sContrast_N[COLOR_STEP_COUNT]	= 																											
{
#ifdef LDH
 nCONTRAST_L_N
,nCONTRAST_L_N+(nCONTRAST_M_N-nCONTRAST_L_N)/5*1,nCONTRAST_L_N+(nCONTRAST_M_N-nCONTRAST_L_N)/5*2
,nCONTRAST_L_N+(nCONTRAST_M_N-nCONTRAST_L_N)/5*3,nCONTRAST_L_N+(nCONTRAST_M_N-nCONTRAST_L_N)/5*4
,nCONTRAST_M_N
,nCONTRAST_M_N+(nCONTRAST_H_N-nCONTRAST_M_N)/5*1,nCONTRAST_M_N+(nCONTRAST_H_N-nCONTRAST_M_N)/5*2
,nCONTRAST_M_N+(nCONTRAST_H_N-nCONTRAST_M_N)/5*3,nCONTRAST_M_N+(nCONTRAST_H_N-nCONTRAST_M_N)/5*4
,nCONTRAST_H_N
#else
 nCONTRAST_M_N - 9,
 nCONTRAST_M_N - 6,nCONTRAST_M_N - 4,
 nCONTRAST_M_N - 2,nCONTRAST_M_N - 1,
 nCONTRAST_M_N,
 nCONTRAST_M_N + 1,
 nCONTRAST_M_N + 2,nCONTRAST_M_N + 4,
 nCONTRAST_M_N + 6,nCONTRAST_M_N + 9

#endif
};

BYTE sBrightness_N[COLOR_STEP_COUNT]	= 
{
#ifdef LDH
nBRIGHTNESS_L_N
,nBRIGHTNESS_L_N+(nBRIGHTNESS_M_N-nBRIGHTNESS_L_N)/5*1,nBRIGHTNESS_L_N+(nBRIGHTNESS_M_N-nBRIGHTNESS_L_N)/5*2
,nBRIGHTNESS_L_N+(nBRIGHTNESS_M_N-nBRIGHTNESS_L_N)/5*3,nBRIGHTNESS_L_N+(nBRIGHTNESS_M_N-nBRIGHTNESS_L_N)/5*4
,nBRIGHTNESS_M_N
,nBRIGHTNESS_M_N+(nBRIGHTNESS_H_N-nBRIGHTNESS_M_N)/5*1,nBRIGHTNESS_M_N+(nBRIGHTNESS_H_N-nBRIGHTNESS_M_N)/5*2
,nBRIGHTNESS_M_N+(nBRIGHTNESS_H_N-nBRIGHTNESS_M_N)/5*3,nBRIGHTNESS_M_N+(nBRIGHTNESS_H_N-nBRIGHTNESS_M_N)/5*4
,nBRIGHTNESS_H_N
#else
nBRIGHTNESS_M_N - 9,
nBRIGHTNESS_M_N - 6,nBRIGHTNESS_M_N - 4,
nBRIGHTNESS_M_N - 2,nBRIGHTNESS_M_N - 1,
nBRIGHTNESS_M_N,
nBRIGHTNESS_M_N + 1,
nBRIGHTNESS_M_N + 2,nBRIGHTNESS_M_N + 4,
nBRIGHTNESS_M_N + 6,nBRIGHTNESS_M_N + 9
#endif
};


BYTE sHue_N[COLOR_STEP_COUNT]			= 
{ 
 #ifdef LDH
 nHUE_L_N
,nHUE_L_N-(nHUE_L_N-nHUE_M_N)/5*1,nHUE_L_N-(nHUE_L_N-nHUE_M_N)/5*2
,nHUE_L_N-(nHUE_L_N-nHUE_M_N)/5*3,nHUE_L_N-(nHUE_L_N-nHUE_M_N)/5*4
,nHUE_M_N
,nHUE_H_N+(0x100-nHUE_H_N)/5*4,nHUE_H_N+(0x100-nHUE_H_N)/5*3
,nHUE_H_N+(0x100-nHUE_H_N)/5*2,nHUE_H_N+(0x100-nHUE_H_N)/5*1
,nHUE_H_N
#else
nHUE_M_N - 9,
nHUE_M_N - 6,nHUE_M_N - 4,
nHUE_M_N - 2,nHUE_M_N - 1,
nHUE_M_N,
nHUE_M_N + 1,
nHUE_M_N + 2,nHUE_M_N + 4,
nHUE_M_N + 6,nHUE_M_N + 9

#endif
};
 


BYTE ADC_CVBS_INIT[] =
{
	0x00, 0x02,//CHANNEL
	0x03, 0x2D,
	//0x08, 0x04,
	0x0a, 0x5e,//color
	0x0c, 0x5a,//contrast
	0x09, 0x8f,//brightness
	//0x0f, 0x02,
	0xFF		// end flag

};

void videoIICWriteInitPara(void)
{
	BYTE *p = ADC_CVBS_INIT;
	while (0xFF != *p)
	{
		actualIICWrite(I2_1_ID,CAM_IIC_ADD_W,p,2);
		p += 2;
	}

	printk("\nvideo Write Init Data");
}

void tvp5150RegRewrite(void)
{
	int i;
	BYTE iicPara[] = {0x00,0x00};

	for (i = 0;i < 256;i++)
	{
		if (pGlobalHardwareInfo->tvp5150RegIsReWrite[i])
		{
			iicPara[0] = i;
			iicPara[1] = pGlobalHardwareInfo->tvp5150RegReWriteData[i];

			actualIICWrite(I2_1_ID,CAM_IIC_ADD_W,iicPara,2);
		}
	}
}

void videoIICChangeInput(BYTE iChannel)
{
	BYTE iicPara[] = {0x00,0x00};

	if(1) //(MediaCD == iChannel)
	{
		iicPara[0] = 0x00;iicPara[1] = 0x02;
	}
	else
	{
		iicPara[0] = 0x00;iicPara[1] = 0x00;
	}
	actualIICWrite(I2_1_ID,CAM_IIC_ADD_W,iicPara,2);
	pGlobalHardwareInfo->tvp5150RegIsReWrite[iicPara[0]] = TRUE;
	pGlobalHardwareInfo->tvp5150RegReWriteData[iicPara[0]] = iicPara[1];

	printk("\nvideo Write Change Input %d",iChannel);
}

void videoIICChangeColorful(BYTE iColor,BYTE iHue,BYTE iContrast,BYTE iBrightness)
{
	BYTE iicPara[] = {0x00,0x00};

	iicPara[0] = 0x0A;iicPara[1] = sColor_N[iColor];
	actualIICWrite(I2_1_ID,CAM_IIC_ADD_W,iicPara,2);
	pGlobalHardwareInfo->tvp5150RegIsReWrite[iicPara[0]] = TRUE;
	pGlobalHardwareInfo->tvp5150RegReWriteData[iicPara[0]] = iicPara[1];

	iicPara[0] = 0x0B;iicPara[1] = sHue_N[iHue];
	actualIICWrite(I2_1_ID,CAM_IIC_ADD_W,iicPara,2);
	pGlobalHardwareInfo->tvp5150RegIsReWrite[iicPara[0]] = TRUE;
	pGlobalHardwareInfo->tvp5150RegReWriteData[iicPara[0]] = iicPara[1];

	iicPara[0] = 0x0C;iicPara[1] = sContrast_N[iContrast];
	actualIICWrite(I2_1_ID,CAM_IIC_ADD_W,iicPara,2);
	pGlobalHardwareInfo->tvp5150RegIsReWrite[iicPara[0]] = TRUE;
	pGlobalHardwareInfo->tvp5150RegReWriteData[iicPara[0]] = iicPara[1];

	iicPara[0] = 0x09;iicPara[1] = sBrightness_N[iBrightness];
	actualIICWrite(I2_1_ID,CAM_IIC_ADD_W,iicPara,2);
	pGlobalHardwareInfo->tvp5150RegIsReWrite[iicPara[0]] = TRUE;
	pGlobalHardwareInfo->tvp5150RegReWriteData[iicPara[0]] = iicPara[1];

	printk("\nvideo Write Change Color %d %d %d %d",iColor,iHue,iContrast,iBrightness);
	printk("\n2012-3-1 15:37");
	printk("\n2012-3-1 video Write Change Color sColor_N: %d sHue_N: %d sContrast_N: %d sBrightness_N: %d",sColor_N[iColor],sHue_N[iHue],sContrast_N[iContrast],sBrightness_N[iBrightness]);
}

BOOL videoIICbHaveVideo(void)
{
	BYTE iRegAdd,iRegValue;
	iRegAdd = 0x88;

	actualIICRead(I2_1_ID,CAM_IIC_ADD_R,iRegAdd,&iRegValue,1);

	if(
		((iRegValue&(0x01 << 4))) ||
		((iRegValue&(0x01 << 3)) == 0) ||
		((iRegValue&(0x01 << 2)) == 0) ||
		((iRegValue&(0x01 << 1)) == 0)
		)
		return 0;
	else
	{

		//检测到视频


		if ((iRegValue&(0x01 << 5)))
		{
			pGlobalHardwareInfo->camPreVideoPal = TRUE;
		}
		else
		{
			pGlobalHardwareInfo->camPreVideoPal = FALSE;
		}

		//if((reg_value&(0x01 << 5)))
		//{
		//	RETAILMSG(0, (TEXT("PAL 50Hz\r\n")));
		//	vin_type = PAL;
		//}
		//else
		//{
		//	RETAILMSG(0, (TEXT("NTSC 60Hz\r\n")));
		//	vin_type = NTSC;
		//}

		return 1;

	}
}

void TVP5150Init(BOOL bInitAll)
{
	//参数初始化
	if (bInitAll)
	{
		pGlobalHardwareInfo->camCurVideoChannel = MediaCD;
		pGlobalHardwareInfo->camParaColor = COLOR_STEP_COUNT/2;
		pGlobalHardwareInfo->camParaHue = COLOR_STEP_COUNT/2;
		pGlobalHardwareInfo->camParaContrast = COLOR_STEP_COUNT/2;
		pGlobalHardwareInfo->camParaBrightness = COLOR_STEP_COUNT/2;
	}

	SOC_IO_Output(CAM_POWER_G,CAM_POWER_I,1);//上电

	msleep(100);
	SOC_IO_Output(CAM_RESET_G,CAM_RESET_I,0);//复位
	msleep(10);
	SOC_IO_Output(CAM_RESET_G,CAM_RESET_I,1);
	msleep(10);

	videoIICWriteInitPara();
	videoIICChangeInput(pGlobalHardwareInfo->camCurVideoChannel);
	videoIICChangeColorful(pGlobalHardwareInfo->camParaColor
		,pGlobalHardwareInfo->camParaHue
		,pGlobalHardwareInfo->camParaContrast
		,pGlobalHardwareInfo->camParaBrightness);

	tvp5150RegRewrite();

	printk("\nvideo Init %d",bInitAll);
}

ULONG iTVP5150LastCheckTime;

void TVP5150StatusCheck(void)
{
	BYTE iRegAdd,iRegValue;

	return;

	if (GetTickCount() - iTVP5150LastCheckTime > 1000)
	{
		iTVP5150LastCheckTime = GetTickCount();

		iRegAdd = 0x03;
		actualIICRead(I2_1_ID,CAM_IIC_ADD_R,iRegAdd,&iRegValue,1);
		if ((iRegValue | (0x01 << 2)) == 0)
		{
			printk("0x03Miscellaneous Control %x\n", iRegValue);
			goto reset5150;
		}

		iRegAdd = 0x08;
		actualIICRead(I2_1_ID,CAM_IIC_ADD_R,iRegAdd,&iRegValue,1);
		if (iRegValue != 0x04)
		{
			printk("0x08Luminance Processing %x\n", iRegValue);
			goto reset5150;
		}

		iRegAdd = 0x0f;
		actualIICRead(I2_1_ID,CAM_IIC_ADD_R,iRegAdd,&iRegValue,1);
		if (iRegValue != 0x00)
		{
			printk("0x0fConfiguration Shared %x\n", iRegValue);
			goto reset5150;
		}
	}

	return;

reset5150:
	printk("TVP5150 Check Error,Reset...\n");
	TVP5150Init(FALSE);
}

UINT noBlockVideoMessage(BYTE *pData,UINT length)
{
	UINT returnCount = 0;
	if (MSG_VIDEO_REQ_HAVE_VIDEO == pData[0])
	{
		if (videoIICbHaveVideo())
		{
			if (pGlobalHardwareInfo->camPreVideoPal)
			{
				pData[2] = 0x01;
			}
			else
			{
				pData[2] = 0x00;
			}
			pGlobalHardwareInfo->camCurVideoPal = pGlobalHardwareInfo->camPreVideoPal;

			pData[1] = 0x01;
		}
		else
		{
			pData[1] = 0x00;
		}

		return 3;
	}

	return returnCount;
}

void dealVideoCommand(BYTE *buf, UINT len)
{
	printk("video comm:%x\n",buf[0]);
	switch (buf[0])
	{
	case MSG_VIDEO_INIT:
		TVP5150Init(FALSE);
		break;
	case MSG_VIDEO_CON_INPUT:
		pGlobalHardwareInfo->camCurVideoChannel = buf[1];
		videoIICChangeInput(pGlobalHardwareInfo->camCurVideoChannel);
		switchMainVideoInput(pGlobalHardwareInfo->camCurVideoChannel);
		if (BACK != pGlobalHardwareInfo->camCurVideoChannel)
		{
			switchSecondVideoInput(pGlobalHardwareInfo->camCurVideoChannel);	
		}
		break;
	case MSG_VIDEO_CON_COLORFUL:
		pGlobalHardwareInfo->camParaColor = buf[1];
		pGlobalHardwareInfo->camParaHue = buf[2];
		pGlobalHardwareInfo->camParaContrast = buf[3];
		pGlobalHardwareInfo->camParaBrightness = buf[4];
		videoIICChangeColorful(pGlobalHardwareInfo->camParaColor
			,pGlobalHardwareInfo->camParaHue
			,pGlobalHardwareInfo->camParaContrast
			,pGlobalHardwareInfo->camParaBrightness);
		break;
	default:
		break;
	}
}

void halVideoPowerOn(void)
{
	TVP5150Init(FALSE);
}

void halVideoFirstInit(void)
{
	TVP5150Init(TRUE);
}

