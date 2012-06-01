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
#if AUDIO_RADIO_CHIP_SEL == AUDIO_RADIO_7741_7000

#include "FlyRadio7000.h"
#include "../../include/allInOneOthers.h"
#include "../../include/commonFunc.h"

extern struct flyradio_struct_info *pFlyRadioInfo;
extern struct flyAllInOne *pFlyAllInOneInfo;

#if RADIO_RDS
/////////RDS全局变量

struct RDS_TEMP_STRUCT
{
	BYTE rdsdec_cnt0x;
	BYTE rds_ps[2][8];// for Decode Programme Service (PS) Name  节目名字
	BYTE rds_af[8];// for Decode Alternative Frequency List—AF

	BYTE rdsdec_cnt2x;
	BYTE rds_rt_a[2][64];    // for Decode RadioText (RT) 这是 GROUP_A的 TEXT_A 和 TEXT_B
	BYTE rds_rt_b[32];    // for Decode RadioText (RT) 这是 GROUP_B的 TEXT_A 和 TEXT_B

	BYTE rds_10a[8];
	BYTE rds_10a_flag;
	BYTE rds_ptyn[8];

	BYTE rdsdec_cnt10x;
}sRDSTemp;

/*
BYTE * pty_display_table[33]=		 // 节目类型的显示数组 	rdsdec_flag_pty 的数值 是此数组的下标
{
"      None      ",
"      News      ",
"Current Affairs ",
"  Information   ",
"     Sport      ",
"   Education    ",
"     Drama      ",
"    Cultures    ",
"    Science     ",
" Varied Speech  ",
"   Pop Music    ",
"   Rock Music   ",
" Easy Listening ",
"Light Classics M",
"Serious Classics",
"  Other Music   ",
" Weather & Metr ",
"    Finance     ",
"Children’s Progs",
" Social Affairs ",
"    Religion    ",
"    Phone In    ",
"Travel & Touring",
"Leisure & Hobby ",
"   Jazz Music   ",
" Country Music  ",
" National Music ",
"  Oldies Music  ",
"   Folk Music   ",
"  Documentary   ",
"   Alarm Test   ",
"Alarm - Alarm ! ",
"    PTY Seek    "
};
*/
//////////////处理部分

BYTE rdsdec_buf[8];// Groupe = 4 blocks ;  8 chars

#define ZERO(a)  memset(a,0,sizeof(a))
#define GET_T_AB(buf)  (buf[3]>>4) &0x01
#define GET_SEG(buf)  buf[3]&0x0f
#define GET_PTY(buf)  (buf[2]>>4)&0x0f
#define GET_G_AB(buf)  (buf[2]>>3)&0x01

