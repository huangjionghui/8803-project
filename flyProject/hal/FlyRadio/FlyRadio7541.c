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

#include "../../include/ShareMemoryStruct.h"
#if AUDIO_RADIO_CHIP_SEL == AUDIO_RADIO_7419_7541

#define LOCAL_HAL_ID		HAL_DEVICE_NAME_RADIO
#define LOCAL_HAL_NAME		"flyradio Stub"
#define LOCAL_HAL_AUTOHR	"Flyradio"
#define CURRENT_SHARE_MEMORY_ID	SHARE_MEMORY_RADIO

#include "FlyRadio7541.h"
#include "../../include/allInOneOthers.c"
#include "../../include/commonFunc.c"
#include "../../include/HalApi.c"

struct flyradio_struct_info *pFlyRadioInfo = NULL;

/******************************************************************************/
/*                                  各种通信ID                               */
/******************************************************************************/

static void RegDataWriteRadio(void);
void readFromhardwareProc(BYTE *buf,UINT length)
{
	UINT16 temp;
	DBG0(debugBuf("\nTDA7541 RadioHAL read from hardware---->",buf,length);)
		if (SHARE_MEMORY_RADIO == buf[0])
		{
			//if (MSG_RADIO_CON_AD_GET == buf[1])
			//{
			//	temp = buf[2];
			//	temp = (temp<<8) | buf[3];
			//	pFlyRadioInfo->RadioAD = 0;
			//	pFlyRadioInfo->RadioAD = temp;

			//	pFlyRadioInfo->bRadioADReturn = TRUE;
			//}
			if (MSG_RADIO_TDA7541_RDS_ID == buf[1])
			{
				memcpy(&pFlyRadioInfo->rdsdec_buf, buf, length);
				PostSignal(&pFlyRadioInfo->RDSRecThreadMutex,&pFlyRadioInfo->RDSRecThreadCond,&pFlyRadioInfo->bRDSThreadRunAgain);
			}

		}
}

void ipcEventProcProc(UINT32 sourceEvent)
{
	DBG0(debugOneData("\nFlyRadio SAF7741 HAL IPC Read ID---->",sourceEvent);)
		switch (sourceEvent)
	{
		case EVENT_AUTO_CLR_RESUME_ID:
			pFlyRadioInfo->bNeedInit = TRUE;
			break;
		default:
			break;
	}

	sem_post(&pFlyRadioInfo->MainThread_sem);//激活一次
}

/******************************************************************************/
/*                                 各种IO操作                             */
/******************************************************************************/

static void control_radio_ant(BYTE ant_id, BOOL bOn)
{
	BYTE buff[3];
	buff[0] = CURRENT_SHARE_MEMORY_ID;
	buff[1] = ant_id;
	buff[2] = bOn;
	writeDataToHardware(buff, 3);

	DBG0(debugString("\nFlyRadio SAF7541 ANT Control");)
}

static void control_tda7541_AFMute(BOOL bOn)
{
	BYTE buff[3];
	buff[0] = CURRENT_SHARE_MEMORY_ID;
	buff[1] = MSG_RADIO_TDA7541_AFMUTE;
	buff[2] = bOn;
	writeDataToHardware(buff, 3);

	//DBG0(debugString("\nTDA7541 Radio AFMute Control TDA7541");)
}

static void radioANTControl(BOOL bOn)
{
	if (bOn)
	{
		control_radio_ant(MSG_RADIO_CON_ANT1,1);
		control_radio_ant(MSG_RADIO_CON_ANT2,1);
	}
	else
	{
		control_radio_ant(MSG_RADIO_CON_ANT1,0);
		control_radio_ant(MSG_RADIO_CON_ANT2,0);
	}
}

static void radioAFMuteControl(BOOL bOn)
{
	if (bOn)
	{
		control_tda7541_AFMute(1);
	}
	else
	{
		control_tda7541_AFMute(0);
	}
}

