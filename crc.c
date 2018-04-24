#include <stdio.h>

uint8_t data[] = {0x01,0x00,0x6c,0xaa,0x12,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x02,0xd1,0x00,0x38};
unsigned short CRC1(unsigned char *Data,unsigned char Data_length)
{
	unsigned int mid=0;
	unsigned char times=0,Data_index=0;
	unsigned short CRC_data=0xFFFF;
	while(Data_length)
	{
		CRC_data=Data[Data_index]^CRC_data;
		for(times=0;times<8;times++)
		{
			mid=CRC_data;
			CRC_data=CRC_data>>1;
			if(mid & 0x0001)
			{
				CRC_data=CRC_data^0xA001;
			}
		}
		Data_index++;
		Data_length--;
	}
	return CRC_data;
}

int main(int argc, void *argv[])
{
	printf("crc is %x\r\n", CRC1(data,sizeof(data)));
	return 0;
}