void UpdateRDS_PI(void)
{
	UINT16 temp;
	temp  = (rdsdec_buf[0]<<8) | rdsdec_buf[1];          // 16bit

#if RADIO_RDS_DEBUG_MSG
	DBG2(debugOneData("\nRDS PI-->",temp);)
#endif

	if(pFlyRadioInfo->RDSInfo.rdsdec_flag_pi != temp)
	{
		pFlyRadioInfo->RDSInfo.rdsdec_flag_pi = temp;
	}
	if(0 == pFlyRadioInfo->RDSInfo.BaseReferencePI)//确定基准PI
	{
		pFlyRadioInfo->RDSInfo.BaseReferencePI = temp;
	}
}
void printfRDS_PTY(void)//20100427 节目类型
{
	BYTE buff[] = {RADIORDS_ID_PTY,0x00};
	BYTE temp;
	temp  = ((rdsdec_buf[2]&3)<<3) | (rdsdec_buf[3]>>5); // 5bit


#if RADIO_RDS_DEBUG_MSG
	DBG2(debugOneData("\nRDS PTY-->",temp);)
#endif

	if(pFlyRadioInfo->RDSInfo.rdsdec_flag_pty != temp)
	{
		pFlyRadioInfo->RDSInfo.rdsdec_flag_pty = temp;
		buff[1] = temp;
		flyRadioReturnToUserPutToBuff(buff,2);
	}
}
void printfRDS_TP(void)
{	
	BYTE temp;
	temp  = (rdsdec_buf[2]>>2)&1;                        // 1bit

#if RADIO_RDS_DEBUG_MSG
	DBG2(debugOneData("\nRDS TP FLAG-->",temp);)
#endif

	if(pFlyRadioInfo->RDSInfo.rdsdec_flag_tp != temp)
	{
		pFlyRadioInfo->RDSInfo.rdsdec_flag_tp = temp;
	}
} 
void printfRDS_MS_DI(void)
{
	BYTE temp;
	temp = (rdsdec_buf[3]>>2)&0x01; // 1bit

#if RADIO_RDS_DEBUG_MSG
	DBG2(debugTwoData("\nRDS DI-->",rdsdec_buf[3]&0x03,temp);)
#endif
	if(pFlyRadioInfo->RDSInfo.rdsdec_flag_di[rdsdec_buf[3]&0x03] != temp)
	{

		pFlyRadioInfo->RDSInfo.rdsdec_flag_di[rdsdec_buf[3]&0x03] = temp;
	}

	temp = (rdsdec_buf[3]>>3)&0x01; // 1bit

#if RADIO_RDS_DEBUG_MSG
	DBG2(debugOneData("\nRDS MS-->",temp);)
#endif
	if(pFlyRadioInfo->RDSInfo.rdsdec_flag_ms != temp)
	{
		pFlyRadioInfo->RDSInfo.rdsdec_flag_ms = temp;
	}
}
void printfRDS_PS(BYTE *p)//20100427	 频道名称
{
	BYTE buff[9];
	BYTE i;
	buff[0] = RADIORDS_ID_PS;
	for(i = 0; i < 8; i++)
	{
		buff[i+1] = p[i];
	}
	flyRadioReturnToUserPutToBuff(buff,9);
}	
void printfRDS_RT(BYTE *p,BYTE len) //RadioText
{
	BYTE buff[65];
	BYTE i;
	buff[0] = RADIORDS_ID_RT;
	for(i = 0;i < len;i++)
	{
		buff[i+1] = p[i];
	} 
	flyRadioReturnToUserPutToBuff(buff,len+1);
}
//void printfRDS_AF(BYTE *p) //AF
//{
//	BYTE buff[1+16];
//	BYTE i;
//	buff[0] = RADIORDS_ID_AF;
//	for(i = 0;i < 8;i++)
//	{
//		buff[i*2+1] = (p[i]+875)>>8;
//		buff[i*2+1+1] = p[i]+875;
//	} 
//	flyRadioReturnToUserPutToBuff(buff,1+16);
//}
void printfRDS_Date(void)
{

}
void printfRDS_PTYN(BYTE *p)
{

}

void printfRDS_TA(void)	  //20100427 交通信息
{
	BYTE buff[3] = {RADIORDS_ID_TA,0x00};
	if(((rdsdec_buf[3]>>4)&0x01) && ((rdsdec_buf[2]>>2)&0x01))//开始TA广播
	{
		if(pFlyRadioInfo->RDSInfo.rdsdec_flag_ta == 0)
		{
			pFlyRadioInfo->RDSInfo.rdsdec_flag_ta = 0x01;

			buff[1] = 0x01;
			flyRadioReturnToUserPutToBuff(buff,2);
		}
	}
	else
	{
		if(pFlyRadioInfo->RDSInfo.rdsdec_flag_ta)
		{
			pFlyRadioInfo->RDSInfo.rdsdec_flag_ta = 0x00;

			buff[1] = 0x00;
			flyRadioReturnToUserPutToBuff(buff,2);		
		}	
	}

}
void printfRDS_TA_Spec(void)	  //20100427 交通信息
{
	BYTE buff[3] = {RADIORDS_ID_TA,0x00};
	if(((rdsdec_buf[3]>>3)&0x03) == 0x03)//开始TA广播
	{
		if(pFlyRadioInfo->RDSInfo.rdsdec_flag_ta == 0)
		{
			pFlyRadioInfo->RDSInfo.rdsdec_flag_ta = 0x03;

			buff[1] = 0x01;
			flyRadioReturnToUserPutToBuff(buff,3);
		}
	}
	else
	{
		if(pFlyRadioInfo->RDSInfo.rdsdec_flag_ta)
		{
			pFlyRadioInfo->RDSInfo.rdsdec_flag_ta = 0x00;

			buff[1] = 0x00;
			flyRadioReturnToUserPutToBuff(buff,2);		
		}	
	}
}

void RDSParaInit(void)
{
	DBG2(debugString("\nFLY7741Radio RDS para init!");)

	memset(&pFlyRadioInfo->RDSInfo,0,sizeof(FLY_RDS_INFO));
}