static BOOL READ_IO_SSTOP_IN(void)
{
	BYTE buff[4];
	buff[0] = CURRENT_SHARE_MEMORY_ID;
	buff[1] = S_NO_BLOCK_ID;
	buff[2] = MSG_RADIO_REQ_TDA7541_SSTOP_ID;
	buff[3] = 0x00;

	readDataFromHardwareNoBlock(buff,4);

	if (buff[3])
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

/******************************************************************************/
/*                                各种I2C操作                             */
/******************************************************************************/
BOOL I2C_Write_Tda7541(BYTE ulRegAddr, BYTE *pRegValBuf, UINT uiValBufLen)
{
	BYTE buff[256];
	buff[0] = CURRENT_SHARE_MEMORY_ID;
	buff[1] = MSG_RADIO_CON_TDA7541_ID;
	buff[2] = ulRegAddr;
	memcpy(&buff[3],pRegValBuf,uiValBufLen);
	writeDataToHardware(buff, uiValBufLen+3);

	//DBG2(debugBuf("\nTDA7541 Radio IIC Write---->",buff,uiValBufLen+3);)
	return TRUE;
}

BOOL I2C_Read_EEPROM(BYTE ulRegAddr, BYTE *pRegValBuf, UINT uiValBufLen)
{
	BYTE buff[256];
	buff[0] = CURRENT_SHARE_MEMORY_ID;
	buff[1] = S_NO_BLOCK_ID;
	buff[2] = MSG_RADIO_CON_TDA7541_EEPROM_ID;
	buff[3] = ulRegAddr;

	readDataFromHardwareNoBlock(&buff[3],uiValBufLen);
	memcpy(pRegValBuf,&buff[3],uiValBufLen);

	DBG2(debugBuf("\nTDA7541 Radio EEPROM IIC READ---->",buff,uiValBufLen+3);)
	return TRUE;
}

static void ReadParameterFromEEPROM(BYTE *pReadParameter)
{
	UINT i;
	BYTE ReadData[INIT_DATA_SIZE];
	I2C_Read_EEPROM(0, ReadData, INIT_DATA_SIZE);
	for(i=0; i<INIT_DATA_SIZE; i++)
	{
		pReadParameter[i]=ReadData[i];
	}
}

static void TDA7541_WriteTunerRegister(BYTE start, BYTE end, BYTE *pBuffer)
{
	/***************************************************************************/
	//DBG3(RETAILMSG(1, (TEXT("\r\nFlyAudio prepare to write 7541's reg:")));)
	//DBG3(RETAILMSG(1, (TEXT("\r\n--addr:0x%x,--len:%d"),start,(end-start+1)));)
	/***************************************************************************/

	//I2C_Write_Tda7541(start,pBuffer,(end-start+1));
	if (start < end)
	{
		I2C_Write_Tda7541(start|AUTO_INCREMENT_MODE,pBuffer,(end-start+1)); 
	}
	else if (start == end)
	{
		I2C_Write_Tda7541(start,pBuffer,(end-start+1));
	}
}

void flyRadioReturnToUserPutToBuff(BYTE *buf, UINT16 len)
 {
	 UINT dwLength;

	 dwLength = writeToJNIBuff(CURRENT_SHARE_MEMORY_ID,buf,len);
	 if (dwLength)
	 {
		 DBG1(debugBuf("\nRADIO-HAL write  bytes to User OK:", buf,len);)
	 }
	 else
	 {
		 DBG1(debugBuf("\nRADIO-HAL write  bytes to User Error:", buf,len);)
	 }
 }
 
 /******************************************************************************/
 /*                            返回给用户的各种信息                        */
 /******************************************************************************/
 void returnRadioPowerMode(BOOL bOn)
 {
	 BYTE buf[] = {0x01,0x00};

	 if (bOn)
	 {
		 buf[1] = 1;
	 }
	 else
	 {
		 buf[1] = 0;
	 }

	 flyRadioReturnToUserPutToBuff(buf,2);
 }

 void returnRadioInitStatus(BOOL bOn)
 {
	 BYTE buf[] = {0x02,0x00};

	 if (bOn)
	 {
		 buf[1] = 1;
	 }
	 else
	 {
		 buf[1] = 0;
	 }

	 flyRadioReturnToUserPutToBuff(buf,2);
 }

 void returnRadioMode(BYTE eMode)
 {
	 BYTE buf[] = {0x20,0x00};

	 buf[1] = eMode;

	 flyRadioReturnToUserPutToBuff(buf,2);
 }

 void returnRadioFreq(UINT iFreq)
 {
	 BYTE buf[] = {0x10,0x00,0x00};

	 buf[1] = iFreq >> 8;
	 buf[2] = iFreq;

	 flyRadioReturnToUserPutToBuff(buf,3);
 }

 void returnRadioAFStatus(BOOL bOn)
 {
	 BYTE buf[] = {0x16,0x00};

	 if (bOn)
	 {
		 buf[1] = 1;
	 }
	 else
	 {
		 buf[1] = 0;
	 }

	 flyRadioReturnToUserPutToBuff(buf,2);
 }

 void returnRadioTAStatus(BOOL bOn)
 {
	 BYTE buf[] = {0x17,0x00};

	 if (bOn)
	 {
		 buf[1] = 1;
	 }
	 else
	 {
		 buf[1] = 0;
	 }

	 flyRadioReturnToUserPutToBuff(buf,2);
 }

 void returnRadioScanCtrl(BYTE cmd)
 {
	 BYTE buf[] = {0x13,0x00};

	 buf[1] = cmd;

	 flyRadioReturnToUserPutToBuff(buf,2);
 }

 void returnRadioMuteStatus(BOOL bOn)
 {
	 BYTE buf[] = {0x15,0x00};

	 if (bOn)
	 {
		 buf[1] = 1;
	 }
	 else
	 {
		 buf[1] = 0;
	 }

	 flyRadioReturnToUserPutToBuff(buf,2);
 }

 void returnRadioRDSWorkStatus(BOOL bOn)
 {
	 BYTE buf[] = {0x30,0x00};

	 if (bOn)
	 {
		 buf[1] = 1;
	 }
	 else
	 {
		 buf[1] = 0;
	 }

	 flyRadioReturnToUserPutToBuff(buf,2);
 }

 void returnRadioHaveSearched(BOOL bHave)
 {
	 BYTE buf[] = {0x14,0x00};

	 if (bHave)
	 {
		 buf[1] = 0;
	 }

	 flyRadioReturnToUserPutToBuff(buf,2);
 }

 void returnRadioBlinkingStatus(BOOL bOn)
 {
	 BYTE buf[] = {0x18,0x00};

	 if (bOn)
	 {
		 buf[1] = 1;
	 }
	 else
	 {
		 buf[1] = 0;
	 }

	 flyRadioReturnToUserPutToBuff(buf,2);
 }

/******************************************************************************/
/*                                各种7541操作                             */
/******************************************************************************/
 static BYTE TDA7541_Calculate_TV_SM_Value(INT range,UINT RF_Freq, BYTE mode)
 {
	 UINT val;
	 UINT sta_val,sto_val;
	 UINT sta_freq,sto_freq;
	 BYTE Area;

	 Area = pFlyRadioInfo->radioInfo.eCurRadioArea;
	 if(Area==AREA_OIRT && RF_Freq<=74000)
	 {
		 if(mode==TV)
		 {
			 sta_val = pFlyRadioInfo->mParameterTable[OIRT_TV_OFFSET+range-1];
			 sto_val = pFlyRadioInfo->mParameterTable[OIRT_TV_OFFSET+range];
		 }
		 else
		 {
			 sta_val = pFlyRadioInfo->mParameterTable[OIRT_SM_OFFSET+range-1];
			 sto_val = pFlyRadioInfo->mParameterTable[OIRT_SM_OFFSET+range];
		 }
		 sta_freq = (UINT)((UINT)(pFlyRadioInfo->mParameterTable[OIRT_FREQ_OFFSET+range-1] + 640) * 100);
		 sto_freq = (UINT)((UINT)(pFlyRadioInfo->mParameterTable[OIRT_FREQ_OFFSET+range] + 640) * 100);
	 }
	 else if(Area==AREA_JAPAN&& RF_Freq<=90000)
	 {
		 if(mode == TV)
		 {
			 sta_val = pFlyRadioInfo->mParameterTable[JAPAN_TV_OFFSET+range-1];
			 sto_val = pFlyRadioInfo->mParameterTable[JAPAN_TV_OFFSET+range];
		 }
		 else
		 {
			 sta_val = pFlyRadioInfo->mParameterTable[JAPAN_SM_OFFSET+range-1];
			 sto_val = pFlyRadioInfo->mParameterTable[JAPAN_SM_OFFSET+range];
		 }
		 sta_freq = (UINT)((UINT)(pFlyRadioInfo->mParameterTable[JAPAN_FREQ_OFFSET+range-1] + 760) * 100);
		 sto_freq = (UINT)((UINT)(pFlyRadioInfo->mParameterTable[JAPAN_FREQ_OFFSET+range] + 760) * 100);
	 }
	 else
	 {
		 if(mode == TV)
		 {
			 sta_val = pFlyRadioInfo->mParameterTable[EU_TV_OFFSET+range-2];
			 sto_val = pFlyRadioInfo->mParameterTable[EU_TV_OFFSET+range];
		 }
		 else
		 {
			 sta_val = pFlyRadioInfo->mParameterTable[EU_SM_OFFSET+range-1];
			 sto_val = pFlyRadioInfo->mParameterTable[EU_SM_OFFSET+range];
		 }
		 sta_freq = (UINT)((UINT)(pFlyRadioInfo->mParameterTable[EU_FREQ_OFFSET+range-2] + 875) * 100);
		 sto_freq = (UINT)((UINT)(pFlyRadioInfo->mParameterTable[EU_FREQ_OFFSET+range] + 875) * 100);
	 }

	 if(mode==TV)
	 {
		 sta_val=(sta_val<0x80)?(~sta_val)&0x7f:sta_val;
		 sto_val=(sto_val<0x80)?(~sto_val)&0x7f:sto_val;
	 }
	 if(sta_val>=sto_val)
	 {
		 val=(float) ((sto_freq-RF_Freq)*(sta_val-sto_val)/(sto_freq-sta_freq))+0.5;
		 val=(sto_val+val);
	 }
	 else
	 {
		 val=(float) ((RF_Freq-sta_freq)*(sto_val-sta_val)/(sto_freq-sta_freq))+0.5;
		 val=(sta_val+val);
	 }

	 if(mode==TV)
	 {
		 return (val<0x80)?(~val)&0x7f:val;
	 }
	 else return val;
 }
 static BYTE TDA7541_Get_TV_SM_Value(BYTE mode)
 {
	 INT i, align_num;
	 UINT fr;
	 BYTE Band;
	 BYTE Area;
	 UINT RF_Freq;

	 Band = pFlyRadioInfo->radioInfo.eCurRadioMode;
	 Area = pFlyRadioInfo->radioInfo.eCurRadioArea;
	 RF_Freq = *pFlyRadioInfo->radioInfo.pCurRadioFreq;

	 if(Band==FM1||Band==FM2)
	 {
		 RF_Freq = RF_Freq *10;
		 if(Area==AREA_OIRT && RF_Freq<=74000)
		 {
			 align_num=(pFlyRadioInfo->mParameterTable[OIRT_EU_ANUM]>>4)&0x0f;//Get the 4 High Bits.It determined by the points of OIRT or EU.
			 fr=(RF_Freq-64000)/10;//Range of value is from 0 to 1000.
			 i=1;

			 return TDA7541_Calculate_TV_SM_Value(i, RF_Freq, mode);
		 }
		 else if(Area==AREA_JAPAN && RF_Freq<=90000)
		 {
			 align_num=(pFlyRadioInfo->mParameterTable[JPN_ANUM])&0x0f;
			 fr=(RF_Freq-76000)/10;
			 i=1;

			 return TDA7541_Calculate_TV_SM_Value(i, RF_Freq, mode);
		 }
		 else
		 {
			 align_num=(pFlyRadioInfo->mParameterTable[OIRT_EU_ANUM])&0x0f;
			 fr=(RF_Freq-87500)/10;

			 for(i=0;i<align_num;i++)
			 {
				 if(fr<=(UINT) pFlyRadioInfo->mParameterTable[EU_FREQ_OFFSET+i]*10) break;
			 }
			 if(i==0) i=1;

			 return TDA7541_Calculate_TV_SM_Value(i, RF_Freq, mode);
		 }
	 }
	 else
	 {
		 if(mode==TV) return 0x80;
		 else return 2;
	 }
 }

 static BYTE TDA7541_GetCorrespondingSmeter(void)
 {
	 return TDA7541_Get_TV_SM_Value(SMETER);
 }

 static BYTE TDA7541_GetCorrespondingTV(void)
 {
	 return TDA7541_Get_TV_SM_Value(TV);
 }
 //y=ax+b(y单位是 mV)当FM时a=200;b=300 当AM时a=300;b=800
 static UINT TDA7541_GetSmeterVoltage(BYTE band)
 {
	 BYTE temp;
	 UINT voltage;
	 UINT adval;

	 if(band != AM)
	 {
		 temp = (pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+10] >> 4) & 0x0f;
		//DBG2(debugOneData("\nTDA7541 Radio SSTH----->",temp);)
		 if(temp == 0)
		 {
			 return 0;
		 }
		 voltage = 200*temp + 300;	
	 }
	 else
	 {
		 temp = (pFlyRadioInfo->mParameterTable[AM_REG_OFFSET+10] >> 4) & 0x0f;
		 if(temp == 0)
		 {
			 return 0;
		 }
		 voltage = 300*temp + 800;		
	 }
	 adval = (1024 * voltage)/3300;
	 return adval;
 }

 static UINT TDA7541_ADC_GetResult(void)
 {
	 BYTE buff[5];
	 BYTE timer;
	 timer = 0;
	 pFlyRadioInfo->bRadioADReturn = FALSE;

	 pFlyRadioInfo->RadioAD = 0;

	 buff[0] = CURRENT_SHARE_MEMORY_ID;
	 buff[1] = S_NO_BLOCK_ID;
	 buff[2] = MSG_RADIO_CON_AD_GET;
	 buff[3] = 0;
	 buff[4] = 0;
	 readDataFromHardwareNoBlock(buff,5);

	 pFlyRadioInfo->RadioAD = buff[3];
	 pFlyRadioInfo->RadioAD = (pFlyRadioInfo->RadioAD<<8) | buff[4];

	 //while ((!pFlyRadioInfo->bRadioADReturn) && (timer < 100))
	 //{
		// Sleep(10);
		// timer++;
	 //}

	 //DBG0(debugOneData("\nTDA7541  Radio ADC GetResult---->",pFlyRadioInfo->RadioAD);)
	 //DBG2(debugOneData("\nTDA7541  Radio timer---->",timer);)
	 return pFlyRadioInfo->RadioAD;
 }
 static UINT TDA7541_GetSmeter(void)
 {
	 return TDA7541_ADC_GetResult();
 }

 static void TDA7541_UpdateInitTblBit(BYTE *InitTbl, BYTE start, BYTE end, BYTE data)
 {
	 UINT i;
	 BYTE temp = 0;

	 for(i=start; i<=end; i++)
	 {
		 temp |= (1 << i);
	 }
	 *InitTbl = (BYTE)(*InitTbl & (BYTE)~temp) | data;
 }

 static BYTE  _2NPower(BYTE sender)
 {
	 BYTE pow;
	 switch(sender)
	 {
	 case 0:
		 pow=0x01;
		 break;
	 case 1:
		 pow=0x02;
		 break;
	 case 2:
		 pow=0x04;
		 break;
	 case 3:
		 pow=0x08;
		 break;
	 case 4:
		 pow=0x10;
		 break;
	 case 5:
		 pow=0x20;
		 break;
	 case 6:
		 pow=0x40;
		 break;
	 case 7:
		 pow=0x80;
		 break;
	 }
	 return pow;
 }
 static BYTE TDA7541_GetTunerParameter(BYTE data,BYTE B_sta,BYTE B_sto)
 {
	 BYTE i,value0=0,temp,array0[8]={0};

	 for(i=B_sta;i<=B_sto;i++)
	 {
		 temp = data & _2NPower(i);
		 array0[i]=temp;
		 value0|=array0[i];
	 }	
	 value0>>=B_sta;

	 return value0;
 }

 static void  LoadAlignedData2Tuner(BYTE Band)
 {
	 switch(Band)
	 {
	 case FM1:
	 case FM2:
		 TDA7541_WriteTunerRegister(TUNER_PARA_AREA_ADDR0_23, &pFlyRadioInfo->mParameterTable[FM_REG_OFFSET]);
		 TDA7541_WriteTunerRegister(TUNER_TESTING_CTROL_ADDR24_31, &FM_Test[0]);
		 break;
	 case AM:
		 TDA7541_WriteTunerRegister(TUNER_PARA_AREA_ADDR0_23, &pFlyRadioInfo->mParameterTable[AM_REG_OFFSET]);
		 TDA7541_WriteTunerRegister(TUNER_TESTING_CTROL_ADDR24_31, &AM_Test[0]);	
		 break;
	 default:break;
	 }
 }

 static void ChargePumpSetting(void)
 {
	 BYTE Area;
	 BYTE Band;

	 Band = pFlyRadioInfo->radioInfo.eCurRadioMode;
	 Area = pFlyRadioInfo->radioInfo.eCurRadioArea;
	 switch(Band)
	 {
	 case FM1:
	 case FM2:
		 switch(Area)
		 {
		 case AREA_EU:
		 case AREA_OIRT:
		 case AREA_JAPAN:
		 case AREA_USA_WB:
		 case AREA_SAM:
			 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+0], TDA7541_ADDR0_REG_ICP, TDA7541_PLL_HICURRENT_2MA);
			 TDA7541_WriteTunerRegister(TDA7541_ADDR0_REG, &pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+0]);
			 break;
		 }
		 break;
	 case AM:
		 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+0], TDA7541_ADDR0_REG_ICP, TDA7541_PLL_HICURRENT_3MA);
		 TDA7541_WriteTunerRegister(TDA7541_ADDR0_REG, &pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+0]);
		 break;			
	 }
 }

 static void Set_Freq_7541(BYTE mode,UINT freq)
 {
	 UINT Pc = 0;
	 BYTE Pcl = 0;
	 BYTE Pch = 0;
	 BYTE tv = 0;
	 BYTE sm = 0;
	 BYTE OIRT_Image = 0;
	 BYTE i = 0;
	 UINT Level = 0;
	 BYTE Temp[10] = {1};
	 BYTE Area = 0;

	 Area = pFlyRadioInfo->radioInfo.eCurRadioArea;
	 DBG0(debugOneData("\nTDA7541 Radio set freq start freq---->",freq);)
	 //DBG2(debugOneData("\nTDA7541 Radio set freq start mode---->",mode);)
	 DBG2(debugOneData("\nTDA7541 Radio set freq start area---->",Area);)
	 if(mode != AM)
	 {
		 freq = freq*10;

		 if (freq > 100000)
		 {
			 //SSTOP Config 
			 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+10], TDA7541_ADDR10_REG_SSTH, TDA7541_SSTOP_IFC_FM1_1_AM2_0);
		 }
		 else if (freq < 98100)
		 {
			 //SSTOP Config 
			 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+10], TDA7541_ADDR10_REG_SSTH, TDA7541_SSTOP_IFC_FM0_9_AM1_7);
		 }
		 else
		 {
			 //SSTOP Config 
			 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+10], TDA7541_ADDR10_REG_SSTH, TDA7541_SSTOP_IFC_FM0_9_AM1_7);
		 }
		 TDA7541_WriteTunerRegister(TDA7541_ADDR10_REG, &pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+10]);

		 switch(Area)
		 {
		 case AREA_OIRT:
			 if((freq>=64000)&&(freq<=74000))
			 {
				 Pc=((freq+IF)/10)-32;
				 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+5], TDA7541_ADDR5_REG_RC, TDA7541_PLL_REFERECE_10KHz);//10k
				 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+5], TDA7541_ADDR5_REG_VCOD, TDA7541_VCO_DIVIDER_3);// vco 1/3 div
				 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+5], TDA7541_ADDR5_REG_VCOI, TDA7541_VCO_PHASE_180);// 180
				 TDA7541_WriteTunerRegister(TDA7541_ADDR5_REG, &pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+5]);

				 tv=TDA7541_GetCorrespondingTV();
				 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+3], TDA7541_ADDR3_REG_TV, tv);// tv value
				 TDA7541_WriteTunerRegister(TDA7541_ADDR3_REG, &pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+3]);

				 //sm=TDA7541_GetCorrespondingSmeter();
				 //TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+13], TDA7541_ADDR13_REG_SMETER, sm);//load smeter
				 //TDA7541_WriteTunerRegister(TDA7541_ADDR13_REG, &pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+13]);

				 OIRT_Image = TDA7541_GetTunerParameter(pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+15],TDA7541_ADDR15_ORT_IQ);
				 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[OIRT_IQ_OFFESET],TDA7541_ADDR15_ORT_IQ,OIRT_Image);//OIRT Image
				 TDA7541_WriteTunerRegister(TDA7541_ADDR15_REG, &pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+15]);

				 /*************************************************************************************************/
				 //TDA7541_UpdateInitTblBit(&pTDA7541RadioInfo->mParameterTable[FM_REG_OFFSET+4],TDA7541_ADDR4_REG_TVO,TDA7541_PLL_TVOFFSET_ENABLE);
				 //TDA7541_WriteTunerRegister(pTDA7541RadioInfo,TDA7541_ADDR4_REG, &pTDA7541RadioInfo->mParameterTable[FM_REG_OFFSET+4]);
				 /*************************************************************************************************/
				 TDA7541_WriteTunerRegister(TDA7541_ADDR4_REG, &pFlyRadioInfo->mParameterTable[OIRT_TVOP]);// load ORT +3.175 setting from eeprom

				 TDA7541_WriteTunerRegister(TDA7541_ADDR17_REG,&pFlyRadioInfo->mParameterTable[OIRT_SEP]);// load ORT sep.
				 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+19],TDA7541_ADDR19_DEEMP,TDA7541_DEEMP_50US);				
				 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+19],TDA7541_ADDR19_HIGH_CUT_MAX,TDA7541_HIGH_CUT_MAX_10DB);//Maximum high cut 10dB.
				 TDA7541_WriteTunerRegister(TDA7541_ADDR19_REG, &pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+19]);
				 break;
			 }
		 case AREA_EU:					
			 Pc=((freq+IF)/50)-32;
			 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+5], TDA7541_ADDR5_REG_RC, TDA7541_PLL_REFERECE_50KHz);//50k	
			 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+5], TDA7541_ADDR5_REG_VCOD, TDA7541_VCO_DIVIDER_2);// vco 1/2 div
			 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+5], TDA7541_ADDR5_REG_VCOI, TDA7541_VCO_PHASE_180);// 180
			 TDA7541_WriteTunerRegister(TDA7541_ADDR5_REG, &pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+5]);

			 tv=TDA7541_GetCorrespondingTV();
			 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+3], TDA7541_ADDR3_REG_TV, tv);// tv value
			 TDA7541_WriteTunerRegister(TDA7541_ADDR3_REG, &pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+3]);

			 //sm=TDA7541_GetCorrespondingSmeter();
			 //TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+13], TDA7541_ADDR13_REG_SMETER, sm);//load smeter
			 //TDA7541_WriteTunerRegister(TDA7541_ADDR13_REG, &pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+13]);

			 TDA7541_WriteTunerRegister(TDA7541_ADDR15_REG, &pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+15]);//FM Image

			 /**************************************************************************************/
			 //TDA7541_UpdateInitTblBit(&pTDA7541RadioInfo->mParameterTable[FM_REG_OFFSET+4],TDA7541_ADDR4_REG_TVO,TDA7541_PLL_TVOFFSET_DISABLE);// disable +3.175
			 /**************************************************************************************/
			 TDA7541_WriteTunerRegister(TDA7541_ADDR4_REG, &pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+4]);// load EU disable +3.175 setting	

			 TDA7541_WriteTunerRegister(TDA7541_ADDR17_REG,&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+17]);// load Eu sep.
			 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+19],TDA7541_ADDR19_DEEMP,TDA7541_DEEMP_50US);				
			 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+19],TDA7541_ADDR19_HIGH_CUT_MAX,TDA7541_HIGH_CUT_MAX_10DB);//Maximum high cut 10dB.
			 TDA7541_WriteTunerRegister(TDA7541_ADDR19_REG, &pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+19]);
			 break;
		 case AREA_JAPAN:    		
			 if(freq>=74000 && freq<87500)
			 {
				 Pc=((freq-IF)/50)-32;
				 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+5], TDA7541_ADDR5_REG_RC, TDA7541_PLL_REFERECE_50KHz);//50k		
				 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+5], TDA7541_ADDR5_REG_VCOD, TDA7541_VCO_DIVIDER_3);// vco 1/3 div
				 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+5], TDA7541_ADDR5_REG_VCOI, TDA7541_VCO_PHASE_0);// 0	
				 TDA7541_WriteTunerRegister(TDA7541_ADDR5_REG, &pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+5]);					
			 }                			
			 else if(freq>=64000 && freq<=74000)
			 {
				 Pc=((freq+IF)/50)-32;
				 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+5], TDA7541_ADDR5_REG_RC, TDA7541_PLL_REFERECE_10KHz);//10k
				 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+5], TDA7541_ADDR5_REG_VCOD, TDA7541_VCO_DIVIDER_3);// vco 1/3 div
				 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+5], TDA7541_ADDR5_REG_VCOI, TDA7541_VCO_PHASE_180);// 180
				 TDA7541_WriteTunerRegister(TDA7541_ADDR5_REG, &pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+5]);
			 }					
			 else 
			 {
				 Pc=((freq+IF)/50)-32;
				 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+5], TDA7541_ADDR5_REG_RC, TDA7541_PLL_REFERECE_50KHz);//50k
				 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+5], TDA7541_ADDR5_REG_VCOD, TDA7541_VCO_DIVIDER_3);// vco 1/3 div
				 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+5], TDA7541_ADDR5_REG_VCOI, TDA7541_VCO_PHASE_180);// 180
				 TDA7541_WriteTunerRegister(TDA7541_ADDR5_REG, &pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+5]);
			 }					
			 tv=TDA7541_GetCorrespondingTV();
			 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+3], TDA7541_ADDR3_REG_TV, tv);// tv value
			 TDA7541_WriteTunerRegister(TDA7541_ADDR3_REG, &pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+3]);

			 //sm=TDA7541_GetCorrespondingSmeter();
			 //TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+13], TDA7541_ADDR13_REG_SMETER, sm);//load smeter
			 //TDA7541_WriteTunerRegister(TDA7541_ADDR13_REG, &pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+13]);

			 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+11],TDA7541_ADDR11_WNON,TDA7541_GetTunerParameter(pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+11],TDA7541_ADDR11_WNON));// wb off
			 TDA7541_WriteTunerRegister(TDA7541_ADDR11_REG, &pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+11]);

			 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+4],TDA7541_ADDR4_REG_TVO,TDA7541_PLL_TVOFFSET_DISABLE);// disable +3.175
			 TDA7541_WriteTunerRegister(TDA7541_ADDR4_REG, &pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+4]);// load EU disable +3.175 setting	
			 break;
		 case AREA_USA_WB:
			 Pc=((freq+IF)/25)-32;
			 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+5], TDA7541_ADDR5_REG_RC, TDA7541_PLL_REFERECE_25KHz);//25k
			 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+5], TDA7541_ADDR5_REG_VCOD, TDA7541_VCO_DIVIDER_ORIGINAL);// vco 1/1 div
			 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+5], TDA7541_ADDR5_REG_VCOI, TDA7541_VCO_PHASE_180);// 180
			 TDA7541_WriteTunerRegister(TDA7541_ADDR5_REG, &pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+5]);

			 tv=TDA7541_GetCorrespondingTV();
			 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+3], TDA7541_ADDR3_REG_TV, tv);// tv value
			 TDA7541_WriteTunerRegister(TDA7541_ADDR3_REG, &pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+3]);

			 //sm=TDA7541_GetCorrespondingSmeter();
			 //TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+13], TDA7541_ADDR13_REG_SMETER, sm);//load smeter
			 //TDA7541_WriteTunerRegister(TDA7541_ADDR13_REG, &pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+13]);

			 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+11],TDA7541_ADDR11_WNON,~(TDA7541_GetTunerParameter(pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+11],TDA7541_ADDR11_WNON)));// wb on
			 TDA7541_WriteTunerRegister(TDA7541_ADDR11_REG, &pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+11]);

			 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+4],TDA7541_ADDR4_REG_TVO,TDA7541_PLL_TVOFFSET_DISABLE);// disable +3.175
			 TDA7541_WriteTunerRegister(TDA7541_ADDR4_REG, &pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+4]);// load EU disable +3.175 setting	
			 break;
		 case AREA_SAM:
			 Pc=((freq+IF)/50)-32;
			 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+5], TDA7541_ADDR5_REG_RC, TDA7541_PLL_REFERECE_50KHz);//50k
			 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+5], TDA7541_ADDR5_REG_VCOD, TDA7541_VCO_DIVIDER_2);// vco 1/2 div
			 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+5], TDA7541_ADDR5_REG_VCOI, TDA7541_VCO_PHASE_180);// 180
			 TDA7541_WriteTunerRegister(TDA7541_ADDR5_REG, &pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+5]);

			 tv=TDA7541_GetCorrespondingTV();
			 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+3], TDA7541_ADDR3_REG_TV, tv);// tv value
			 TDA7541_WriteTunerRegister(TDA7541_ADDR3_REG, &pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+3]);

			 //sm=TDA7541_GetCorrespondingSmeter();
			 //TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+13], TDA7541_ADDR13_REG_SMETER, sm);//load smeter
			 //TDA7541_WriteTunerRegister(TDA7541_ADDR13_REG, &pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+13]);

			 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+4],TDA7541_ADDR4_REG_TVO,TDA7541_PLL_TVOFFSET_DISABLE);// disable +3.175
			 TDA7541_WriteTunerRegister(TDA7541_ADDR4_REG, &pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+4]);// load EU disable +3.175 setting	

			 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+19],TDA7541_ADDR19_DEEMP,TDA7541_DEEMP_75US);				
			 TDA7541_WriteTunerRegister(TDA7541_ADDR19_REG, &pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+19]);
			 break;
		 }

		 Pcl=Pc&0xff;
		 Pch=((Pc>>8)&0xff);
		 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+1], TDA7541_ADDR1_REG_PC_LSB, Pcl);
		 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+2], TDA7541_ADDR2_REG_PC_MSB, Pch);
		 TDA7541_WriteTunerRegister(TDA7541_ADDR1_REG, &pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+1]);
		 TDA7541_WriteTunerRegister(TDA7541_ADDR2_REG, &pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+2]);
	 }
	 else//AM
	 {
		 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[AM_REG_OFFSET+4], TDA7541_ADDR4_REG_FMON, TDA7541_AMMODE);//AM mode
		 TDA7541_WriteTunerRegister(TDA7541_ADDR4_REG, &pFlyRadioInfo->mParameterTable[AM_REG_OFFSET+4]);
		 TDA7541_WriteTunerRegister(TDA7541_ADDR14_REG, &pFlyRadioInfo->mParameterTable[AM_REG_OFFSET+14]);
		 TDA7541_WriteTunerRegister(TDA7541_ADDR10_REG, &pFlyRadioInfo->mParameterTable[AM_REG_OFFSET+10]);
		 Pc=(freq+IF)-32;			

		 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[AM_REG_OFFSET+5], TDA7541_ADDR5_REG_RC, TDA7541_PLL_REFERECE_10KHz);
		 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[AM_REG_OFFSET+5], TDA7541_ADDR5_REG_VCOD, TDA7541_VCO_DIVIDER_2);// vco 1/2 div
		 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[AM_REG_OFFSET+5], TDA7541_ADDR5_REG_AMD, TDA7541_AM_PREDIVIDER_10);
		 TDA7541_WriteTunerRegister(TDA7541_ADDR5_REG, &pFlyRadioInfo->mParameterTable[AM_REG_OFFSET+5]);
		 Pcl=Pc&0xff;
		 Pch=((Pc>>8)&0xff);
		 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[AM_REG_OFFSET+1], TDA7541_ADDR1_REG_PC_LSB, Pcl);
		 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[AM_REG_OFFSET+2], TDA7541_ADDR2_REG_PC_MSB, Pch);
		 TDA7541_WriteTunerRegister(TDA7541_ADDR1_REG, &pFlyRadioInfo->mParameterTable[AM_REG_OFFSET+1]);
		 TDA7541_WriteTunerRegister(TDA7541_ADDR2_REG, &pFlyRadioInfo->mParameterTable[AM_REG_OFFSET+2]);
		 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[AM_REG_OFFSET+11],TDA7541_ADDR11_WNON,TDA7541_GetTunerParameter(pFlyRadioInfo->mParameterTable[AM_REG_OFFSET+11],TDA7541_ADDR11_WNON));// wb off
		 TDA7541_WriteTunerRegister(TDA7541_ADDR11_REG, &pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+11]);
		 switch(Area)
		 {
		 case AREA_OIRT:					
		 case AREA_EU:
		 case AREA_JAPAN:
			 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[AM_REG_OFFSET+19],TDA7541_ADDR19_DEEMP,TDA7541_DEEMP_50US);				
			 TDA7541_WriteTunerRegister(TDA7541_ADDR19_REG, &pFlyRadioInfo->mParameterTable[AM_REG_OFFSET+19]);
			 break;
		 case AREA_SAM:
		 case AREA_USA_WB:
			 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[AM_REG_OFFSET+19],TDA7541_ADDR19_DEEMP,TDA7541_DEEMP_75US);				
			 TDA7541_WriteTunerRegister(TDA7541_ADDR19_REG, &pFlyRadioInfo->mParameterTable[AM_REG_OFFSET+19]);
			 break;
		 }		
	 }
	 ChargePumpSetting();
	 //DBG2(debugString("\nTDA7541 Radio FM/AM TUNER_PARA_AREA_ADDR0_23");)
	 //for (i = 0;i < 24; i++)
	 //{
		// if (mode != AM)
		// {		
		//	 DBG2(debugString("\n[");)
		//	 DBG2(debugOneData("",i);)
		//	 DBG2(debugString("]---->");)
		//	 DBG2(debugOneData("0x",pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+i]);)
		// }
		// else
		// {
		//	 DBG2(debugString("\n[");)
		//	 DBG2(debugOneData("",i);)
		//	 DBG2(debugString("]---->");)
		//	 DBG2(debugOneData("0x",pFlyRadioInfo->mParameterTable[AM_REG_OFFSET+i]);)
		// }
	 //}
	 //DBG2(debugOneData("\nTDA7541 Radio set freq end freq---->",freq);)
	 //DBG2(debugOneData("\nTDA7541 Radio set freq end mode---->",mode);)
	 //DBG2(debugOneData("\nTDA7541 Radio set freq end area---->",Area);)

	 RegDataWriteRadio();
 }

 static void TDA7541Radio_ChangeToFMAM(BYTE mode)
 {
	 DBG2(debugString("\nTDA7541 Radio_ChangeToFMAM");)
	 if (AM != mode)
	 {
		 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+4], TDA7541_ADDR4_REG_FMON, TDA7541_FMMODE);//FM mode
		 TDA7541_WriteTunerRegister(TDA7541_ADDR4_REG, &pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+4]);
		 //TDA7541_UpdateInitTblBit(&pTDA7541RadioInfo->mParameterTable[FM_REG_OFFSET+14], TDA7541_ADDR14_IF2Q, TDA7541_IF2Q_R5k_FMQ3);
		 if (mode == FM1)
		 {
			 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+0], TDA7541_ADDR0_REG_LM, TDA7541_LOCAL_DISABLE);
			 //	TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+14], TDA7541_ADDR14_IF1G, TDA7541_IF2_GAIN_21);
			 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+18], TDA7541_ADDR18_SBC, TDA7541_SBC_29_PERSENT);//控制开始分离
		 } 
		 else
		 {		
			 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+0], TDA7541_ADDR0_REG_LM, TDA7541_LOCAL_DISABLE);
			 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+14], TDA7541_ADDR14_IF1G, TDA7541_IF2_GAIN_21);
			 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+18], TDA7541_ADDR18_SBC, TDA7541_SBC_29_PERSENT);
		 }
		 TDA7541_WriteTunerRegister(TDA7541_ADDR0_REG, &pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+0]);
		 TDA7541_WriteTunerRegister(TDA7541_ADDR14_REG, &pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+14]);
		 TDA7541_WriteTunerRegister(TDA7541_ADDR18_REG, &pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+18]);
	 }
	 else
	 {
		 
	 }

	 Set_Freq_7541(mode,*pFlyRadioInfo->radioInfo.pCurRadioFreq);
	 pFlyRadioInfo->bCurMute = TRUE;//触发恢复静音状态
 }

 static UINT RadioStepFreqGenerate(BYTE eMode,UINT iFreq,BYTE eForward,BYTE eStepMode)
 {
	 if (STEP_FORWARD == eForward)//Forward
	 {	
		 if(AM != eMode)
		 {
			 if(STEP_MANUAL == eStepMode)//手动
			 {
				 iFreq += pFlyRadioInfo->radioInfo.iFreqFMManualStep;
			 }
			 else//自动
			 {
				 iFreq += pFlyRadioInfo->radioInfo.iFreqFMScanStep;
			 }
			 if(iFreq > pFlyRadioInfo->radioInfo.iFreqFMMax)	iFreq = pFlyRadioInfo->radioInfo.iFreqFMMin;
			 if(iFreq < pFlyRadioInfo->radioInfo.iFreqFMMin)	iFreq = pFlyRadioInfo->radioInfo.iFreqFMMin;

		 }
		 else
		 {
			 if(STEP_MANUAL == eStepMode)//手动
			 {
				 iFreq += pFlyRadioInfo->radioInfo.iFreqAMManualStep;
			 }
			 else//自动
			 {
				 iFreq += pFlyRadioInfo->radioInfo.iFreqAMScanStep;
			 }
			 if(iFreq > pFlyRadioInfo->radioInfo.iFreqAMMax)	iFreq = pFlyRadioInfo->radioInfo.iFreqAMMin;
			 if(iFreq < pFlyRadioInfo->radioInfo.iFreqAMMin)	iFreq = pFlyRadioInfo->radioInfo.iFreqAMMin;
		 }
	 }
	 else if (STEP_BACKWARD == eForward)//Backward
	 {
		 if(AM != eMode)
		 {
			 if(STEP_MANUAL == eStepMode)//手动
			 {
				 iFreq -= pFlyRadioInfo->radioInfo.iFreqFMManualStep;
			 }
			 else//自动
			 {
				 iFreq -= pFlyRadioInfo->radioInfo.iFreqFMScanStep;
			 }
			 if(iFreq < pFlyRadioInfo->radioInfo.iFreqFMMin)	iFreq = pFlyRadioInfo->radioInfo.iFreqFMMax;
			 if(iFreq > pFlyRadioInfo->radioInfo.iFreqFMMax)	iFreq = pFlyRadioInfo->radioInfo.iFreqFMMax;
		 }
		 else
		 {
			 if(STEP_MANUAL == eStepMode)//手动
			 {
				 iFreq -= pFlyRadioInfo->radioInfo.iFreqAMManualStep;
			 }
			 else// if(RadioScanStatus == 1)//自动
			 {
				 iFreq -= pFlyRadioInfo->radioInfo.iFreqAMScanStep;
			 }
			 if(iFreq < pFlyRadioInfo->radioInfo.iFreqAMMin)	iFreq = pFlyRadioInfo->radioInfo.iFreqAMMax;
			 if(iFreq > pFlyRadioInfo->radioInfo.iFreqAMMax)	iFreq = pFlyRadioInfo->radioInfo.iFreqAMMax;
		 }
	 }
	 else {
		 if(AM != eMode)
		 {
			 if(iFreq > pFlyRadioInfo->radioInfo.iFreqFMMax)	iFreq = pFlyRadioInfo->radioInfo.iFreqFMMin;
			 if(iFreq < pFlyRadioInfo->radioInfo.iFreqFMMin)	iFreq = pFlyRadioInfo->radioInfo.iFreqFMMin;
		 }
		 else
		 {
			 if(iFreq > pFlyRadioInfo->radioInfo.iFreqAMMax)	iFreq = pFlyRadioInfo->radioInfo.iFreqAMMin;
			 if(iFreq < pFlyRadioInfo->radioInfo.iFreqAMMin)	iFreq = pFlyRadioInfo->radioInfo.iFreqAMMin;
		 }
	 }
	 return iFreq;
 }

 static void TDA7541_Radio_Mute(BOOL ctrl)
 {
	 if (ctrl == TRUE)
	 {
		 DBG2(debugString("\nTDA7541 Radio Mute---->ON");)
		 
		 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+0], TDA7541_ADDR0_REG_SDM,  TDA7541_STEREO_MUTE_ENABLE);  // mute
		 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[AM_REG_OFFSET+0], TDA7541_ADDR0_REG_SDM,  TDA7541_STEREO_MUTE_ENABLE);
	 
		 if (AM != pFlyRadioInfo->radioInfo.eCurRadioMode)
		 {
			 TDA7541_WriteTunerRegister(TDA7541_ADDR0_REG, &pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+0]);
		 }
		 else
		 {
			 TDA7541_WriteTunerRegister(TDA7541_ADDR0_REG, &pFlyRadioInfo->mParameterTable[AM_REG_OFFSET+0]);
		 }

		 //while(ipcWhatEventOn(EVENT_GLOBAL_RADIO_MUTE_IN_REQ_ID) || ipcWhatEventOn(EVENT_GLOBAL_RADIO_MUTE_OUT_REQ_ID))
		 //{
			// Sleep(10);
		 //}
		 //ipcStartEvent(EVENT_GLOBAL_RADIO_MUTE_IN_REQ_ID);//发送进入静音
		 //while (!ipcWhatEventOn(EVENT_GLOBAL_RADIO_MUTE_IN_OK_ID))//等待OK
		 //{
			// Sleep(10);
		 //}
		 //ipcClearEvent(EVENT_GLOBAL_RADIO_MUTE_IN_OK_ID);//清除
	 }
	 else
	 {
		 DBG2(debugString("\nTDA7541 Radio Mute---->OFF");)

		 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+0], TDA7541_ADDR0_REG_SDM,  TDA7541_STEREO_MUTE_DISABLE);  // demute
		 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[AM_REG_OFFSET+0], TDA7541_ADDR0_REG_SDM,  TDA7541_STEREO_MUTE_DISABLE);

		 if (AM != pFlyRadioInfo->radioInfo.eCurRadioMode)
		 {
			 TDA7541_WriteTunerRegister(TDA7541_ADDR0_REG, &pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+0]);
		 }
		 else
		 {
			 TDA7541_WriteTunerRegister(TDA7541_ADDR0_REG, &pFlyRadioInfo->mParameterTable[AM_REG_OFFSET+0]);
		 }

		 //ipcStartEvent(EVENT_GLOBAL_RADIO_MUTE_OUT_REQ_ID);//发送退出静音
	 }

 }

 static BOOL bRadioSignalGood(BYTE radioMode,UINT *pLevel)
 {
	 BYTE i =0;
	 UINT sm[5];
	 UINT sm_total,sm_average,smth;
	 UINT cnt,high_flag;

	 cnt = 0;
	 sm_average = 0;
	 high_flag = 0;
	 *pLevel = high_flag;

	 if(radioMode != AM)
	 {
		 smth = TDA7541_GetSmeterVoltage(FM1);
	 }
	 else 
	 {
		 smth = TDA7541_GetSmeterVoltage(AM);
	 }

	 sm_total = 0;
	 for (i = 0; i <5; i++)
	 {
		 sm[i] = TDA7541_GetSmeter();
		 sm_total = sm_total + sm[i];
		 Sleep(5);
	 }
	 sm_average = sm_total / 5;
	 DBG0(debugOneData("\nTDA7541 Radio Signal sm_average-->",sm_average*3300/1024);)
	 DBG0(debugOneData("smth_mV-->",smth*3300/1024);)
	 if (radioMode != AM)
	 {
		 if (*pFlyRadioInfo->radioInfo.pCurRadioFreq > 10000)
		 {
			 if(sm_average < 387)  //1250mV
			 {		
				 return FALSE;
				}
		 }
		 else if (*pFlyRadioInfo->radioInfo.pCurRadioFreq < 9810)
		 {
			 if(sm_average < 300)  //950mV
			 {		
				 return FALSE;
				}
		 }
		 else
		 {
			 if(sm_average < 316)  //1020mV
			 {		
				 return FALSE;
				}
		 }
	 }
	 else//AM
	 {
		 if (pFlyRadioInfo->radioInfo.iCurRadioFreqAM < 900)
			{
				if(sm_average < 433)
				{		
					return FALSE;
				}
			}
		 else if((900 < pFlyRadioInfo->radioInfo.iCurRadioFreqAM) && (pFlyRadioInfo->radioInfo.iCurRadioFreqAM < 1300))
			{
				if(sm_average < 395)
				{		
					return FALSE;
				}
			}
		 else
		{
				if(sm_average < 370)
				{		
					return FALSE;
				}
		 }	
	 }
	 Sleep(24);
	 //IO_Control_Init();
	 while (cnt<100)
	 {
		 if (READ_IO_SSTOP_IN())
		 {
			 high_flag++;
		 }
		 cnt++;
		 //Sleep(1);
	 }	
	 DBG0(debugOneData("\nTDA7541 Radio Signal sstop persent of 100-->",high_flag);)
	 if(radioMode != AM)
	 {
		 if (high_flag<88)
		 {
			 return FALSE;
		 }
	 }
	 else  //AM
	 {
		 if (high_flag<50)        //调节AM停台灵敏度
		 {
			 return FALSE;
		 }
	 }
	 *pLevel = (high_flag + sm_average);
	 return TRUE;
 }

 void TDA7541RadioJumpNewFreqParaInit(void)
 {
#if RADIO_RDS
	 RDSParaInit();
#endif
 }

 static void buttomJumpFreqAndReturn(void)//界面按键跳频用
 {
	 *pFlyRadioInfo->radioInfo.pPreRadioFreq = 
		 RadioStepFreqGenerate(pFlyRadioInfo->radioInfo.ePreRadioMode,*pFlyRadioInfo->radioInfo.pPreRadioFreq,pFlyRadioInfo->radioInfo.eButtomStepDirection,STEP_MANUAL);
	 /************************************************************************/
	 //Set_Freq_7541(pTDA7541RadioInfo
	 //	,pTDA7541RadioInfo->radioInfo.ePreRadioMode
	 //	,*pTDA7541RadioInfo->radioInfo.pPreRadioFreq);
	 /************************************************************************/
	 returnRadioFreq(*pFlyRadioInfo->radioInfo.pPreRadioFreq);
 }

 static UINT GetCorrectScanStartFreq(UINT *pFreq)
 {
	 UINT base;
	 if (AM != pFlyRadioInfo->radioInfo.eCurRadioMode)
	 {
		 if (((*pFreq - pFlyRadioInfo->radioInfo.iFreqFMMin) % pFlyRadioInfo->radioInfo.iFreqFMScanStep) != 0)
		 {
			 base = 1;
		 }
		 else
		 {
			 base = 0;
		 }
		 *pFreq = pFlyRadioInfo->radioInfo.iFreqFMMin + (base + (*pFreq - pFlyRadioInfo->radioInfo.iFreqFMMin)/pFlyRadioInfo->radioInfo.iFreqFMScanStep)*pFlyRadioInfo->radioInfo.iFreqFMScanStep;
	 }
	 else
	 {
		 if (((*pFreq - pFlyRadioInfo->radioInfo.iFreqAMMin) % pFlyRadioInfo->radioInfo.iFreqAMScanStep) != 0)
		 {
			 base = 1;
		 }
		 else
		 {
			 base = 0;
		 }		
		 *pFreq = pFlyRadioInfo->radioInfo.iFreqAMMin + (base + (*pFreq - pFlyRadioInfo->radioInfo.iFreqAMMin)/pFlyRadioInfo->radioInfo.iFreqAMScanStep)*pFlyRadioInfo->radioInfo.iFreqAMScanStep;
	 }
	 return *pFreq;
 }

 /******************************************************************************/
 /*                               各种数据初始化                           */
 /******************************************************************************/
 static void recoveryRadioRegData(void)
 {
	 LoadAlignedData2Tuner(pFlyRadioInfo->radioInfo.eCurRadioMode);
 }

 static void ParameterTable_Init(void)
 {
	 UINT i;
	 /************************************************************/
	 //BYTE temp;
	 //BYTE checksum1,checksum2,checksum3;

	 //ReadParameterFromEEPROM(pTDA7541RadioInfo,pTDA7541RadioInfo->mParameterTable);
	 //checksum1 = 0;
	 //for(i=0; i<34; i++)			// 计算校验
	 //{
	 //	checksum1 += pTDA7541RadioInfo->mParameterTable[i];
	 //}
	 //checksum1 -= 1;
	 //checksum1 &= 0xff;

	 //checksum2 = 0;
	 //for(i=34; i<58; i++)
	 //{
	 //	checksum2 += pTDA7541RadioInfo->mParameterTable[i];
	 //}
	 //checksum2 -= 1;
	 //checksum2 &= 0xff;

	 //checksum3 = 0;
	 //for(i=58; i<141; i++)
	 //{
	 //	checksum3 += pTDA7541RadioInfo->mParameterTable[i];
	 //}
	 //checksum3 -= 1;	
	 //checksum3 &= 0xff;

	 //if((pTDA7541RadioInfo->mParameterTable[0] != 0xca) || (pTDA7541RadioInfo->mParameterTable[FM_SUM] != checksum1) || 	   // 判断数据是否有错
	 //	(pTDA7541RadioInfo->mParameterTable[AM_SUM] != checksum2) || (pTDA7541RadioInfo->mParameterTable[AN_SUM] != checksum3))	 
	 //{	// 有错使用默认数据
	 //	for(i=0; i<INIT_DATA_SIZE; i++)
	 //	{
	 //		pTDA7541RadioInfo->mParameterTable[i] = DefaultData[i];
	 //	}
	 //}	
	 /************************************************************/
	 for (i=0; i<INIT_DATA_SIZE; i++)
	 {
		 pFlyRadioInfo->mParameterTable[i] = DefaultData[i];
	 }
	 /************************************************************/
	 //DBG3(RETAILMSG(1, (TEXT("\r\nFlyAudio mParameterTable data[0] == %x"),pTDA7541RadioInfo->mParameterTable[0]));)
	 //DBG3(RETAILMSG(1, (TEXT("\r\nFlyAudio mParameterTable checksum1 == %x[%x]"),checksum1,pTDA7541RadioInfo->mParameterTable[FM_SUM]));)
	 //DBG3(RETAILMSG(1, (TEXT("\r\nFlyAudio mParameterTable checksum2 == %x[%x]"),checksum2,pTDA7541RadioInfo->mParameterTable[AM_SUM]));)
	 //DBG3(RETAILMSG(1, (TEXT("\r\nFlyAudio mParameterTable checksum3 == %x[%x]\r\n"),checksum3,pTDA7541RadioInfo->mParameterTable[AN_SUM]));)
	 /************************************************************/

	 //FM
	 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+0], TDA7541_ADDR0_REG_LDENA, TDA7541_PLL_LOCK_ENABLE);
	 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+0], TDA7541_ADDR0_REG_SDM, TDA7541_STEREO_MUTE_ENABLE);
	 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+0], TDA7541_ADDR0_REG_LM, TDA7541_LOCAL_DISABLE);
	 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+0], TDA7541_ADDR0_REG_ASFC, TDA7541_ASFC_NORMAL);
	 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+0], TDA7541_ADDR0_REG_SEEK, TDA7541_SEEK_OFF);

	 pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+3] = 0x98;

	 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+20], TDA7541_ADDR20_MPFAST, TDA7541_MPTC_ENABLE);

	 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+4], TDA7541_ADDR4_REG_TVM, TDA7541_PLL_TV_TRACK);
	 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+4], TDA7541_ADDR4_REG_TVO, TDA7541_PLL_TVOFFSET_DISABLE);

	 /***********************************************************************/
	 //TDA7541_UpdateInitTblBit(&pTDA7541RadioInfo->mParameterTable[FM_REG_OFFSET+4], TDA7541_ADDR4_REG_ISSENA, TDA7541_ISS_ENABLE);

	 //TDA7541_UpdateInitTblBit(&pTDA7541RadioInfo->mParameterTable[FM_REG_OFFSET+4], TDA7541_ADDR4_REG_IFS, TDA7541_PLL_TSAMPLE_FM10_24_AM64);
	 //TDA7541_UpdateInitTblBit(&pTDA7541RadioInfo->mParameterTable[FM_REG_OFFSET+4], TDA7541_ADDR4_REG_EW, TDA7541_PLL_IFC_EW_FM100_AM16K);
	 /***********************************************************************/

	 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+4], TDA7541_ADDR4_REG_IFS, TDA7541_PLL_TSAMPLE_FM20_48_AM128);
	 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+4], TDA7541_ADDR4_REG_EW, TDA7541_PLL_IFC_EW_FM12_5_AM2K);

	 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+4], TDA7541_ADDR4_REG_FMON, TDA7541_FMMODE);

	 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+5], TDA7541_ADDR5_REG_VCOD, TDA7541_VCO_DIVIDER_2);
	 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+5], TDA7541_ADDR5_REG_VCOI, TDA7541_VCO_PHASE_180);
	 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+5], TDA7541_ADDR5_REG_RC, TDA7541_PLL_REFERECE_50KHz);

	 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+11], TDA7541_ADDR11_WNON, TDA7541_WB_DISABLE);

	 /***********************************************************************/
	 //TDA7541_UpdateInitTblBit(&pTDA7541RadioInfo->mParameterTable[FM_REG_OFFSET+11], TDA7541_ADDR11_WMTH, TDA7541_WMTH_STARTMUTE_1);
	 /***********************************************************************/

	 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+13], TDA7541_ADDR13_REG_SMETER, TDA7541_REG_SMETER_SLOPE_125V_20DB);
	 /***********************************************************************/
	 //TDA7541_UpdateInitTblBit(&pTDA7541RadioInfo->mParameterTable[FM_REG_OFFSET+13], TDA7541_ADDR13_REG_XTAL, TDA7541_XTAL_24_75PF);

	 //TDA7541_UpdateInitTblBit(&pTDA7541RadioInfo->mParameterTable[FM_REG_OFFSET+14], TDA7541_ADDR14_IF2A, TDA7541_IF2A_30_8PF);
	 /***********************************************************************/
	 pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+14] = 0xEF;

	 /***********************************************************************/
	 //TDA7541_UpdateInitTblBit(&pTDA7541RadioInfo->mParameterTable[FM_REG_OFFSET+15], TDA7541_ADDR15_PH, TDA7541_PH_3_DEGREE);
	 /***********************************************************************/

	 //SSTOP Config 
	 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+10], TDA7541_ADDR10_REG_SSTH, TDA7541_SSTOP_IFC_FM3_1_AM5_0);

	 //AM
	 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[AM_REG_OFFSET+0], TDA7541_ADDR0_REG_LDENA, TDA7541_PLL_LOCK_ENABLE);
	 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[AM_REG_OFFSET+0], TDA7541_ADDR0_REG_SDM, TDA7541_STEREO_MUTE_ENABLE);
	 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[AM_REG_OFFSET+0], TDA7541_ADDR0_REG_LM, TDA7541_LOCAL_DISABLE);
	 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[AM_REG_OFFSET+0], TDA7541_ADDR0_REG_ASFC, TDA7541_ASFC_NORMAL);
	 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[AM_REG_OFFSET+0], TDA7541_ADDR0_REG_SEEK, TDA7541_SEEK_OFF);

	 //pFlyRadioInfo->mParameterTable[AM_REG_OFFSET+3] = 0x98;

	 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[AM_REG_OFFSET+20], TDA7541_ADDR20_MPFAST, TDA7541_MPTC_ENABLE);

	 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[AM_REG_OFFSET+4], TDA7541_ADDR4_REG_TVM, TDA7541_PLL_TV_TRACK);
	 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[AM_REG_OFFSET+4], TDA7541_ADDR4_REG_TVO, TDA7541_PLL_TVOFFSET_DISABLE);

	 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[AM_REG_OFFSET+4], TDA7541_ADDR4_REG_IFS, TDA7541_PLL_TSAMPLE_FM10_24_AM64);
	 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[AM_REG_OFFSET+4], TDA7541_ADDR4_REG_EW, TDA7541_PLL_IFC_EW_FM100_AM16K);

	 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[AM_REG_OFFSET+4], TDA7541_ADDR4_REG_FMON, TDA7541_AMMODE);									

	 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[AM_REG_OFFSET+5], TDA7541_ADDR5_REG_VCOD, TDA7541_VCO_DIVIDER_2);
	 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[AM_REG_OFFSET+5], TDA7541_ADDR5_REG_VCOI, TDA7541_VCO_PHASE_180);
	 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[AM_REG_OFFSET+5], TDA7541_ADDR5_REG_RC, TDA7541_PLL_REFERECE_10KHz);
	 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[AM_REG_OFFSET+5], TDA7541_ADDR5_REG_AMD, TDA7541_AM_PREDIVIDER_10);

	 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[AM_REG_OFFSET+11], TDA7541_ADDR11_WNON, TDA7541_WB_DISABLE); 

	 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+11], TDA7541_ADDR11_WMTH, TDA7541_WMTH_STARTMUTE_3);

	 pFlyRadioInfo->mParameterTable[AM_REG_OFFSET+14] = 0x45;

	 /****************************************************************/
	 //TDA7541_UpdateInitTblBit(&pTDA7541RadioInfo->mParameterTable[AM_REG_OFFSET+10], TDA7541_ADDR10_REG_SSTH, TDA7541_SSTOP_IFC_FM0_7_AM1_4);
	 /****************************************************************/
	 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[AM_REG_OFFSET+10], TDA7541_ADDR10_REG_SSTH, TDA7541_SSTOP_IFC_FM0_5_AM1_1);

	 /* 以下程序是改正分离度问题 */
	 pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+17] = 0x87;

	 LoadAlignedData2Tuner(pFlyRadioInfo->radioInfo.eCurRadioMode);
 }

 static void RegDataReadRadio(void)
 {
	 DBG2(debugString("\nTDA7541 Radio read the freq last save !");)

	 pFlyRadioInfo->radioInfo.iPreRadioFreqFM1 = pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.dwFreqFM1;
	 if ((pFlyRadioInfo->radioInfo.iPreRadioFreqFM1 > 10800) || (pFlyRadioInfo->radioInfo.iPreRadioFreqFM1 < 8750))
	 {
		 pFlyRadioInfo->radioInfo.iPreRadioFreqFM1 = 8750;
	 }
	 pFlyRadioInfo->radioInfo.iPreRadioFreqFM2 = pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.dwFreqFM2;
	 if ((pFlyRadioInfo->radioInfo.iPreRadioFreqFM2 > 10800) || (pFlyRadioInfo->radioInfo.iPreRadioFreqFM2 < 8750))
	 {
		 pFlyRadioInfo->radioInfo.iPreRadioFreqFM2 = 8750;
	 }
	 pFlyRadioInfo->radioInfo.iPreRadioFreqAM = pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.dwFreqAM;
	 if ((pFlyRadioInfo->radioInfo.iPreRadioFreqAM > 1620) || (pFlyRadioInfo->radioInfo.iPreRadioFreqAM < 522))
	 {
		 pFlyRadioInfo->radioInfo.iPreRadioFreqAM = 522;
	 }

	 //DBG2(debugOneData("\nTDA7541 Radio read freq FM1---->",pFlyRadioInfo->radioInfo.iPreRadioFreqFM1);)
	 //DBG2(debugOneData("\nTDA7541 Radio read freq FM2---->",pFlyRadioInfo->radioInfo.iPreRadioFreqFM2);)
	 //DBG2(debugOneData("\nTDA7541 Radio read freq AM---->",pFlyRadioInfo->radioInfo.iPreRadioFreqAM);)
 }

 static void RegDataWriteRadio(void)
 {
	 //DBG2(debugString("\nTDA7541 Radio write the freq for next read !");)

	 pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.dwFreqFM1 = pFlyRadioInfo->radioInfo.iPreRadioFreqFM1;
	 pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.dwFreqFM2 = pFlyRadioInfo->radioInfo.iPreRadioFreqFM2;
	 pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.dwFreqAM = pFlyRadioInfo->radioInfo.iPreRadioFreqAM;

	 //DBG2(debugOneData("\nTDA7541 Radio write freq FM1---->",pFlyRadioInfo->radioInfo.iPreRadioFreqFM1);)
	 //DBG2(debugOneData("\nTDA7541 Radio write freq FM2---->",pFlyRadioInfo->radioInfo.iPreRadioFreqFM2);)
	 //DBG2(debugOneData("\nTDA7541 Radio write freq AM---->",pFlyRadioInfo->radioInfo.iPreRadioFreqAM);)
 }

 static void radioIDChangePara(BYTE iID)
 {
	 DBG2(debugOneData("\nTDA7541 Radio ID---->",iID);)
	 if (0x00 == iID)//China
	 {
		 pFlyRadioInfo->radioInfo.iFreqFMMin = 8750;
		 pFlyRadioInfo->radioInfo.iFreqFMMax = 10800;
		 pFlyRadioInfo->radioInfo.iFreqFMManualStep = 5;
		 pFlyRadioInfo->radioInfo.iFreqFMScanStep = 10;

		 pFlyRadioInfo->radioInfo.iFreqAMMin = 522;
		 pFlyRadioInfo->radioInfo.iFreqAMMax = 1620;
		 pFlyRadioInfo->radioInfo.iFreqAMManualStep = 9;
		 pFlyRadioInfo->radioInfo.iFreqAMScanStep = 9;

		 pFlyRadioInfo->radioInfo.iPreRadioFreqFM1 = 8750;
		 /*******************************************************/
		 //pTDA7541RadioInfo->radioInfo.iCurRadioFreqFM1 = 8750;
		 /*******************************************************/
		 pFlyRadioInfo->radioInfo.iPreRadioFreqFM2 = 8750;
		 /*******************************************************/
		 //pTDA7541RadioInfo->radioInfo.iCurRadioFreqFM2 = 8750;
		 /*******************************************************/
		 pFlyRadioInfo->radioInfo.iPreRadioFreqAM = 522;
		 /*******************************************************/
		 //pTDA7541RadioInfo->radioInfo.iCurRadioFreqAM = 522;
		 /*******************************************************/
	 }
	 else if (0x01 == iID)//USA
	 {
		 pFlyRadioInfo->radioInfo.iFreqFMMin = 8750;
		 pFlyRadioInfo->radioInfo.iFreqFMMax = 10790;
		 pFlyRadioInfo->radioInfo.iFreqFMManualStep = 5;
		 pFlyRadioInfo->radioInfo.iFreqFMScanStep = 20;

		 pFlyRadioInfo->radioInfo.iFreqAMMin = 530;
		 pFlyRadioInfo->radioInfo.iFreqAMMax = 1720;
		 pFlyRadioInfo->radioInfo.iFreqAMManualStep = 10;
		 pFlyRadioInfo->radioInfo.iFreqAMScanStep = 10;

		 pFlyRadioInfo->radioInfo.iPreRadioFreqFM1 = 8750;
		 /*******************************************************/
		 //pTDA7541RadioInfo->radioInfo.iCurRadioFreqFM1 = 8750;
		 /*******************************************************/
		 pFlyRadioInfo->radioInfo.iPreRadioFreqFM2 = 8750;
		 /*******************************************************/
		 //pTDA7541RadioInfo->radioInfo.iCurRadioFreqFM2 = 8750;
		 /*******************************************************/
		 pFlyRadioInfo->radioInfo.iPreRadioFreqAM = 530;
		 /*******************************************************/
		 //pTDA7541RadioInfo->radioInfo.iCurRadioFreqAM = 530;
		 /*******************************************************/
	 }
	 else if (0x02 == iID)//S.Amer-1
	 {
		 pFlyRadioInfo->radioInfo.iFreqFMMin = 8750;
		 pFlyRadioInfo->radioInfo.iFreqFMMax = 10800;
		 pFlyRadioInfo->radioInfo.iFreqFMManualStep = 10;
		 pFlyRadioInfo->radioInfo.iFreqFMScanStep = 10;

		 pFlyRadioInfo->radioInfo.iFreqAMMin = 530;
		 pFlyRadioInfo->radioInfo.iFreqAMMax = 1710;
		 pFlyRadioInfo->radioInfo.iFreqAMManualStep = 10;
		 pFlyRadioInfo->radioInfo.iFreqAMScanStep = 10;

		 pFlyRadioInfo->radioInfo.iPreRadioFreqFM1 = 8750;
		 /*******************************************************/
		 //pTDA7541RadioInfo->radioInfo.iCurRadioFreqFM1 = 8750;
		 /*******************************************************/
		 pFlyRadioInfo->radioInfo.iPreRadioFreqFM2 = 8750;
		 /*******************************************************/
		 //pTDA7541RadioInfo->radioInfo.iCurRadioFreqFM2 = 8750;
		 /*******************************************************/
		 pFlyRadioInfo->radioInfo.iPreRadioFreqAM = 530;
		 /*******************************************************/
		 //pTDA7541RadioInfo->radioInfo.iCurRadioFreqAM = 530;
		 /*******************************************************/
	 }
	 else if (0x03 == iID)//S.Amer-2
	 {
		 pFlyRadioInfo->radioInfo.iFreqFMMin = 8750;
		 pFlyRadioInfo->radioInfo.iFreqFMMax = 10800;
		 pFlyRadioInfo->radioInfo.iFreqFMManualStep = 10;
		 pFlyRadioInfo->radioInfo.iFreqFMScanStep = 10;

		 pFlyRadioInfo->radioInfo.iFreqAMMin = 520;
		 pFlyRadioInfo->radioInfo.iFreqAMMax = 1600;
		 pFlyRadioInfo->radioInfo.iFreqAMManualStep = 5;
		 pFlyRadioInfo->radioInfo.iFreqAMScanStep = 5;

		 pFlyRadioInfo->radioInfo.iPreRadioFreqFM1 = 8750;
		 /*******************************************************/
		 //pTDA7541RadioInfo->radioInfo.iCurRadioFreqFM1 = 8750;
		 /*******************************************************/
		 pFlyRadioInfo->radioInfo.iPreRadioFreqFM2 = 8750;
		 /*******************************************************/
		 //pTDA7541RadioInfo->radioInfo.iCurRadioFreqFM2 = 8750;
		 /*******************************************************/
		 pFlyRadioInfo->radioInfo.iPreRadioFreqAM = 520;
		 /*******************************************************/
		 //pTDA7541RadioInfo->radioInfo.iCurRadioFreqAM = 520;
		 /*******************************************************/
	 }
	 else if (0x04 == iID)//KOREA
	 {
		 pFlyRadioInfo->radioInfo.iFreqFMMin = 8810;
		 pFlyRadioInfo->radioInfo.iFreqFMMax = 10790;
		 pFlyRadioInfo->radioInfo.iFreqFMManualStep = 5;
		 pFlyRadioInfo->radioInfo.iFreqFMScanStep = 20;

		 pFlyRadioInfo->radioInfo.iFreqAMMin = 531;
		 pFlyRadioInfo->radioInfo.iFreqAMMax = 1620;
		 pFlyRadioInfo->radioInfo.iFreqAMManualStep = 9;
		 pFlyRadioInfo->radioInfo.iFreqAMScanStep = 9;

		 pFlyRadioInfo->radioInfo.iPreRadioFreqFM1 = 8810;
		 /*******************************************************/
		 //pTDA7541RadioInfo->radioInfo.iCurRadioFreqFM1 = 8810;
		 /*******************************************************/
		 pFlyRadioInfo->radioInfo.iPreRadioFreqFM2 = 8810;
		 /*******************************************************/
		 //pTDA7541RadioInfo->radioInfo.iCurRadioFreqFM2 = 8810;
		 /*******************************************************/
		 pFlyRadioInfo->radioInfo.iPreRadioFreqAM = 531;
		 /*******************************************************/
		 //pTDA7541RadioInfo->radioInfo.iCurRadioFreqAM = 531;
		 /*******************************************************/
	 }
	 else if (0x05 == iID)//Thailand
	 {
		 pFlyRadioInfo->radioInfo.iFreqFMMin = 8750;
		 pFlyRadioInfo->radioInfo.iFreqFMMax = 10800;
		 pFlyRadioInfo->radioInfo.iFreqFMManualStep = 5;
		 pFlyRadioInfo->radioInfo.iFreqFMScanStep = 25;

		 pFlyRadioInfo->radioInfo.iFreqAMMin = 531;
		 pFlyRadioInfo->radioInfo.iFreqAMMax = 1602;
		 pFlyRadioInfo->radioInfo.iFreqAMManualStep = 9;
		 pFlyRadioInfo->radioInfo.iFreqAMScanStep = 9;

		 pFlyRadioInfo->radioInfo.iPreRadioFreqFM1 = 8750;
		 /*******************************************************/
		 //pTDA7541RadioInfo->radioInfo.iCurRadioFreqFM1 = 8750;
		 /*******************************************************/
		 pFlyRadioInfo->radioInfo.iPreRadioFreqFM2 = 8750;
		 /*******************************************************/
		 //pTDA7541RadioInfo->radioInfo.iCurRadioFreqFM2 = 8750;
		 /*******************************************************/
		 pFlyRadioInfo->radioInfo.iPreRadioFreqAM = 531;
		 /*******************************************************/
		 //pTDA7541RadioInfo->radioInfo.iCurRadioFreqAM = 531;
		 /*******************************************************/
	 }
	 if (AM == pFlyRadioInfo->radioInfo.ePreRadioMode)
	 {
		 returnRadioFreq(pFlyRadioInfo->radioInfo.iPreRadioFreqAM);
	 }
	 else if (FM1 == pFlyRadioInfo->radioInfo.ePreRadioMode)
	 {
		 returnRadioFreq(pFlyRadioInfo->radioInfo.iPreRadioFreqFM1);
	 }
	 else if (FM2 == pFlyRadioInfo->radioInfo.ePreRadioMode)
	 {
		 returnRadioFreq(pFlyRadioInfo->radioInfo.iPreRadioFreqFM2);
	 }
 }

 static void powerOnNormalInit(void)
 {
	 pFlyRadioInfo->bPreMute = FALSE;
	 pFlyRadioInfo->bCurMute = TRUE;

	 radioIDChangePara(pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iRadioIDDriver);

	 RegDataReadRadio();

	 pFlyRadioInfo->radioInfo.ePreRadioMode = FM1;
	 pFlyRadioInfo->radioInfo.eCurRadioMode = FM1;

	 pFlyRadioInfo->radioInfo.pPreRadioFreq = &pFlyRadioInfo->radioInfo.iPreRadioFreqFM1;
	 pFlyRadioInfo->radioInfo.pCurRadioFreq = &pFlyRadioInfo->radioInfo.iCurRadioFreqFM1;

	 pFlyRadioInfo->radioInfo.bPreScaning = FALSE;
	 pFlyRadioInfo->radioInfo.bCurScaning = FALSE;
	 pFlyRadioInfo->radioInfo.eScanDirection = STEP_FORWARD;	
	 pFlyRadioInfo->radioInfo.bScanRepeatFlag = FALSE;

	 pFlyRadioInfo->radioInfo.bPreStepButtomDown = FALSE;
	 /*********************************************************/
	 //pTDA7541RadioInfo->radioInfo.bCurStepButtomDown = FALSE;
	 /*********************************************************/
	 pFlyRadioInfo->radioInfo.eButtomStepDirection = STEP_FORWARD;
	 pFlyRadioInfo->radioInfo.iButtomStepCount = 0;

	 pFlyRadioInfo->RDSInfo.RadioRDSAFControlOn = FALSE;
	 pFlyRadioInfo->RDSInfo.RadioRDSTAControlOn = FALSE;

	 memcpy(pFlyRadioInfo->mParameterTable,DefaultData,INIT_DATA_SIZE);

	 /*********************************************************/
	 //IO_Control_Init(pTDA7541RadioInfo);
	 /*********************************************************/

	 //DBG2(debugOneData("\nTDA7541 Radio (TDA7541RadioParaInit) iCurRadioFreqFM1---->",pFlyRadioInfo->radioInfo.iCurRadioFreqFM1);)
	 //DBG2(debugOneData("\nTDA7541 Radio (TDA7541RadioParaInit) iCurRadioFreqFM2---->",pFlyRadioInfo->radioInfo.iCurRadioFreqFM2);)
	 //DBG2(debugOneData("\nTDA7541 Radio (TDA7541RadioParaInit) iCurRadioFreqAM---->",pFlyRadioInfo->radioInfo.iCurRadioFreqAM);)
	// DBG2(debugOneData("\nTDA7541 Radio (TDA7541RadioParaInit) pCurRadioFreq---->",*pFlyRadioInfo->radioInfo.pCurRadioFreq);)
 }

 static void powerOnFirstInit()
 {
	 pFlyRadioInfo->bPowerUp = FALSE;
	 pFlyRadioInfo->bNeedInit = TRUE;

	 pFlyRadioInfo->bKillRadioMainThread = TRUE;
	 sem_init(&pFlyRadioInfo->MainThread_sem, 0, 0);

	 pFlyRadioInfo->bKillRadioScanThread = TRUE;
	 pthread_mutex_init(&pFlyRadioInfo->ScanThreadMutex, NULL);
	 pthread_cond_init(&pFlyRadioInfo->ScanThreadCond, NULL);

	 pFlyRadioInfo->bScanThreadRunAgain = FALSE;

	 pFlyRadioInfo->bKillRadioRDSRecThread = TRUE;
	 pthread_mutex_init(&pFlyRadioInfo->RDSRecThreadMutex, NULL);
	 pthread_cond_init(&pFlyRadioInfo->RDSRecThreadCond, NULL);
 }

 /******************************************************************************/
 /*                                各种线程函数                            */
 /******************************************************************************/
 void *radio_main_thread(void *arg)
 {
	 static UINT RadioScanStatus;

	 /********************************************************************/
	 //DBG1(RETAILMSG(1, (TEXT("\r\nFlyAudio TDA7541RadioMainThread")));)
	 /********************************************************************/

	 while (!pFlyRadioInfo->bKillRadioMainThread)
	 {
		 sem_wait(&pFlyRadioInfo->MainThread_sem);

		 DBG2(debugString("\nTDA7541 Radio MainThread Running!");)


		 if (FALSE == pFlyRadioInfo->bPowerUp)
		 {
		 }
		 else
		 {
			 if (pFlyRadioInfo->bNeedInit)
			 {
				 pFlyAllInOneInfo->pMemory_Share_Common->iNeedProcVoltageShakeRadio = 85;

				 ParameterTable_Init();

				 TDA7541Radio_ChangeToFMAM(pFlyRadioInfo->radioInfo.eCurRadioMode);

				 radioAFMuteControl(FALSE);

				 //TDA7541RadioJumpNewFreqParaInit();
				 pFlyRadioInfo->bNeedInit = FALSE;
			 }

		 if (ipcWhatEventOn(EVENT_GLOBAL_RADIO_CHANGE_ID))
		 {
			 ipcClearEvent(EVENT_GLOBAL_RADIO_CHANGE_ID);
			 DBG0(debugString("\nTDA7541 Radio ID Change!");)
			 radioIDChangePara(pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iRadioIDUser);
		 }

		 if (ipcWhatEventOn(EVENT_GLOBAL_BATTERY_RECOVERY_RADIO_ID))
		 {
			 DBG2(debugString("\nTDA7541 Radio Voltage After Low Proc");)
			 ipcClearEvent(EVENT_GLOBAL_BATTERY_RECOVERY_RADIO_ID);
			 recoveryRadioRegData();
		 }

		 if (ipcWhatEventOn(EVENT_GLOBAL_RADIO_ANTENNA_ID))
		 {
			 DBG2(debugString("\nTDA7541 Radio ANTENNA ctrl");)
			 ipcClearEvent(EVENT_GLOBAL_RADIO_ANTENNA_ID);
			 if (pFlyAllInOneInfo->pMemory_Share_Common->eAudioInput == RADIO)
			 {
				 radioANTControl(TRUE);
			 }
			 else
			 {
				 radioANTControl(FALSE);
			 }
		 }
		 PostSignal(&pFlyRadioInfo->ScanThreadMutex,&pFlyRadioInfo->ScanThreadCond,&pFlyRadioInfo->bScanThreadRunAgain);
	 
		 }
	 }
	 DBG2(debugString("\nTDA7541 Radio MainThread Prepare exit!");)

	 pFlyRadioInfo->bPowerUp = FALSE;
	 pFlyAllInOneInfo->pMemory_Share_Common->iNeedProcVoltageShakeRadio = 0;

	 DBG2(debugString("\nTDA7541 Radio MainThread exit!");)
	 return NULL;
 }

 void *radio_scan_thread(void *arg)
 {
	 UINT iScanStartFreq = 0;
	 UINT m_RF_Freq = 0;
	 BOOL bHaveSearched = 0;
	 UINT iHaveSearchedLevel = 0;
	 UINT iTempHaveSearchedLevel = 0;
	 ULONG nowTimer = 0;
	 ULONG lastTimer = 0;
	 ULONG iThreadFreqStepWaitTime = 0;

	 BYTE iBlinkingTimes = 0;
	 BOOL bBlinkingStatus = 0;

	 DBG2(debugString("\nTDA7541 Radio ScanThread in");)
	 while (!pFlyRadioInfo->bKillRadioScanThread)
	 {
		 WaitSignedTimeOut(&pFlyRadioInfo->ScanThreadMutex,&pFlyRadioInfo->ScanThreadCond,&pFlyRadioInfo->bScanThreadRunAgain,iThreadFreqStepWaitTime);
		 iThreadFreqStepWaitTime = 0;
		 debugString("TDA7541 Radio Scan Thread Running");

		 debugOneData("\nbCurScaning---->",pFlyRadioInfo->radioInfo.bCurScaning);
		 debugOneData("\nbPreScaning---->",pFlyRadioInfo->radioInfo.bPreScaning);

		 if (pFlyRadioInfo->radioInfo.bCurScaning == TRUE && pFlyRadioInfo->radioInfo.bPreScaning == FALSE)//跳出搜台
		 {
			 DBG2(debugString("\nTDA7541 Radio ScanThread --------------exit scaning");)
			 pFlyRadioInfo->radioInfo.bCurScaning = pFlyRadioInfo->radioInfo.bPreScaning;
			 /*********************************************************************************/
			 //TDA7541_UpdateInitTblBit(&FM_Test[1],TDA7541_ADDR25_44,backup);//resume the 4th Bit of addr.25.
			 /*********************************************************************************/
			 TDA7541_WriteTunerRegister(TDA7541_ADDR25_REG, &FM_Test[1]);
			 /*********************************************************************************/
			 //TDA7541_UpdateInitTblBit(&pTDA7541RadioInfo->mParameterTable[FM_REG_OFFSET +10],TDA7541_ADDR10_REG_MUX,mux);
			 /*********************************************************************************/	
			 TDA7541_WriteTunerRegister(TDA7541_ADDR10_REG, &pFlyRadioInfo->mParameterTable[FM_REG_OFFSET +10]);//write
			 if(pFlyRadioInfo->radioInfo.eCurRadioMode != AM)		// exit seek, restore previos value
			 {
				 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+0], TDA7541_ADDR0_REG_SEEK, TDA7541_SEEK_OFF); //resume the 7th Bit of addr.0 under FM mode.
				 TDA7541_WriteTunerRegister(TDA7541_ADDR0_REG, &pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+0]);
			 }
			 else
			 {
				 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[AM_REG_OFFSET+0], TDA7541_ADDR0_REG_SEEK, TDA7541_SEEK_OFF); //resume the 7th Bit of addr.0 under AM mode.
				 TDA7541_WriteTunerRegister(TDA7541_ADDR0_REG, &pFlyRadioInfo->mParameterTable[AM_REG_OFFSET+0]);
			 }
			 returnRadioFreq(*pFlyRadioInfo->radioInfo.pPreRadioFreq);
			 returnRadioScanCtrl(0x03);//stop

			 pFlyRadioInfo->bCurMute = !pFlyRadioInfo->bPreMute;
		 }
		 //设置波段
		 if (pFlyRadioInfo->radioInfo.eCurRadioMode != pFlyRadioInfo->radioInfo.ePreRadioMode)
		 {
			 TDA7541_Radio_Mute(TRUE);
			 Sleep(10);
			 //DBG2(debugString("\nTDA7541 Radio ScanThread --------------change mode");)
			 while (ipcWhatEventOn(EVENT_GLOBAL_RADIO_MUTE_IN_REQ_ID) || ipcWhatEventOn(EVENT_GLOBAL_RADIO_MUTE_OUT_REQ_ID))
			 {
				 Sleep(10);
			 }
			 if (pFlyAllInOneInfo->pMemory_Share_Common->bAudioMuteControlable)
			 {
				 ipcStartEvent(EVENT_GLOBAL_RADIO_MUTE_IN_REQ_ID);//发送进入静音
				 while (!ipcWhatEventOn(EVENT_GLOBAL_RADIO_MUTE_IN_OK_ID))//等待OK
				 {
					 Sleep(10);
				 }
				 ipcClearEvent(EVENT_GLOBAL_RADIO_MUTE_IN_OK_ID);//清除
			 }

			 pFlyRadioInfo->radioInfo.eCurRadioMode = pFlyRadioInfo->radioInfo.ePreRadioMode;			
			 if (FM1 == pFlyRadioInfo->radioInfo.eCurRadioMode)
			 {
				 pFlyRadioInfo->radioInfo.pPreRadioFreq = &pFlyRadioInfo->radioInfo.iPreRadioFreqFM1;
				 pFlyRadioInfo->radioInfo.pCurRadioFreq = &pFlyRadioInfo->radioInfo.iCurRadioFreqFM1;
			 }
			 else if (FM2 == pFlyRadioInfo->radioInfo.eCurRadioMode)
			 {
				 pFlyRadioInfo->radioInfo.pPreRadioFreq = &pFlyRadioInfo->radioInfo.iPreRadioFreqFM2;
				 pFlyRadioInfo->radioInfo.pCurRadioFreq = &pFlyRadioInfo->radioInfo.iCurRadioFreqFM2;
			 }
			 else if (AM == pFlyRadioInfo->radioInfo.eCurRadioMode)
			 {
				 pFlyRadioInfo->radioInfo.pPreRadioFreq = &pFlyRadioInfo->radioInfo.iPreRadioFreqAM;
				 pFlyRadioInfo->radioInfo.pCurRadioFreq = &pFlyRadioInfo->radioInfo.iCurRadioFreqAM;
			 }
			 TDA7541Radio_ChangeToFMAM(pFlyRadioInfo->radioInfo.eCurRadioMode);

			 TDA7541RadioJumpNewFreqParaInit();

			 if (pFlyRadioInfo->bPreMute == FALSE)
			 {
				 TDA7541_Radio_Mute(FALSE);//取消静音
			 }

			 if (pFlyAllInOneInfo->pMemory_Share_Common->bAudioMuteControlable)
			 {
				 //if (AM == pFlyRadioInfo->radioInfo.eCurRadioMode)
				 //{
					// Sleep(314);
				 //}

				 Sleep(314);

				 ipcStartEvent(EVENT_GLOBAL_RADIO_MUTE_OUT_REQ_ID);//发送退出静音

			 }
		 }
		 //设置频率
		 if (*pFlyRadioInfo->radioInfo.pCurRadioFreq != *pFlyRadioInfo->radioInfo.pPreRadioFreq)
		 {
			 //DBG2(debugString("\nTDA7541 Radio ScanThread --------------change freq");)
			 TDA7541_Radio_Mute(TRUE);
			 /**************************************************************************/
			 //if (pTDA7541RadioInfo->pFlyDriverGlobalInfo->FlySystemRunningInfo.bAudioMuteControlable)
			 //{
			 //	eventInterSetEvent(pTDA7541RadioInfo,EVENT_GLOBAL_RADIO_MUTE_IN_REQ_ID);//发送进入静音
			 //	while (!eventInterWhatEventOn(pTDA7541RadioInfo,EVENT_GLOBAL_RADIO_MUTE_IN_OK_ID))//等待OK
			 //	{
			 //		Sleep(100);
			 //	}
			 //	eventInterClrEvent(pTDA7541RadioInfo,EVENT_GLOBAL_RADIO_MUTE_IN_OK_ID);//清除
			 //}
		     /**************************************************************************/
			 *pFlyRadioInfo->radioInfo.pCurRadioFreq = *pFlyRadioInfo->radioInfo.pPreRadioFreq;
			 Set_Freq_7541(pFlyRadioInfo->radioInfo.eCurRadioMode,*pFlyRadioInfo->radioInfo.pCurRadioFreq);
			 /**************************************************************************/
			 //returnRadioFreq(pTDA7541RadioInfo,*(pTDA7541RadioInfo->radioInfo.pCurRadioFreq));
			 /**************************************************************************/
			 TDA7541RadioJumpNewFreqParaInit();
			 if (pFlyRadioInfo->bPreMute == FALSE)
			 {
				 TDA7541_Radio_Mute(FALSE);//取消静音
			 }
			 
			 //RegDataWriteRadio();
		 }

		 if(pFlyRadioInfo->bPreMute != pFlyRadioInfo->bCurMute)//收音机静音开关
		 {
			 if (pFlyRadioInfo->bPreMute == TRUE)
			 {
				 TDA7541_Radio_Mute(TRUE); // mute
			 }
			 else
			 {
				 TDA7541_Radio_Mute(FALSE);// demute
			 }
			 pFlyRadioInfo->bCurMute = pFlyRadioInfo->bPreMute;
		 }

		 if (pFlyRadioInfo->radioInfo.bPreStepButtomDown)//按下，则持续跳台
		 {
			 if (0 == pFlyRadioInfo->radioInfo.iButtomStepCount)
			 {
				 pFlyRadioInfo->radioInfo.iButtomStepCount++;
				 iThreadFreqStepWaitTime = 314;
			 }
			 else
			 {
				 buttomJumpFreqAndReturn();
				 pFlyRadioInfo->radioInfo.iButtomStepCount++;
				 iThreadFreqStepWaitTime = 100;
			 }
			 continue;//跳到开头
		 }
		 nowTimer = GetTickCount();
		 lastTimer = nowTimer;
		 while(pFlyRadioInfo->radioInfo.bPreScaning)//搜索
		 {
			 nowTimer = GetTickCount();
			 DBG2(debugString("\nTDA7541 Radio ScanThread --------------scaning ....");)
			 /******************************************************************************/
			 //FMTHVal=GetSSTOPPinLevel(TDA7541_GetTunerParameter(pTDA7541RadioInfo->mParameterTable[FM_REG_OFFSET +10],TDA7541_ADDR10_REG_SSTH));//get seek aligned value.
			 //mux=TDA7541_GetTunerParameter(pTDA7541RadioInfo->mParameterTable[FM_REG_OFFSET +10],TDA7541_ADDR10_REG_MUX);	//backup
			 //TDA7541_UpdateInitTblBit(&pTDA7541RadioInfo->mParameterTable[FM_REG_OFFSET +10],TDA7541_ADDR10_REG_MUX,2);	//set
			 /******************************************************************************/
			 TDA7541_WriteTunerRegister(TDA7541_ADDR10_REG, &pFlyRadioInfo->mParameterTable[FM_REG_OFFSET +10]);//write
			 /******************************************************************************/
			 //backup=TDA7541_GetTunerParameter(FM_Test[1],TDA7541_ADDR25_44);//Backup the 4th Bit of addr.25.
			 //TDA7541_UpdateInitTblBit(&FM_Test[1],TDA7541_ADDR25_44,1);//Force set 1 into its 4th Bit when seeking.
			 /******************************************************************************/
			 TDA7541_WriteTunerRegister(TDA7541_ADDR25_REG, &FM_Test[1]);//write	

			 TDA7541_Radio_Mute(TRUE); // mute			
			 if(AM != pFlyRadioInfo->radioInfo.eCurRadioMode)
			 {	
				 /******************************************************************************/
				 //seek =TDA7541_GetTunerParameter(pTDA7541RadioInfo->mParameterTable[FM_REG_OFFSET+0],TDA7541_ADDR0_REG_SEEK);//Backup the 7th Bit of addr.0.
				 /******************************************************************************/
				 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+0], TDA7541_ADDR0_REG_SEEK, TDA7541_SEEK_ON); //Force set 1 into its 7th Bit when seeking.
				 TDA7541_WriteTunerRegister(TDA7541_ADDR0_REG, &pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+0]);
				 /******************************************************************************/
				 //temp = pTDA7541RadioInfo->mParameterTable[FM_REG_OFFSET+16];
				 //temp &= 0x3f;
				 //TDA7541_WriteTunerRegister(pTDA7541RadioInfo,TDA7541_ADDR16_REG, &temp);
				 /******************************************************************************/
				 m_RF_Freq = (*pFlyRadioInfo->radioInfo.pPreRadioFreq)*10;
			 }
			 else
			 {
				 /******************************************************************************/
				 //seek =TDA7541_GetTunerParameter(pTDA7541RadioInfo->mParameterTable[AM_REG_OFFSET+0],TDA7541_ADDR0_REG_SEEK);//Backup the 7th Bit of addr.0.
				 /******************************************************************************/
				 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[AM_REG_OFFSET+0], TDA7541_ADDR0_REG_SEEK, TDA7541_SEEK_ON); //Force set 1 into its 7th Bit when seeking.
				 TDA7541_WriteTunerRegister(TDA7541_ADDR0_REG, &pFlyRadioInfo->mParameterTable[AM_REG_OFFSET+0]);
				 /******************************************************************************/
				 //temp = pTDA7541RadioInfo->mParameterTable[AM_REG_OFFSET+16];
				 //temp &= 0x3f;
				 //TDA7541_WriteTunerRegister(pTDA7541RadioInfo,TDA7541_ADDR16_REG, &temp);
				 /******************************************************************************/
				 m_RF_Freq = *pFlyRadioInfo->radioInfo.pPreRadioFreq;
			 }

			 if(m_RF_Freq>=87500)//EU/WB
			 {
				 TDA7541_WriteTunerRegister(TDA7541_ADDR10_REG, &pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+10]);//load aligned data of SSTH from eeprom to make sure that seek stop correctly
			 }
			 else if(m_RF_Freq>=64000&&m_RF_Freq<=74000)//OIRT
			 {
				 /*****************************************************************************/
				 //TDA7541_WriteTunerRegister(pTDA7541RadioInfo,TDA7541_ADDR10_REG, &pTDA7541RadioInfo->mParameterTable[OIRT_SD_Offset]);
				 /*****************************************************************************/
			 }
			 else if(m_RF_Freq<=19244)//MW LW SW
			 {
				 TDA7541_WriteTunerRegister(TDA7541_ADDR10_REG, &pFlyRadioInfo->mParameterTable[AM_REG_OFFSET+10]);//load aligned data of SSTH from eeprom to make sure that seek stop correctly
			 }
			 //

			 if (pFlyRadioInfo->radioInfo.bPreScaning == FALSE)
			 {
				 pFlyRadioInfo->radioInfo.bCurScaning = TRUE;
			 }
			 else
			 {
				 if (pFlyRadioInfo->radioInfo.bCurScaning != pFlyRadioInfo->radioInfo.bPreScaning)//起始搜索频率
				 {
					 pFlyRadioInfo->radioInfo.bCurScaning = pFlyRadioInfo->radioInfo.bPreScaning;
					 iScanStartFreq = GetCorrectScanStartFreq(pFlyRadioInfo->radioInfo.pPreRadioFreq);
					 bHaveSearched = FALSE;
					 iHaveSearchedLevel = 0;
				 }
					
				 DBG0(debugOneData("\nTDA7541 Radio ScanThread iScanStartFreq---->",iScanStartFreq);)
				 DBG0(debugOneData("\nTDA7541 Radio ScanThread PreRadioFreq---->",*pFlyRadioInfo->radioInfo.pPreRadioFreq);)
				 *pFlyRadioInfo->radioInfo.pPreRadioFreq = RadioStepFreqGenerate(pFlyRadioInfo->radioInfo.eCurRadioMode
				 ,*pFlyRadioInfo->radioInfo.pPreRadioFreq
				 ,pFlyRadioInfo->radioInfo.eScanDirection
				 ,STEP_SCAN);

				 *pFlyRadioInfo->radioInfo.pCurRadioFreq = *pFlyRadioInfo->radioInfo.pPreRadioFreq;
				 Set_Freq_7541(pFlyRadioInfo->radioInfo.eCurRadioMode,*pFlyRadioInfo->radioInfo.pCurRadioFreq);
				 if (iScanStartFreq == *pFlyRadioInfo->radioInfo.pPreRadioFreq )//一圈都没好台
				 {
					 pFlyRadioInfo->radioInfo.bPreScaning = FALSE;
					 pFlyRadioInfo->radioInfo.bCurScaning = TRUE;
					 returnRadioFreq(*pFlyRadioInfo->radioInfo.pPreRadioFreq);
					 returnRadioScanCtrl(0x03);//stop
					 if (pFlyRadioInfo->bPreMute == FALSE)//收到台要出声音
					 {
						 TDA7541_Radio_Mute(FALSE);
					 }
				 }
				 if ((nowTimer - lastTimer) >  157)//定时返回频点
				 {
					 lastTimer = nowTimer;
					 if (pFlyRadioInfo->radioInfo.bPreScaning)
					 {
						 returnRadioFreq(*pFlyRadioInfo->radioInfo.pPreRadioFreq);
					 }
				 }
				 if (AM != pFlyRadioInfo->radioInfo.eCurRadioMode)
				 {
					 Sleep(10);
				 } 
				 else
				 {
					 Sleep(138);
				 }

				 if (bRadioSignalGood(pFlyRadioInfo->radioInfo.eCurRadioMode,&iTempHaveSearchedLevel))
				 {
					 bHaveSearched = TRUE;
					 if (iHaveSearchedLevel > iTempHaveSearchedLevel)//OK
					 {
						 if (STEP_BACKWARD == pFlyRadioInfo->radioInfo.eScanDirection)
						 {
							 *pFlyRadioInfo->radioInfo.pPreRadioFreq = RadioStepFreqGenerate(pFlyRadioInfo->radioInfo.eCurRadioMode
								 ,*pFlyRadioInfo->radioInfo.pPreRadioFreq
								 ,STEP_FORWARD
								 ,STEP_SCAN);
						 } 
						 else
						 {
							 *pFlyRadioInfo->radioInfo.pPreRadioFreq = RadioStepFreqGenerate(pFlyRadioInfo->radioInfo.eCurRadioMode
								 ,*pFlyRadioInfo->radioInfo.pPreRadioFreq
								 ,STEP_BACKWARD
								 ,STEP_SCAN);
						 }
						 *pFlyRadioInfo->radioInfo.pCurRadioFreq = *pFlyRadioInfo->radioInfo.pPreRadioFreq;
						 Set_Freq_7541(pFlyRadioInfo->radioInfo.eCurRadioMode,*pFlyRadioInfo->radioInfo.pCurRadioFreq);

						 if (pFlyRadioInfo->bPreMute == FALSE)//收到台要出声音
						 {
							 TDA7541_Radio_Mute(FALSE);
						 }
						 TDA7541_WriteTunerRegister(TDA7541_ADDR25_REG, &FM_Test[1]);
						 TDA7541_WriteTunerRegister(TDA7541_ADDR10_REG, &pFlyRadioInfo->mParameterTable[FM_REG_OFFSET +10]);//write
						 if(pFlyRadioInfo->radioInfo.eCurRadioMode != AM)		// exit seek, restore previos value
						 {
							 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+0], TDA7541_ADDR0_REG_SEEK, TDA7541_SEEK_OFF); //resume the 7th Bit of addr.0 under FM mode.
							 TDA7541_WriteTunerRegister(TDA7541_ADDR0_REG, &pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+0]);
						 }
						 else
						 {
							 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[AM_REG_OFFSET+0], TDA7541_ADDR0_REG_SEEK, TDA7541_SEEK_OFF); //resume the 7th Bit of addr.0 under AM mode.
							 TDA7541_WriteTunerRegister(TDA7541_ADDR0_REG, &pFlyRadioInfo->mParameterTable[AM_REG_OFFSET+0]);
						 }

						 returnRadioFreq(*pFlyRadioInfo->radioInfo.pPreRadioFreq);
						 returnRadioHaveSearched(TRUE);

						 if (pFlyRadioInfo->radioInfo.bScanRepeatFlag == FALSE)
						 {
							 pFlyRadioInfo->radioInfo.bPreScaning = FALSE;
							 pFlyRadioInfo->radioInfo.bCurScaning = FALSE;
							 returnRadioScanCtrl(0x03);//stop							
							 break;
						 }
						 else
						 {
							 /************************/
							 //blinking 5 times
							 /************************/
							 iBlinkingTimes = 0;
							 bBlinkingStatus = TRUE;
							 while (pFlyRadioInfo->radioInfo.bPreScaning == TRUE && iBlinkingTimes < 10)
							 {
								 Sleep(500);
								 iBlinkingTimes++;
								 bBlinkingStatus = !bBlinkingStatus;
								 returnRadioBlinkingStatus(bBlinkingStatus);							
							 }
							 returnRadioBlinkingStatus(TRUE);
							 Sleep(500);
							 if (pFlyRadioInfo->radioInfo.bPreScaning == TRUE)
							 {
								 returnRadioScanCtrl(0x04);//repeat
							 }

							 iScanStartFreq = *pFlyRadioInfo->radioInfo.pPreRadioFreq;
							 bHaveSearched = FALSE;
							 iHaveSearchedLevel = 0;
						 }
					 }				
				 }
				 else
				 {
					 if (bHaveSearched)
					 {
						 if (STEP_BACKWARD == pFlyRadioInfo->radioInfo.eScanDirection)
						 {
							 *pFlyRadioInfo->radioInfo.pPreRadioFreq = RadioStepFreqGenerate(pFlyRadioInfo->radioInfo.eCurRadioMode
								 ,*pFlyRadioInfo->radioInfo.pPreRadioFreq
								 ,STEP_FORWARD
								 ,STEP_SCAN);
						 } 
						 else
						 {
							 *pFlyRadioInfo->radioInfo.pPreRadioFreq = RadioStepFreqGenerate(pFlyRadioInfo->radioInfo.eCurRadioMode
								 ,*pFlyRadioInfo->radioInfo.pPreRadioFreq
								 ,STEP_BACKWARD
								 ,STEP_SCAN);
						 }
						 *pFlyRadioInfo->radioInfo.pCurRadioFreq = *pFlyRadioInfo->radioInfo.pPreRadioFreq;
						 Set_Freq_7541(pFlyRadioInfo->radioInfo.eCurRadioMode,*pFlyRadioInfo->radioInfo.pCurRadioFreq);

						 if (pFlyRadioInfo->bPreMute == FALSE)//收到台要出声音
						 {
							 TDA7541_Radio_Mute(FALSE);
						 }
						 /*******************************************************************/
						 //TDA7541_UpdateInitTblBit(&FM_Test[1],TDA7541_ADDR25_44,backup);//resume the 4th Bit of addr.25.
						 /*******************************************************************/
						 TDA7541_WriteTunerRegister(TDA7541_ADDR25_REG, &FM_Test[1]);
						 /*******************************************************************/
						 //TDA7541_UpdateInitTblBit(&pTDA7541RadioInfo->mParameterTable[FM_REG_OFFSET +10],TDA7541_ADDR10_REG_MUX,mux);
						 /*******************************************************************/
						 TDA7541_WriteTunerRegister(TDA7541_ADDR10_REG, &pFlyRadioInfo->mParameterTable[FM_REG_OFFSET +10]);//write
						 if(pFlyRadioInfo->radioInfo.eCurRadioMode != AM)		// exit seek, restore previos value
						 {
							 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+0], TDA7541_ADDR0_REG_SEEK, TDA7541_SEEK_OFF); //resume the 7th Bit of addr.0 under FM mode.
							 TDA7541_WriteTunerRegister(TDA7541_ADDR0_REG, &pFlyRadioInfo->mParameterTable[FM_REG_OFFSET+0]);
						 }
						 else
						 {
							 TDA7541_UpdateInitTblBit(&pFlyRadioInfo->mParameterTable[AM_REG_OFFSET+0], TDA7541_ADDR0_REG_SEEK, TDA7541_SEEK_OFF); //resume the 7th Bit of addr.0 under AM mode.
							 TDA7541_WriteTunerRegister(TDA7541_ADDR0_REG, &pFlyRadioInfo->mParameterTable[AM_REG_OFFSET+0]);
						 }

						 returnRadioFreq(*pFlyRadioInfo->radioInfo.pPreRadioFreq);
						 returnRadioHaveSearched(TRUE);

						 if (pFlyRadioInfo->radioInfo.bScanRepeatFlag == FALSE)
						 {
							 pFlyRadioInfo->radioInfo.bPreScaning = FALSE;
							 pFlyRadioInfo->radioInfo.bCurScaning = FALSE;
							 returnRadioScanCtrl(0x03);//stop							
							 break;
						 }
						 else
						 {
							 //blinking 5 times
							 iBlinkingTimes = 0;
							 bBlinkingStatus = TRUE;
							 while (pFlyRadioInfo->radioInfo.bPreScaning == TRUE && iBlinkingTimes < 10)
							 {
								 Sleep(500);
								 iBlinkingTimes++;
								 bBlinkingStatus = !bBlinkingStatus;
								 returnRadioBlinkingStatus(bBlinkingStatus);							
							 }
							 returnRadioBlinkingStatus(TRUE);
							 Sleep(500);
							 if (pFlyRadioInfo->radioInfo.bPreScaning == TRUE)
							 {
								 returnRadioScanCtrl(0x04);//repeat
							 }

							 iScanStartFreq = *pFlyRadioInfo->radioInfo.pPreRadioFreq;
							 bHaveSearched = FALSE;
							 iHaveSearchedLevel = 0;
						 }
					 }
				 }
				 iHaveSearchedLevel = iTempHaveSearchedLevel;
			 }
		 }

		 //if(pFlyRadioInfo->bPreMute != pFlyRadioInfo->bCurMute)//收音机静音开关
		 //{
			// if (pFlyRadioInfo->bPreMute == TRUE)
			// {
			//	 TDA7541_Radio_Mute(TRUE); // mute
			// }
			// else
			// {
			//	 TDA7541_Radio_Mute(FALSE);// demute
			// }
			// pFlyRadioInfo->bCurMute = pFlyRadioInfo->bPreMute;
		 //}

		 DBG0(debugString("\nTDA7541 Radio ScanThread Running End");)
	}
		DBG0(debugString("\nTDA7541 Radio ScanThread exit!");)
		return NULL;
 }

 /******************************************************************************/
 /*                                创建各种线程                            */
 /******************************************************************************/
 static INT create_thread(void)
 {
	 INT res;
	 pthread_t thread_id;

	 //创建线程 （1）线程ID，
	 //		    （2）线程属性
	 //		    （3）线程函数起始地址
	 //		    （4）线程函数的参数

	 //MAIN线程
	 pFlyRadioInfo->bKillRadioMainThread = FALSE;
	 res = pthread_create(&thread_id,NULL,radio_main_thread,NULL);
	 DBG0(debugOneData("\nTDA7541 Radio radio_main_thread ID---->",thread_id);)
	 if(res != 0) 
	 {
		 pFlyRadioInfo->bKillRadioMainThread = TRUE;
		 return -1;
	 }

	 //SCAN线程
	 pFlyRadioInfo->bKillRadioScanThread = FALSE;
	 res = pthread_create(&thread_id,NULL,radio_scan_thread,NULL);
	 DBG0(debugOneData("\nTDA7541 Radio radio_scan_thread ID---->",thread_id);)
	 if(res != 0) 
	 {
		 pFlyRadioInfo->bKillRadioScanThread = TRUE;
		 return -1;
	 }

	 //RDSREC线程
	 pFlyRadioInfo->bKillRadioRDSRecThread = FALSE;
	 res = pthread_create(&thread_id,NULL,radio_rdsrec_thread,NULL);
	 DBG0(debugOneData("\nTDA7541 Radio radio_scan_thread ID---->",thread_id);)
	 if(res != 0) 
	 {
		 pFlyRadioInfo->bKillRadioScanThread = TRUE;
		 return -1;
	 }

	 return 0;
 }

 /******************************************************************************/
 /*                                  处理数据                              */
 /******************************************************************************/
 static void DealRightDataProcessor(BYTE *buf, UINT16 len)
 {
	 UINT iTemp;

	 switch(buf[0])
	 {
	 case 0x01:
		 if (0x01 == buf[1])//初始化命令开始
			{
				DBG2(debugString("\nTDA7541 Radio hal init");)

				if (FALSE == pFlyRadioInfo->bPowerUp)
				{
					pFlyRadioInfo->bPowerUp = TRUE;
					pFlyRadioInfo->bNeedInit = TRUE;
				}

				returnRadioPowerMode(TRUE);
				returnRadioInitStatus(TRUE);

				sem_post(&pFlyRadioInfo->MainThread_sem);//激活一次
			}
		 else if (0x00 == buf[1])
			{
				returnRadioPowerMode(FALSE);
			}
		 break;
	 case 0x03://软件模拟按键 1-FM1 2-FM2 3-AM 4-STOP RADIO 5-AF 6-TA
		 switch(buf[1])
			{
		 case 0x01:
		 case 0x02:
		 case 0x03:
			 if (pFlyRadioInfo->radioInfo.bPreScaning)
				{
					pFlyRadioInfo->radioInfo.bPreScaning = FALSE;
				}
			 if (pFlyRadioInfo->bPowerUp)//直到之前的扫描停止
				{	
					sem_post(&pFlyRadioInfo->MainThread_sem);
					while (pFlyRadioInfo->radioInfo.bCurScaning)
					{
						Sleep(100);
					}
				}
			 if (0x01 == buf[1])
				{
					pFlyRadioInfo->radioInfo.ePreRadioMode = FM1;
					DBG2(debugString("\nTDA7541 Radio set mode --------FM1");)
				} 
			 else if (0x02 == buf[1])
				{
					pFlyRadioInfo->radioInfo.ePreRadioMode = FM2;
					DBG2(debugString("\nTDA7541 Radio set mode --------FM2");)
				}
			 else if (0x03 == buf[1])
				{
					pFlyRadioInfo->radioInfo.ePreRadioMode = AM;
					DBG2(debugString("\nTDA7541 Radio set mode --------AM");)
				}

			 returnRadioMode(pFlyRadioInfo->radioInfo.ePreRadioMode);
			 if (AM == pFlyRadioInfo->radioInfo.ePreRadioMode)
				{
					returnRadioFreq(pFlyRadioInfo->radioInfo.iPreRadioFreqAM);
				}
			 else if (FM1 == pFlyRadioInfo->radioInfo.ePreRadioMode)
				{
					returnRadioFreq(pFlyRadioInfo->radioInfo.iPreRadioFreqFM1);
				}
			 else if (FM2 == pFlyRadioInfo->radioInfo.ePreRadioMode)
				{
					returnRadioFreq(pFlyRadioInfo->radioInfo.iPreRadioFreqFM2);
				}

			 if (pFlyRadioInfo->bPowerUp)
				{	
					sem_post(&pFlyRadioInfo->MainThread_sem);
				}
			 break;
		 case 0x04:
			 pFlyRadioInfo->radioInfo.bPreScaning = !pFlyRadioInfo->radioInfo.bPreScaning;
			 if (pFlyRadioInfo->bPowerUp)
			{
				sem_post(&pFlyRadioInfo->MainThread_sem);
			}
			 break;
		 case 0x05:
			 pFlyRadioInfo->RDSInfo.RadioRDSAFControlOn = !pFlyRadioInfo->RDSInfo.RadioRDSAFControlOn;
			 returnRadioAFStatus(pFlyRadioInfo->RDSInfo.RadioRDSAFControlOn);
			 if (pFlyRadioInfo->bPowerUp)
			{
				sem_post(&pFlyRadioInfo->MainThread_sem);
			}
			break;
		 case 0x06:
			 pFlyRadioInfo->RDSInfo.RadioRDSTAControlOn = !pFlyRadioInfo->RDSInfo.RadioRDSTAControlOn;
			 returnRadioTAStatus(pFlyRadioInfo->RDSInfo.RadioRDSTAControlOn);
			 if (pFlyRadioInfo->bPowerUp)
			{
				sem_post(&pFlyRadioInfo->MainThread_sem);
			}
			break;
		 case 0x07:
			 if (pFlyRadioInfo->radioInfo.bPreScaning)
				{
					pFlyRadioInfo->radioInfo.bPreScaning = FALSE;
				}
			 if (pFlyRadioInfo->radioInfo.ePreRadioMode == pFlyRadioInfo->radioInfo.eCurRadioMode)
			 {
				 *pFlyRadioInfo->radioInfo.pPreRadioFreq = 
					 RadioStepFreqGenerate(pFlyRadioInfo->radioInfo.ePreRadioMode
					 ,*pFlyRadioInfo->radioInfo.pPreRadioFreq
					 ,STEP_FORWARD
					 ,STEP_MANUAL);
				 DBG2(debugOneData("\nTDA7541 Radio set freq value---->",*pFlyRadioInfo->radioInfo.pPreRadioFreq);)
					 returnRadioFreq(*pFlyRadioInfo->radioInfo.pPreRadioFreq);
				 if (pFlyRadioInfo->bPowerUp)
				 {
					 sem_post(&pFlyRadioInfo->MainThread_sem);
				 }
			 }
			 break;
		 case 0x08:
			 if (pFlyRadioInfo->radioInfo.bPreScaning)
				{
					pFlyRadioInfo->radioInfo.bPreScaning = FALSE;
				}
			 if (pFlyRadioInfo->radioInfo.ePreRadioMode == pFlyRadioInfo->radioInfo.eCurRadioMode)
			 {
				 *pFlyRadioInfo->radioInfo.pPreRadioFreq = 
					 RadioStepFreqGenerate(pFlyRadioInfo->radioInfo.ePreRadioMode
					 ,*pFlyRadioInfo->radioInfo.pPreRadioFreq
					 ,STEP_BACKWARD
					 ,STEP_MANUAL);
					 DBG2(debugOneData("\nTDA7541 Radio set freq value---->",*pFlyRadioInfo->radioInfo.pPreRadioFreq);)
					 returnRadioFreq(*pFlyRadioInfo->radioInfo.pPreRadioFreq);
				 if (pFlyRadioInfo->bPowerUp)
					{
						sem_post(&pFlyRadioInfo->MainThread_sem);
					}
			 }
			 break;
		 default:
			 DBG2(debugOneData("\nTDA7541 Radio user command Key unDeal---->",buf[1]);)
			 break;
			}
		 break;
	 case 0x10://设置收音机频率 
		 if (pFlyRadioInfo->radioInfo.bPreScaning)
			{
				pFlyRadioInfo->radioInfo.bPreScaning = FALSE;
			}
		 iTemp = buf[1]*256+buf[2];
		 iTemp = 
			 RadioStepFreqGenerate(pFlyRadioInfo->radioInfo.ePreRadioMode,iTemp,STEP_NONE,STEP_MANUAL);
		 if (AM == pFlyRadioInfo->radioInfo.ePreRadioMode)
			{
				pFlyRadioInfo->radioInfo.iPreRadioFreqAM = iTemp;
			}
		 else if (FM1 == pFlyRadioInfo->radioInfo.ePreRadioMode)
			{
				pFlyRadioInfo->radioInfo.iPreRadioFreqFM1 = iTemp;
			}
		 else if (FM2 == pFlyRadioInfo->radioInfo.ePreRadioMode)
			{
				pFlyRadioInfo->radioInfo.iPreRadioFreqFM2 = iTemp;
			}
			DBG2(debugOneData("\nTDA7541 Radio set freq value---->",iTemp);)
			returnRadioFreq(iTemp);
		 if (pFlyRadioInfo->bPowerUp)
			{
				sem_post(&pFlyRadioInfo->MainThread_sem);
			}
		 break;
	 case 0x11://频点+
		 if (pFlyRadioInfo->radioInfo.bPreScaning)
			{
				pFlyRadioInfo->radioInfo.bPreScaning = FALSE;
			}
		 pFlyRadioInfo->radioInfo.eButtomStepDirection = STEP_FORWARD;
		 if (0x00 == buf[1])
			{
				/**************************************************/
				//TDA7541RadioReturnToUser(pTDA7541RadioInfo,pdata,len);
				/**************************************************/
				buttomJumpFreqAndReturn();
				pFlyRadioInfo->radioInfo.bPreStepButtomDown = TRUE;
				pFlyRadioInfo->radioInfo.iButtomStepCount = 0;
			}
		 else if (0x01 == buf[1])
			{
				/**************************************************/
				//TDA7541RadioReturnToUser(pTDA7541RadioInfo,pdata,len);
				/**************************************************/
				pFlyRadioInfo->radioInfo.bPreStepButtomDown = FALSE;
			}
		 if (pFlyRadioInfo->bPowerUp)
			{
				sem_post(&pFlyRadioInfo->MainThread_sem);
			}
		 break;
	 case 0x12://频点-
		 if (pFlyRadioInfo->radioInfo.bPreScaning)
			{
				pFlyRadioInfo->radioInfo.bPreScaning = FALSE;
			}
		 pFlyRadioInfo->radioInfo.eButtomStepDirection = STEP_BACKWARD;
		 if (0x00 == buf[1])
			{
				/*****************************************************************/
				//TDA7541RadioReturnToUser(pTDA7541RadioInfo,pdata,len);
				/*****************************************************************/
				buttomJumpFreqAndReturn();
				pFlyRadioInfo->radioInfo.bPreStepButtomDown = TRUE;
				pFlyRadioInfo->radioInfo.iButtomStepCount = 0;
			}
		 else if (0x01 == buf[1])
			{
				/*****************************************************************/
				//TDA7541RadioReturnToUser(pTDA7541RadioInfo,pdata,len);
				/*****************************************************************/
				pFlyRadioInfo->radioInfo.bPreStepButtomDown = FALSE;
			}
		 if (pFlyRadioInfo->bPowerUp)
			{
				sem_post(&pFlyRadioInfo->MainThread_sem);
			}
		 break;
	 case 0x13://扫描控制
		 switch(buf[1])
			{
		 case 0x00:
		 case 0x01:
			 pFlyRadioInfo->radioInfo.bPreScaning = TRUE;
			 pFlyRadioInfo->radioInfo.eScanDirection = STEP_FORWARD;
			 pFlyRadioInfo->radioInfo.bScanRepeatFlag = FALSE;
			 break;
		 case 0x02:
			 pFlyRadioInfo->radioInfo.bPreScaning = TRUE;
			 pFlyRadioInfo->radioInfo.eScanDirection = STEP_BACKWARD;
			 pFlyRadioInfo->radioInfo.bScanRepeatFlag = FALSE;
			 break;
		 case 0x03:
			 pFlyRadioInfo->radioInfo.bPreScaning = FALSE;
			 break;
		 case 0x04:
		 case 0x05:
			 pFlyRadioInfo->radioInfo.bPreScaning = TRUE;
			 pFlyRadioInfo->radioInfo.eScanDirection = STEP_FORWARD;
			 pFlyRadioInfo->radioInfo.bScanRepeatFlag = TRUE;
			 break;
		 case 0x06:
			 pFlyRadioInfo->radioInfo.bPreScaning = TRUE;
			 pFlyRadioInfo->radioInfo.eScanDirection = STEP_BACKWARD;
			 pFlyRadioInfo->radioInfo.bScanRepeatFlag = TRUE;
			 break;
		 default:break;
			}
		 returnRadioScanCtrl(buf[1]);
		 if (pFlyRadioInfo->bPowerUp)
			{
				sem_post(&pFlyRadioInfo->MainThread_sem);
			}
			DBG2(debugOneData("\nTDA7541 Radio set scan ctrl---->",buf[1]);)
			break;
	 case 0x15://开始收音
		 if (0x00 == buf[1])
			{
				pFlyRadioInfo->bPreMute = TRUE;
				if (pFlyRadioInfo->radioInfo.bPreScaning)
				{
					pFlyRadioInfo->radioInfo.bPreScaning = FALSE;
				}
				returnRadioMuteStatus(FALSE);
			}
		 else if (0x01 == buf[1])
			{
				pFlyRadioInfo->bPreMute = FALSE;
				returnRadioMuteStatus(TRUE);
		    }				
		 if (pFlyRadioInfo->bPowerUp)
			{
				sem_post(&pFlyRadioInfo->MainThread_sem);
		    }			
		 break;
	 case 0x16://AF开关
		 if (0x01 == buf[1])
			{
				pFlyRadioInfo->RDSInfo.RadioRDSAFControlOn = TRUE;
				returnRadioAFStatus(TRUE);
			}
		 else if (0x00 == buf[1])
			{
				pFlyRadioInfo->RDSInfo.RadioRDSAFControlOn = FALSE;
				returnRadioAFStatus(FALSE);
			}
		 if (pFlyRadioInfo->bPowerUp)
			{
				sem_post(&pFlyRadioInfo->MainThread_sem);
			}
		 break;
	 case 0x17://TA开关
		 if (0x01 == buf[1])
			{
				pFlyRadioInfo->RDSInfo.RadioRDSTAControlOn = TRUE;
				returnRadioTAStatus(TRUE);
			}
		 else if (0x00 == buf[1])
			{
				pFlyRadioInfo->RDSInfo.RadioRDSTAControlOn = FALSE;
				returnRadioTAStatus(FALSE);
			}
		 if (pFlyRadioInfo->bPowerUp)
			{
				sem_post(&pFlyRadioInfo->MainThread_sem);
			}
		 break;
	 case 0x30://交通广播开关
		 if (0x01 == buf[1])
			{
				pFlyRadioInfo->RDSInfo.RadioRDSAFControlOn = TRUE;
				returnRadioRDSWorkStatus(TRUE);				
			}
		 else if (0x00 == buf[1])
			{
				pFlyRadioInfo->RDSInfo.RadioRDSAFControlOn = FALSE;
				returnRadioRDSWorkStatus(FALSE);
			}
		 if (pFlyRadioInfo->bPowerUp)
			{
				sem_post(&pFlyRadioInfo->MainThread_sem);
			}
		 break;
	 case 0xFF:
		 if (0x01 == buf[1])
			{
				//FRA_PowerUp((DWORD)pTDA7541RadioInfo);
		    } 
		 else if (0x00 == buf[1])
			{
				//FRA_PowerDown((DWORD)pTDA7541RadioInfo);
			}
		 break;
	 default:
		 DBG0(debugOneData("\nTDA7541 Radio user command unhandle---->",buf[1]);)
		 break;
	 }
 }
 
 /*==========================以下为导出函数====================================*/
 /******************************************************************************/
 /******************************************************************************/
 /******************************************************************************/
 /*============================================================================*/
 
