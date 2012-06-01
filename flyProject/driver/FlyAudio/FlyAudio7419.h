#ifndef FLY_AUDIO_H_
#define FLY_AUDIO_H_

#define 	MISC_DYNAMIC_MINOR 		255				//动态设备号
#define 	DEVICE_NAME 			"FlyAudio"		//驱动名


//音频各种参数定义
#define BALANCE_LEVEL_COUNT		21
#define FADER_LEVEL_COUNT		21
#define TREB_FREQ_COUNT		    2
#define TREB_LEVEL_COUNT	    11

#define MID_FREQ_COUNT		    3
#define MID_LEVEL_COUNT		    11

#define BASS_FREQ_COUNT		    3
#define BASS_LEVEL_COUNT	    11

#define LOUDNESS_FREQ_COUNT		3
#define LOUDNESS_LEVEL_COUNT	11

#define SUB_FILTER_COUNT	    3
#define SUB_LEVEL_COUNT		    11

#define LARGER(A, B)    ((A) >= (B)? (A):(B))

typedef struct fly_audio_info{

	BOOL bInit;
	//	BOOL GraphicalSpectrumAnalyzer;
	//	BYTE GraphicalSpectrumAnalyzerValue[9];

	BOOL bAmplifierMute;

	BOOL bMuteRadio;//收音机
	BOOL bMuteBT;//BT

	BYTE preMainAudioInput;
	BYTE tmpMainAudioInput;
	BYTE curMainAudioInput;
	BYTE dspMainAudioInput;

	BYTE preMainVolume;
	BYTE curMainVolume;
	BYTE dspMainVolume;

	BOOL preMainMute;
	BOOL tmpMainMute;//中转值
	BOOL curMainMute;
	BOOL dspMainMute;

	BYTE preBassFreq;
	BYTE tmpBassFreq;//中转值
	BYTE curBassFreq;
	BYTE dspBassFreq;
	BYTE usrBassFreq;

	BYTE preBassLevel;
	BYTE tmpBassLevel;
	BYTE curBassLevel;
	BYTE dspBassLevel;
	BYTE usrBassLevel;

	BYTE preMidFreq;
	BYTE tmpMidFreq;
	BYTE curMidFreq;
	BYTE dspMidFreq;
	BYTE usrMidFreq;

	BYTE preMidLevel;
	BYTE tmpMidLevel;
	BYTE curMidLevel;
	BYTE dspMidLevel;
	BYTE usrMidLevel;

	BYTE preTrebleFreq;
	BYTE tmpTrebleFreq;
	BYTE curTrebleFreq;
	BYTE dspTrebleFreq;
	BYTE usrTrebleFreq;

	BYTE preTrebleLevel;
	BYTE tmpTrebleLevel;
	BYTE curTrebleLevel;
	BYTE dspTrebleLevel;
	BYTE usrTrebleLevel;

	BYTE preLoudFreq;
	BYTE tmpLoudFreq;
	BYTE curLoudFreq;
	BYTE dspLoudFreq;

	BYTE preLoudLevel;
	BYTE tmpLoudLevel;
	BYTE curLoudLevel;
	BYTE dspLoudLevel;

	BYTE preBalance;
	BYTE tmpBalance;
	BYTE curBalance;
	BYTE dspBalance;

	BYTE preFader;
	BYTE tmpFader;
	BYTE curFader;
	BYTE dspFader;

	BOOL preLoudnessOn;
	BOOL tmpLoudnessOn;
	BOOL curLoudnessOn;
	BOOL dspLoudnessOn;

	BOOL preSubOn;
	BOOL tmpSubOn;
	BOOL curSubOn;
	BOOL dspSubOn;

	BYTE preSubFilter;
	BYTE tmpSubFilter;
	BYTE curSubFilter;
	BYTE dspSubFilter;

	BYTE preSubLevel;
	BYTE tmpSubLevel;
	BYTE curSubLevel;
	BYTE dspSubLevel;

	BYTE preSimEQ;
	BYTE curSimEQ;
	BYTE dspSimEQ;

	BYTE preGPSSpeaker;
	BYTE tmpGPSSpeaker;
	BYTE curGPSSpeaker;
	BYTE dspGPSSpeaker;

	BOOL bEnableVolumeFader;
}FLY_AUDIO_INFO, *P_FLY_AUDIO_INFO;

#define USER_BUF_MAX  400
#define USER_BUF_LEN  300

#define DATA_BUFF_LENGTH_FROM_MCU		128	//立即处理
typedef struct audio_info{

	BOOL bPowerUp;
	BOOL bNeedInit;
	
	UINT16  buffToHalHx;
	UINT16  buffToHalLx;
	struct  completion buffCompToHal;
	BYTE    buffToHal[USER_BUF_MAX][USER_BUF_LEN];
	
	//audio main thread
	struct work_struct FlyAudioMainWork;
	
	//audio delay thread
	BOOL bKillDispatchFlyAudioDelayThread;
	struct completion comFlyAudioDelayThread;
	struct task_struct *taskFlyAudioDelayThread; 

	//gps audio thread
	BOOL bKillDispatchFlyGPSAudioThread;
	struct task_struct *taskFlyGPSAudioThread; 

	//ipd driver thread
	BOOL bKillDispatchFlyIPCDriverThread;
	//struct task_struct *taskFlyIPCDriverThread; 
	
	wait_queue_head_t read_wait;

	BOOL bAudioNeedSleep;
	BOOL bAudioMainThreadRunning;
	BOOL bAudioDelayThreadRunning;

	BOOL bAudioSleep;
	
	FLY_AUDIO_INFO sFlyAudioInfo;
}AUDIO_INFO,*P_AUDIO_INFO;


#define DEBUG_MSG_ON  0
#if DEBUG_MSG_ON
#define DBG(fmt, arg...) printk(fmt,##arg)
#define DBG_ISR(CODE) printk(fmt,##arg)
#define DBG0(fmt,arg...) printk(fmt,##arg)
#else
#define DBG0(fmt,arg...) printk(fmt,##arg)
#define DBG(fmt, arg...) do { (void)(fmt); } while(0)
#define DBG_ISR(fmt, arg...) do { (void)(fmt); } while(0)
#endif
#endif 