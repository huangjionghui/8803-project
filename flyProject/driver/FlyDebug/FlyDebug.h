#ifndef FLY_DEBUG_H_
#define FLY_DEBUG_H_


#define 	MISC_DYNAMIC_MINOR 		255				//动态设备号
#define 	DEVICE_NAME 			"FlyDebug"	//驱动名


typedef unsigned char  BYTE;
typedef unsigned int   UINT;
typedef unsigned short WORD;
typedef unsigned long  DWORD;
typedef int   BOOL;
typedef int   INT;
typedef float FLOAT;

#define     BUF_MAX_SIZE            4096

typedef struct fly_debug_info{

	BYTE xxxxxxxx;
};

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#endif