/********************************************************************************
 **函数名称：fly_open_device（）函数
 **函数功能：打开设备
 **函数参数：
 **返 回 值：
 **********************************************************************************/
 INT flyOpenDevice(void)
 {
	INT ret = HAL_ERROR_RETURN_FD;
	 //创建线程
	 if (create_thread() == -1)
	 {
		 DBG0(debugString("\nTDA7541 Radio create thread error");)
		 return ret;
	 }
	 DBG0(debugString("\nTDA7541 Radio create all ok");)
	
	 ret = HAL_RADIO_RETURN_FD;
	 return ret;
 }
 
 /********************************************************************************
 **函数名称：fly_init_device_struct（）函数
 **函数功能：初始化结构体里的成员
 **函数参数：
 **返 回 值：
 **********************************************************************************/
void flyInitDeviceStruct(void)
 {
	//为 flyradio_struct_info 结构体分配内存
	pFlyRadioInfo =
		(struct flyradio_struct_info *)malloc(sizeof(struct flyradio_struct_info));
	if (pFlyRadioInfo == NULL)
	{
		return;
	}
	memset(pFlyRadioInfo, 0, sizeof(struct flyradio_struct_info));
	
	powerOnFirstInit();
	allInOneInit();
	
	////参数初始化
	powerOnNormalInit();
	
	debugString("\nFlyRadio SAF7741 init---->2012-03-17");
 }
 
  /********************************************************************************
 **函数名称：flydvd_read()函数
 **函数功能：读出数据
 **函数参数：
 **返 回 值：成功返回实际读得的数据，失败返回-1
 **********************************************************************************/
 INT flyReadData(BYTE *buf, UINT len)
 {
	 UINT16 dwRead;
	 DBG1(debugBuf("\nRADIO-HAL return  bytes Start:", buf,1);)
	 dwRead = readFromJNIBuff(CURRENT_SHARE_MEMORY_ID,buf,len);
	 DBG1(debugBuf("\nRADIO-HAL return  bytes to User:", buf,dwRead);)
	 return dwRead;
 }
 
 /********************************************************************************
 **函数名称：fly_destroy_struct()
 **函数功能：释放内存
 **函数参数：
 **返 回 值：无
 **********************************************************************************/
