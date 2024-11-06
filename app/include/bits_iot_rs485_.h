#pragma once
typedef  struct _bits_iot_data
{
	float  _temperature, _humidity;
	unsigned short _smog, _PM1_0, _PM2_5, _PM10;
} BITS_IOT;

int  read_bits_iot_rs485_data(char *dev, int speed, BITS_IOT* _biotdata);
	