//////////////////////////RDS数据接收处理
//=============================================================================
// RDS Decode Type 0x
//-----------------------------------------------------------------------------
// Parameter(s)
//  fab : Type A or B flag
//-----------------------------------------------------------------------------
// Return Value
//  none
//=============================================================================

void rdsdec_prc_0x(BYTE fab)
{
	BYTE seg = GET_SEG(rdsdec_buf) & 3;

	// Data Order check
	if(seg==0) { sRDSTemp.rdsdec_cnt0x=0; }
	else if(sRDSTemp.rdsdec_cnt0x!=seg) {return;}

	printfRDS_TA();

	sRDSTemp.rds_ps[fab][seg*2+0] = rdsdec_buf[6];
	sRDSTemp.rds_ps[fab][seg*2+1] = rdsdec_buf[7];

	if(fab==GROUP_A){
		sRDSTemp.rds_af[seg*2+0] = rdsdec_buf[4];
		sRDSTemp.rds_af[seg*2+1] = rdsdec_buf[5];
	}

	if(seg==3)// Build Complete
	{
#if RADIO_RDS_DEBUG_MSG
		UINT i;
		DBG2(debugString("\nRDS PS Name-->");)
		for (i = 0;i < 8;i++)
		{
			DBG2(debugOneData(" ",sRDSTemp.rds_ps[fab][i]);)
		}
		if(fab == GROUP_A)
		{
			DBG2(debugString("\nRDS AF List-->");)
			for (i = 0;i < 8;i++)
			{
				DBG2(debugOneData(" ",sRDSTemp.rds_af[i]+875);)
			}
		}
#endif 
		memcpy(pFlyRadioInfo->RDSInfo.rdsdec_ps[fab], sRDSTemp.rds_ps[fab], 8);
		if(memcmp(pFlyRadioInfo->RDSInfo.rdsdec_ps_dis,sRDSTemp.rds_ps,8))
		{
			memcmp(pFlyRadioInfo->RDSInfo.rdsdec_ps_dis,sRDSTemp.rds_ps,8);
			printfRDS_PS(pFlyRadioInfo->RDSInfo.rdsdec_ps_dis);		
		}
		if(fab==GROUP_A)
		{
			if(memcmp(pFlyRadioInfo->RDSInfo.rdsdec_af,sRDSTemp.rds_af,8))//且有变化
			{				
				memcpy(pFlyRadioInfo->RDSInfo.rdsdec_af,sRDSTemp.rds_af,8);	
			}
		}
	}
	sRDSTemp.rdsdec_cnt0x++;
}


//=============================================================================
// RDS Decode Type 1A and 1B
//  Program Item Number and slow labeling codes
//-----------------------------------------------------------------------------
// Parameter(s)
//  fab : Type A or B flag
//-----------------------------------------------------------------------------
// Return Value
//  none
//=============================================================================
void rdsdec_prc_1x(BYTE fab)
{
	UINT16 blk[4];
	BYTE  vc;
	//	BYTE r_page,la;
	//	BYTE  day,hour,min;
	UINT16 label;

	blk[0] = (rdsdec_buf[0]<<8) | rdsdec_buf[1];
	blk[1] = (rdsdec_buf[2]<<8) | rdsdec_buf[3];
	blk[2] = (rdsdec_buf[4]<<8) | rdsdec_buf[5];
	blk[3] = (rdsdec_buf[6]<<8) | rdsdec_buf[7];

	//	r_page = blk[1]&0x1F;
	//	la     = blk[2]&0x8000;    // Linkage Actuator
	vc     =(blk[2]>>12)&0x07; // Variant Code

	//	day = (blk[3]>>11) & 0x1F;
	//	hour= (blk[3]>>6) & 0x1f;
	//	min = 0x3f & blk[3];

	if(fab==GROUP_A){
		label = blk[2]&0x0FFF;

		switch(vc){
case 0: // Extended Country Code
	label = blk[2]&0x00FF;
	pFlyRadioInfo->RDSInfo.rds_ecc = label;
	break;

case 1: // TMC identification
	break;

case 2: // Paging identification
	break;

case 3: // Language codes
	pFlyRadioInfo->RDSInfo.rds_language_code =	label;
	break;

case 4: // not assigned
case 5: // not assigned
	break;

case 6: // For use by broadcasters
	break;

case 7: // Identification of EWS channnel
	break;
		}
	}
}