void flyDestroyDeviceStruct(void)
{
	//各种线程退出
	pFlyRadioInfo->bKillRadioMainThread = TRUE;
	pFlyRadioInfo->bKillRadioScanThread = TRUE;
	pFlyRadioInfo->bKillRadioRDSRecThread = TRUE;

	//释放各种信号量
	sem_destroy(&pFlyRadioInfo->MainThread_sem);

	//释放各种条件变量
	pthread_cond_destroy(&pFlyRadioInfo->ScanThreadCond);
	pthread_cond_destroy(&pFlyRadioInfo->RDSRecThreadCond);

	allInOneDeinit();
	
	free (pFlyRadioInfo);
	pFlyRadioInfo = NULL;
}
 /********************************************************************************
 **函数名称：fly_close_device()函数
 **函数功能：关闭函数
 **函数参数：
 **返 回 值：
 **********************************************************************************/
 INT flyCloseDevice(void)
 {
	  DBG0(debugString("\nTDA7541 Radio close device !");)
	 pFlyRadioInfo->bKillRadioMainThread = TRUE;
	 pFlyRadioInfo->bKillRadioScanThread = TRUE;
	 pFlyRadioInfo->bKillRadioRDSRecThread = TRUE;
	 return TRUE;
 }

 /********************************************************************************
 **函数名称：
 **函数功能：
 **函数参数：
 **返 回 值：
 **********************************************************************************/
void flyCommandProcessor(BYTE *buf, UINT len)
{
	DBG1(debugBuf("\nTDA7541 Radio User write to Radio-HAL---->",buf,len);)
	
	DealRightDataProcessor(&buf[3], buf[2]-1);
}

#endif
