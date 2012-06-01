#ifndef _FLY_SOC_IIC_
#define _FLY_SOC_IIC_

//IIC
//bus_id : 0/1
int SOC_I2C_Send(int bus_id, char chip_addr, char *buf, unsigned int size);
int SOC_I2C_Rec(int bus_id, char chip_addr, unsigned int sub_addr, char *buf, unsigned int size);
int SOC_I2C_Rec_Simple(int bus_id, char chip_addr, char *buf, unsigned int size);//without sub_addr
int SOC_I2C_Rec_SAF7741(int bus_id, char chip_addr, unsigned int sub_addr, char *buf, unsigned int size);
int SOC_I2C_Send_TEF7000(int bus_id, char chip_addr, unsigned int sub_addr, char *buf, unsigned int size);
int SOC_I2C_Rec_TEF7000(int bus_id, char chip_addr, unsigned int sub_addr, char *buf, unsigned int size);

#endif