//=============================================================================
// RDS Decode 2A and 2B
//  Radio Text (2A has 64 charactors,2B has 32 charactors)
//-----------------------------------------------------------------------------
// Parameter(s)
//  fab : Type A or B flag
//-----------------------------------------------------------------------------
// Return Value
//  none
//=============================================================================
void rdsdec_prc_2x(BYTE fab)
{
	BYTE b   = GET_T_AB(rdsdec_buf);
	BYTE seg = GET_SEG(rdsdec_buf);
	BYTE ofs;
	BYTE i;

	// Data Order check
	if(seg==0) {sRDSTemp.rdsdec_cnt2x = 0;}
	else if(sRDSTemp.rdsdec_cnt2x!=seg) {return;}

	if(fab==GROUP_A)
	{
		ofs=seg*4;
		for(i = 0;i < 4;i++)
		{
			sRDSTemp.rds_rt_a[b][ofs+i]=rdsdec_buf[4+i];
			if(rdsdec_buf[4+i] == 0x0D || ofs+i == 63)//0x0D结束符或者达到最大数目
			{
#if RADIO_RDS_DEBUG_MSG
				UINT j;
				DBG2(debugString("\nRDS RadioText-->");)
				j = 0;
				while (sRDSTemp.rds_rt_a[b][j] != 0x0D && j < 64)
				{
					DBG2(debugOneData(" ",sRDSTemp.rds_rt_a[b][j]);)
					j++;
				}
#endif 
				memcpy(pFlyRadioInfo->RDSInfo.rdsdec_rt_a[b],sRDSTemp.rds_rt_a[b],ofs+i+1);
				if(memcmp(pFlyRadioInfo->RDSInfo.rdsdec_rt_a_dis,sRDSTemp.rds_rt_a[b],ofs+i+1))
				{
					memcpy(pFlyRadioInfo->RDSInfo.rdsdec_rt_a_dis,sRDSTemp.rds_rt_a[b],ofs+i+1);
					printfRDS_RT(pFlyRadioInfo->RDSInfo.rdsdec_rt_a_dis,ofs+i+1);
				}
			}	
		}
	} 
	else 
	{
		ofs=seg*2;
		for(i = 0; i < 2; i++)
		{
			sRDSTemp.rds_rt_b[ofs+i]=rdsdec_buf[6+i];
			if(rdsdec_buf[6+i] == 0x0D || ofs+i == 31)//0x0D结束符或者达到最大数目
			{
#if RADIO_RDS_DEBUG_MSG
				UINT j;
				DBG2(debugString("\nRDS RadioText-->");)
				j = 0;
				while (sRDSTemp.rds_rt_b[j] != 0x0D && j < 32)
				{
					DBG2(debugOneData(" ",sRDSTemp.rds_rt_b[j]);)
					j++;
				}
#endif 
				if(memcmp(pFlyRadioInfo->RDSInfo.rdsdec_rt_b,sRDSTemp.rds_rt_b,ofs+i+1))
				{
					memcpy(pFlyRadioInfo->RDSInfo.rdsdec_rt_b,sRDSTemp.rds_rt_b,ofs+i+1);
					printfRDS_RT(pFlyRadioInfo->RDSInfo.rdsdec_rt_b,ofs+i+1);
				}
			}	
		}
	}
	sRDSTemp.rdsdec_cnt2x++;
}



//=============================================================================
// RDS Decode type 3A or 3B
//  3A : Application identification for Open data
//  3B : Open Data Application
//-----------------------------------------------------------------------------
// Parameter(s)
//  fab : Type A or B flag
//-----------------------------------------------------------------------------
// Return Value
//  none
//=============================================================================
void rdsdec_prc_3x(BYTE fab)
{
	//	blk[0] = (rdsdec_buf[0]<<8) | rdsdec_buf[1];
	//	blk[1] = (rdsdec_buf[2]<<8) | rdsdec_buf[3];
	//	blk[2] = (rdsdec_buf[4]<<8) | rdsdec_buf[5];
	//	blk[3] = (rdsdec_buf[6]<<8) | rdsdec_buf[7];


	if(fab==GROUP_A){

	}

}



