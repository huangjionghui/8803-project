#include "typedef_BT.h"

#define GET_BITS(var,pos,size) (((var) >> (pos)) & ((((U8)1)<<(size))-1))
#define SET_BITS(var,pos) ((  (var) << (pos) ))

#define SLIP_END		0xc0
#define SLIP_ESC		0xdb
#define SLIP_ESC_END	0xdc
#define SLIP_ESC_ESC	0xdd

#define PROTOCOL_NONE  		0
#define PROTOCOL_BCSP_LE  	1
#define PROTOCOL_BCCMD		2
#define PROTOCOL_HCI			5
#define PROTOCOL_DFU   		12


//#define	CRC


typedef struct _BCSPPacket {
	U8* header;
	U8* payload;
	U8* PureBCSPBuf;

	U16 BytesPureBCSP;

	U8 seq:3;
	U8 ack:3;
	U8 crcPresent:1;
	U8 protocolType:1;
	U8 protocolID:4;
	U16 payloadLen:12;
	U8 checksum;

#ifdef CRC
	U16 crc;
#endif

}BCSPPacket;

extern U8	g_swithTx;
extern U8	g_rxStartSlip;
extern U8	g_rxEndSlip;
extern U8 	isPACKETHealth;


extern BCSPPacket g_RxPacket;
extern BCSPPacket g_AckPacket;
extern BCSPPacket g_TxPacket;

U8 BCSPFromRawSLIP(U8 *s, U8 start, U8 end);
void HandleBCSPPacket(void);
void HandleIncompleteBCSPPacket(void);
void sendBCSPPacket(BCSPPacket *pkt, U8 IsReSend);
void init_BCCMD(void);
void BCSP_LE_send_SYNC_RESP(void);
void BCSP_LE_send_CONF_RESP(void);

