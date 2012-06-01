#ifndef _TYPEDEF_BT_H
#define _TYPEDEF_BT_H

typedef unsigned char	U8;
typedef unsigned short	U16;
typedef unsigned int	U32;
typedef unsigned char BYTE;

#include <cutils/logger.h>
#include <cutils/logd.h>
#include <cutils/log.h>

#define DLOGD(...) __android_log_print(3 ,  "btdfu", __VA_ARGS__)

/*
//#include "typedef.h"

//typedef U8	UINT8;
//typedef U16	UINT16;
typedef unsigned char	UINT8;
//typedef unsigned int	UINT16;
typedef unsigned long	UINT32;

typedef unsigned char	U8;
typedef unsigned int	U16;
typedef unsigned long	U32;

typedef unsigned char BYTE;
*/

#endif 