//=============================================================================
// RDS Decode type 4A and 4B
//  4A : "Clock-time and date"
//  4B : Open data application
//-----------------------------------------------------------------------------
// Parameter(s)
//  fab : Type A or B flag
//-----------------------------------------------------------------------------
// Return Value
//  none
//=============================================================================
void rdsdec_prc_4x(BYTE fab)
{
	if(fab==GROUP_A) {
		//		char buf[64];
		int date,hour,min,ofs;
		int yd,md,d,y,m;

		date = ((rdsdec_buf[3]&3)<<15) | (rdsdec_buf[4]<<7) | (rdsdec_buf[5]>>1);
		hour = ((rdsdec_buf[5]&1)<<4)  | ((rdsdec_buf[6]>>4));
		min  = ((rdsdec_buf[6]&0xf)<<2)| ((rdsdec_buf[7]>>6));
		ofs  = (rdsdec_buf[7]&0x3f);

		//		if(rdsdec_buf[7]&0x20) ofs=-ofs;

		yd = (int)(((double)date-15078.2)/365.25);
		md = (int)(((((double)date-14956.1)-((double)yd*365.25)))/30.6001);
		d = (int)(date-14956-((double)yd*365.25)-((double)md*30.6001));

		if(md==14 || md==15) {
			y = yd + 1;
			m = md -13;
		} else {
			y = yd;
			m = md -1;
		}

		pFlyRadioInfo->RDSInfo.rds_clock_year = 1900 + y;
		pFlyRadioInfo->RDSInfo.rds_clock_month= m;
		pFlyRadioInfo->RDSInfo.rds_clock_day  = d + 1;
		pFlyRadioInfo->RDSInfo.rds_clock_wd = (BYTE)(((long)date +2)%7 + 1); 

		pFlyRadioInfo->RDSInfo.rds_clock_hour = hour;
		pFlyRadioInfo->RDSInfo.rds_clock_min  = min;
		pFlyRadioInfo->RDSInfo.rds_clock_ofs  = ofs;

#if RADIO_RDS_DEBUG_MSG
		DBG2(debugOneData("\rRDS Time Y-->",pFlyRadioInfo->RDSInfo.rds_clock_year);)
		DBG2(debugOneData("M-->",pFlyRadioInfo->RDSInfo.rds_clock_month);)
		DBG2(debugOneData("D-->",pFlyRadioInfo->RDSInfo.rds_clock_day);)
		DBG2(debugOneData("W-->",pFlyRadioInfo->RDSInfo.rds_clock_wd);)
		DBG2(debugOneData("H-->",pFlyRadioInfo->RDSInfo.rds_clock_hour);)
		DBG2(debugOneData("M-->",pFlyRadioInfo->RDSInfo.rds_clock_min);)
		DBG2(debugOneData("O-->",pFlyRadioInfo->RDSInfo.rds_clock_ofs);)
#endif

		if(date != 0)
		{
			printfRDS_Date();
		}
	}
}
//=============================================================================
// RDS Decode 5A and 5B 
//  5A and 5B : Transparent data channels or ODA
//-----------------------------------------------------------------------------
// Parameter(s)
//  fab : Type A or B flag
//-----------------------------------------------------------------------------
// Return Value
//  none
//=============================================================================
void rdsdec_prc_5x(BYTE fab)
{

}


//=============================================================================
// RDS Decode 6A and 6B
// In-house applications of ODA
//-----------------------------------------------------------------------------
// Parameter(s)
//  fab : Type A or B flag
//-----------------------------------------------------------------------------
// Return Value
//  none
//=============================================================================
void rdsdec_prc_6x(BYTE fab)
{
}


//=============================================================================
// RDS Decode 7A and 7B
// 7A : Radio Paging or ODA
// 7B : Open data application
//-----------------------------------------------------------------------------
// Parameter(s)
//  fab : Type A or B flag
//-----------------------------------------------------------------------------
// Return Value
//  none
//=============================================================================
void rdsdec_prc_7x(BYTE fab)
{
}



//=============================================================================
// RDS Decode 8A and 8B
// Traffic Message Channes or ODA
//-----------------------------------------------------------------------------
// Parameter(s)
//  fab : Type A or B flag
//-----------------------------------------------------------------------------
// Return Value
//  none
//=============================================================================
void rdsdec_prc_8x(BYTE fab)	//TMC
{
}



//=============================================================================
// RDS Decode 9A and 9B
// Emergency warining systems or ODA
//-----------------------------------------------------------------------------
// Parameter(s)
//  fab : Type A or B flag
//-----------------------------------------------------------------------------
// Return Value
//  none
//=============================================================================
void rdsdec_prc_9x(BYTE fab)	//EWS似乎有不同标准，ODA
{
}

