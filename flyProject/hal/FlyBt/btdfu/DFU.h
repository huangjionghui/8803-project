#include "typedef_BT.h"

//#define DEBUG(x) 

typedef struct _DFU_Device_Info_tag {
	U16 DeviceClass;
	U16 DeviceSubClass;
	U16 DeviceProtocol;
	U16 VendorID;
	U16 ProductID;
	U16 bcdDevice;
}DFU_Device_Info;

typedef struct _DFU_Function_Info_tag {
	U16 Attributes;
	U16 DetachTimeout;
	U16 TransferSize;
}DFU_Function_Info;


typedef struct _DFU_Status_Info_tag {
	U8	Status;
	U8	PollTimeout[3];
	U8	State;
	U8	iString;
}DFU_Status_Info;

typedef union _DFU_RESP_data {
	DFU_Device_Info DeviceInfo;
	DFU_Function_Info FunctionInfo;
	DFU_Status_Info Staus_Info;	
}DFU_RESP_data;

typedef struct _DFU_RESP_PACKET {
	U16				status;
	U16				respLength;
	DFU_RESP_data 	respInfo;
}DFU_resp_packet;

enum {
	cmd_DFU_DETACH = 0,
	cmd_DFU_DNLOAD,
	cmd_DFU_UPLOAD,
	cmd_DFU_GETSTATUS,
	cmd_DFU_CLRSTATUS,
	cmd_DFU_GETSTATE,
	cmd_DFU_ABORT,
	cmd_DFU_RESET, 
	cmd_DFU_GETDEVICE,
	cmd_DFU_GETFUNCT
};

enum {
	status_OK = 0,
	status_errTARGET,
	status_errFILE,
	status_errWRITE,
	status_errERASE,
	status_errCHECK_ERASED,
	status_errPROG,
	status_errVERIFIY,
	status_errADDRESS,
	status_errNOTDONE,
	status_errFIRMWARE,
	status_errVENDOR,
	status_errUSBR,
	status_errPOR,
	status_errUNKNOWN,
	status_errSTALLEDPKT
};

enum {
	state_appIDE = 0,
	state_appDETACH,
	state_dfuIDLE,
	state_dfuDNLOAD_SYNC,
	state_dfuDNBUSY,
	state_dfuDNLOAD_IDLE,
	state_dfuMANIFEST_SYNC,
	state_dfuMANIFEST,
	state_dfuMANIFEST_WAIT_RESET,
	state_dfuUPLOAD_IDLE,
	state_dfuERROR
};


void handleDFUResponse(U8 *DFUpayload, U16 len);
void sendDFUcmd(U8 cmd);
void myMemCpy( U8 *src, U8 *dest, U16 n);
void InsertDFUFileData(U8 *destAdd, U32 offset, U16 size);

extern U8 		fEnterDFUMode;


