#include "typedef_BT.h"


#define PSKEY_UART_BAUDRATE			0x01BE
#define PSKEY_BCSP_UART_CONFIG		0x01BF
#define PSKEY_VM_DISABLE	 		0x025d
#define PSKEY_INITIAL_BOOTMODE 		0x03cd
#define PSKEY_BOOTMODE1_LIST			0x04b1
#define PSKEY_BOOTMODE1_KEYBASE	0x04f8

#define VarID_PS_ValueReadWrite		0x7003
#define VarID_PS_ValueSize					0x3006
#define VarID_Warm_Reset					0x4001
#define VarID_Cold_Reset					0x4002

#define PS_STORE_DEFAULT					0x0000
#define PS_STORE_PSI							0x0001
#define PS_STORE_PSF						0x0002
#define PS_STORE_PSROM					0x0004
#define PS_STORE_PSRAM					0x0008
#define PS_STORE_PSI_PSF					0x0003
#define PS_STORE_PSI_PSF_PSROM		0x0007
#define PS_STORE_PSI_PSRAM				0x0009
#define PS_STORE_PSI_PSF_PSRAM		0x000b
#define PS_STORE_PSI_PSF_PSROM_PSRAM	0x000f



enum {
	BCCMD_TYPE_GETREQ = 0,
	BCCMD_TYPE_GETRESP,
	BCCMD_TYPE_SETREQ
};

typedef struct _BCCMDPS_payload {
	U16	key;
	U16 length;
	U16 store;
	U16 value[50];
}BCCMDPS_payload;

/*
typedef struct _BCCMDPS_NULLpayload {
	U16	data[4];
}BCCMDPS_NULLpayload;

typedef union _BCCMD_payload {
	BCCMDPS_payload 		ps;
	BCCMDPS_NULLpayload		nullpayload;
}BCCMD_payload;
*/
typedef struct _BCCMD_RESP_PACKET {
	U16	type;
	U16	length;
	U16	seqno;
	U16	varID;
	U16	status;
	//U16 *payload;
	/*BCCMD_payload payload;*/
}BCCMD_RESP_packet;

enum {
	HOST_INTERFACE_NONE = 0,
	HOST_INTERFACE_BCSP,
	HOST_INTERFACE_H2,
	HOST_INTERFACE_H4,
	HOST_INTERFACE_UART,
	HOST_INTERFACE_H5 = 6,
	HOST_INTERFACE_H4DS
};



enum {
	BCCMD_STATUS_OK = 0,
	BCCMD_STATUS_NO_SUCH_VARID,
	BCCMD_STATUS_TOO_BIG,
	BCCMD_STATUS_NO_VALUE,
	BCCMD_STATUS_BAD_REQ,
	BCCMD_STATUS_NO_ACCESS,
	BCCMD_STATUS_READ_ONLY,
	BCCMD_STATUS_WRITE_ONLY,
	BCCMD_STATUS_ERROR,
	BCCMD_STATUS_PERMISSION_DENIED
};


enum {
	BCCMD_RESP_CODE_BASE = 0,
	BCCMD_RESP_CODE_GET_BDA_AUTH,
	BCCMD_RESP_CODE_GET_BDA_NoAUTH,
	BCCMD_RESP_CODE_COLLEX_SECU_LENG_OK,
	BCCMD_RESP_CODE_COLLEX_SECU_LENG_ERROR,
	BCCMD_RESP_CODE_MODIFY_INIT_BOOTMODE_OK,
	BCCMD_RESP_CODE_GOTO_RESTORE_INIT_BOOTMODE,
	BCCMD_RESP_CODE_RESTORE_INIT_BOOTMODE_OK,
	BCCMD_RESP_CODE_STAGE_GOTO_COLD_RESET,

	BCCMD_RESP_CODE_END
};


void BCCMD_BASIC_CMD(U16 VarID, U16 *payload, U16 payloadLen);
void BCCMDPS_GET_KEY_Value(U16 key, U16 length);
void BCCMDPS_GET_KEY_Length(U16 key);
void BCCMDPS_SET_KEY_Value(U16 key, U16 *Value, U16 ValLen);

void sendBCCMDreset(void);
void DoBCCMDs(void);
void handleBCCMDResponse(U8 *BCCMDpayload, U16 len);

extern U16 g_BCCMD_RespCode;