//=============================================================================
// RDS Decode 10A and 10B
// 10A : Program Type Name
// 10B : Open data
//-----------------------------------------------------------------------------
// Parameter(s)
//  fab : Type A or B flag
//-----------------------------------------------------------------------------
// Return Value
//  none
//=============================================================================
void rdsdec_prc_10x(BYTE fab)
{
	BYTE ofs;
	BYTE b;

	if(fab==GROUP_A)
	{
		ofs = (rdsdec_buf[3]&0x01)*4;
		b = (rdsdec_buf[3]>>4)&0x01;

		if(ofs == 0){sRDSTemp.rdsdec_cnt10x = 0;}
		else if(sRDSTemp.rdsdec_cnt10x == 0){return;	}
		sRDSTemp.rdsdec_cnt10x++;

		sRDSTemp.rds_ptyn[ofs+0] = rdsdec_buf[4];
		sRDSTemp.rds_ptyn[ofs+1] = rdsdec_buf[5];
		sRDSTemp.rds_ptyn[ofs+2] = rdsdec_buf[6];
		sRDSTemp.rds_ptyn[ofs+3] = rdsdec_buf[7];


		if(ofs==4 && sRDSTemp.rdsdec_cnt10x==2)
		{
			if(memcmp(pFlyRadioInfo->RDSInfo.rdsdec_ptyn[b],sRDSTemp.rds_ptyn,8))
			{
				memcpy(pFlyRadioInfo->RDSInfo.rdsdec_ptyn[b],sRDSTemp.rds_ptyn,8);
				printfRDS_PTYN(pFlyRadioInfo->RDSInfo.rdsdec_ptyn[b]);
			}
		}
	}
	else
	{
	}
}

#ifdef DEF_SUPPORT_11	//For Open Data Application
//=============================================================================
// RDS Decode 11A and 11B
// Open Data Application
//-----------------------------------------------------------------------------
// Parameter(s)
//  fab : Type A or B flag
//-----------------------------------------------------------------------------
// Return Value
//  none
//=============================================================================
void rdsdec_prc_11x(BYTE fab)
{
}
#endif

#ifdef DEF_SUPPORT_12	//For Open Data Application
//=============================================================================
// RDS Decode 12A and 12B
// Open Data Application
//-----------------------------------------------------------------------------
// Parameter(s)
//  fab : Type A or B flag
//-----------------------------------------------------------------------------
// Return Value
//  none
//=============================================================================
void rdsdec_prc_12x(BYTE fab)
{
}
#endif


#ifdef DEF_SUPPORT_13	//For Open Data Application
//=============================================================================
// RDS Decode 13A and 13B
// Enhanced Radio Paging or ODA
//-----------------------------------------------------------------------------
// Parameter(s)
//  fab : Type A or B flag
//-----------------------------------------------------------------------------
// Return Value
//  none
//=============================================================================
void rdsdec_prc_13x(BYTE fab)
{
	if(fab==GROUP_A){
		BYTE sty = rdsdec_buf[3]&0x07;
		BYTE inf = rdsdec_buf[3]&0x08;

		switch(sty){
case 0: // address notification bits 24...0,when only 25bits are used 
	break;

case 1: // address notification bits 49..25,when 50bits are used
	break;

case 2: // adress notification bits 24..0,when 50bits are used
	break;

case 3: // Reserved for Value Added Services system information
	break;

case 4: // Reserved for future use
case 5:
case 6:
case 7:
	break;
		}

		host_rds_update(PTY_13A);
	}
}
#endif

#ifdef DEF_SUPPORT_14	//等待处理

UINT16 rds_14a[16];
BYTE  rds_14a_flag;
//=============================================================================
// RDS Decode 14A and 14B
// 14A : Enhanced Other Networks informations
//-----------------------------------------------------------------------------
// Parameter(s)
//  fab : Type A or B flag
//-----------------------------------------------------------------------------
// Return Value
//  none
//=============================================================================
void rdsdec_prc_14x(BYTE fab)
{
	UINT16 pi_tn;
	UINT16 pi_on;
	BYTE tp_tn;
	BYTE tp_on;

	pi_tn = (rdsdec_buf[0]<<8) | rdsdec_buf[1];
	pi_on = (rdsdec_buf[6]<<8) | rdsdec_buf[7];
	tp_tn = (rdsdec_buf[2]&0x08)?1:0;
	tp_on = (rdsdec_buf[2]&0x04)?1:0;

	if(fab==GROUP_A) {
		BYTE vc = rdsdec_buf[3]&0x0f;

		if(vc==0) {
			rds_14a_flag=0;
		}

		rds_14a[vc]=(rdsdec_buf[4]<<8) | rdsdec_buf[5];
		rds_14a_flag++;

		if(rds_14a_flag==16 && vc==15){
			host_rds_update(PTY_14A);
		}
	}
	else {
		host_rds_update(PTY_14B);
	}

}
#endif

