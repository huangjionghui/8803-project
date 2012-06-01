#ifndef SAVE_FILE_H_
#define SAVE_FILE_H_
#include "../../include/global.h"
//#include "../driver/FlyHardware/FlyInclude.h"

#ifndef SCAR_MODEL_MAX
#define SCAR_MODEL_MAX 64
#endif

#ifndef DEBUG_BUFF_LINE_LENGTH
#define DEBUG_BUFF_LINE_LENGTH 256
#endif

typedef struct save_file_struct{

	//车型信息处理
	BYTE sCarModuleName[SCAR_MODEL_MAX];
	BYTE sCarModuleStr[SCAR_MODEL_MAX];
	UINT32 sCarModuleStr_Len;
	BOOL sCarReadModuleName;
	UINT32 sCarDataLen;
	UINT32 sCarDataValue;
	BOOL sCarDataEnd;
	
	//SD卡调试信息
	BYTE debugParaName[DEBUG_BUFF_LINE_LENGTH];
	UINT32 debugParaValue;
	BOOL debugbReadParaName;
	UINT debugiParaLength;
	BOOL debugReadParaEnd;

}FLY_SAVEFILE_INFO , *_p_FLY_SAVEFILE_INFO; 

extern void writeSaveFileData(const char *path, BYTE *buf, UINT32 size);
extern UINT32 readSaveFileData(const char *path, BYTE *buf, UINT32 size);
extern unsigned long get_save_file_size(const char *path);
extern UINT32 SaveDataToSDcard(const char *path,BYTE cmd,BYTE* buf,UINT32 size);
extern BOOL debugParaReadFromFile(void);
extern BOOL dealPanelData(void);
extern BOOL dealSteelWheelData(UINT iSteelwheelIndex);
extern BOOL dealsCarMessageFromFile(void);
#endif