void rdsdec_prc_14x_spec(BYTE fab)
{
	if(fab == GROUP_B)
	{
		printfRDS_TA_Spec();
	}
}

#ifdef DEF_SUPPORT_15	//已在其他地方处理完成
BYTE rds_15a[8];
BYTE rds_15a_flag;
//=============================================================================
// RDS Decode 15A and 15B
// Fast basic tuning and switching information
//-----------------------------------------------------------------------------
// Parameter(s)
//  fab : Type A or B flag
//-----------------------------------------------------------------------------
// Return Value
//  none
//=============================================================================
void rdsdec_prc_15x(BYTE fab)
{
	if(fab==GROUP_A){
		BYTE ps = (rdsdec_buf[3]&1)*4;

		if(ps==0){
			rds_15a_flag=0;
		}

		rds_15a[ps+0]=rdsdec_buf[4];
		rds_15a[ps+1]=rdsdec_buf[5];
		rds_15a[ps+2]=rdsdec_buf[6];
		rds_15a[ps+3]=rdsdec_buf[7];
		rds_15a_flag++;

		if(rds_15a_flag==2 && ps){
			host_rds_update(PTY_15A);
		}
	}
}
#endif
//=============================================================================
// RDS Decode Processing
//-----------------------------------------------------------------------------
// Parameter(s)
//  none
//-----------------------------------------------------------------------------
// Return Value
//  none
//=============================================================================
void rdsdec_process(void)//处理接收到的完整的一组数据
{
	BYTE pty = GET_PTY(rdsdec_buf);
	BYTE fab = GET_G_AB(rdsdec_buf);

	// ALL GROUP
	UpdateRDS_PI();
	printfRDS_PTY();
	printfRDS_TP();
	// MS,TA,DI
	if(pty==0 || (pty==15 && fab==GROUP_B)) {
		printfRDS_MS_DI();
	}
	//		rdsdec_flag_ta = (rdsdec_buf[3]>>4)&1; // 1bit

	// EON TA 
	if(pty==14) {
		pFlyRadioInfo->RDSInfo.rdsdec_flag_eon = 1;	
	}

	pFlyRadioInfo->RDSInfo.rdsdec_flag_recv[fab] |= (1<<pty); // Set received flag


	switch(pty){

case 0: rdsdec_prc_0x(fab); break;

case 1: rdsdec_prc_1x(fab); break;

case 2: rdsdec_prc_2x(fab); break;

case 3: rdsdec_prc_3x(fab); break;

case 4: rdsdec_prc_4x(fab); break;

case 5: rdsdec_prc_5x(fab); break;

case 6: rdsdec_prc_6x(fab); break;

case 7: rdsdec_prc_7x(fab); break;

case 8: rdsdec_prc_8x(fab); break;

case 9: rdsdec_prc_9x(fab); break;

case 10: rdsdec_prc_10x(fab); break;

	/*case 11: rdsdec_prc_11x(pSAF7741RadioInfo,fab); break;

	case 12: rdsdec_prc_12x(pSAF7741RadioInfo,fab); break;

	case 13: rdsdec_prc_13x(pSAF7741RadioInfo,fab); break;

	case 14: rdsdec_prc_14x(pSAF7741RadioInfo,fab); break;

	case 15: rdsdec_prc_15x(pSAF7741RadioInfo,fab); break; */

case 14: rdsdec_prc_14x_spec(fab); break;//特别版，解TA信号

default :
	break;

	}
}

void *radio_rdsrec_thread(void *arg)
{
	BYTE i;
	INT ret = 0;
	struct timeval timenow;
	struct timespec timeout;
	DWORD dwWaitReturn;
	DWORD dwRes;
	DWORD dwMsgFlag;

	BYTE regRDSData[11] = {0x00,0x00,0x30,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};//IIC_RDS1_STATus register
	UINT regAddr = (regRDSData[0] << 16) + (regRDSData[1] << 8) + regRDSData[2];
	BYTE bSynchronised,bDataOverflow,bErrorStatus;

	BYTE sReadCntLast = 0;
	BYTE sReadCntNow;

	debugString("\nTDA7541 Radio RDSRecThread Start!");

	while (!pFlyRadioInfo->bKillRadioRDSRecThread)
	{
		if (!pFlyRadioInfo->bPowerUp)
		{
			Sleep(500);
			continue;
		}

		WaitSignedTimeOut(&pFlyRadioInfo->RDSRecThreadMutex,&pFlyRadioInfo->RDSRecThreadCond,&pFlyRadioInfo->bRDSThreadRunAgain,INFINITE);

		if ((pFlyAllInOneInfo->pMemory_Share_Common->eCurAudioInput != RADIO) ||
			(AM == pFlyRadioInfo->radioInfo.eCurRadioMode) || 
			(TRUE == pFlyRadioInfo->radioInfo.bPreScaning) ||
			(TRUE == pFlyRadioInfo->radioInfo.bCurScaning) ||
			(*pFlyRadioInfo->radioInfo.pPreRadioFreq != *pFlyRadioInfo->radioInfo.pCurRadioFreq) ||
			(pFlyRadioInfo->radioInfo.eCurRadioMode != pFlyRadioInfo->radioInfo.ePreRadioMode) ||
			(TRUE == pFlyRadioInfo->radioInfo.bPreStepButtomDown))
		{
			Sleep(500);
			continue;
		}
		else
		{
			Sleep(20);
		}

		if (!pFlyRadioInfo->bRDSThreadGO || pFlyAllInOneInfo->pMemory_Share_Common->bPrepareToSleep == TRUE)
		{
			continue;
		}
		//debugString("\nFlyRadio SAF7741 RDSRecThread-->received RDS message !");

		rdsdec_process();


		/************下面是通过7741 I2C去读RDS信息自己解码**********/
		//I2C_Read_SAF7741(regAddr,&regRDSData[3],8);//读0x000030,0x000031,0x000032,0x000033(2*4=8)
		////for (i = 0; i < 8; i++)
		////{
		////	debugOneData(" ",regRDSData[i+3]);
		////}

		////memcpy(rdsdec_buf,&regRDSData[3],8);

		//bSynchronised = (regRDSData[4]>>7)&0x01;//regRDSData[4]为RDS_STATus
		//bDataOverflow = (regRDSData[4]>>6)&0x01;
		//bErrorStatus = (regRDSData[4]>>0)&0x03;
		//sReadCntNow = (regRDSData[4]>>2)&0x07;

		//if(bSynchronised == 0 || bDataOverflow || bErrorStatus || sReadCntLast > 3) sReadCntLast = 3;//
		//if(sReadCntLast == 3 && sReadCntNow == 0)
		//{
		//	sReadCntLast = 0;
		//	rdsdec_buf[0] = regRDSData[5];	//第一块
		//	rdsdec_buf[1] = regRDSData[6];
		//	//DBG2(debugString("O");)
		//}
		//else if(sReadCntLast == 0 && sReadCntNow == 1)
		//{
		//	sReadCntLast = 1;	
		//	rdsdec_buf[2] = regRDSData[5];	//第二块
		//	rdsdec_buf[3] = regRDSData[6];
		//	//DBG2(debugString("O");)
		//}
		//else if(sReadCntLast == 1 && ((sReadCntNow == 4) || (sReadCntNow == 2))) //C or C' block
		//{
		//	sReadCntLast = 2;	
		//	rdsdec_buf[4] = regRDSData[5];	//第三块
		//	rdsdec_buf[5] = regRDSData[6];
		//	//DBG2(debugString("O");)
		//}
		//else if(sReadCntLast == 2 && sReadCntNow == 3)
		//{
		//	sReadCntLast = 3;	
		//	rdsdec_buf[6] = regRDSData[5];	//第四块
		//	rdsdec_buf[7] = regRDSData[6];
		//	rdsdec_process();//处理接收到的完整的一组数据
		//	DBG2(debugString("\n---OK---");)
		//}
		//else if(sReadCntLast != sReadCntNow)
		//{
		//	sReadCntLast = 3;
		//	DBG2(debugString("-");)
		//}
		//else
		//{
		//	DBG2(debugString("+");)
		//}
	}
	debugString("\nFlyRadio SAF7741 RDSRecThread exit!");
	return 0;
}

#endif

#endif
