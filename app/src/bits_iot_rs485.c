#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include  "app.h"
#include  "crc.h"
#include  "rs485.h"
#include  "bits_iot_rs485_.h"

int  read_bits_iot_rs485_data(char *dev ,int speed,BITS_IOT* _biotdata)
{
	unsigned char rev_buf[512];
	int send_len;
	int rev_len;
	int fd;
	int result = -1;
	fd = open_dev(dev);
	set_speed(fd, speed);
	config_s(fd); // it's so important that we must face this.
	if (set_parity(fd, 8, 1, 'N') == FALSE)  
	{
		result = -2;
		echo_app("set %d parity error %d", fd, result);
	}
	else
	{
		char send_buf_tmp[] = { 0x01, 0x03, 0x00, 0x01, 0x00, 0x0B, 0x55, 0xCD };
		send_len = write(fd, send_buf_tmp, 8);
		sleep(1);
		
		if ((rev_len = read(fd, rev_buf, 512)) == -1)
		{
			result = -3;
			echo_app("read %d  error  -3, send %d  rev%d", fd, send_len, rev_len);
		}
		else if (rev_len >= 27)
		{
			PrintHEX("rev", rev_buf, rev_len);
			/**
			 010316 1363 0915 0003 0000 0005 0006 0004 0005 0005 000A 0000 25C6
			 010316 AAAA BBBB CCCC DDDD EEEE FFFF GGGG HHHH IIII JJJJ KKKK UVWX
			        0    1    2    3    4    5    6    7    8    9    10
			     010316 固定格式不变，AAAA 位置为温度值，BBBB 位置为湿度值，
			     GGGG 位置为 PM1.0 值，HHHH 位置为 PM2.5值，
			     IIII 位置为 PM10 值，KKKK 位置为烟雾值(从 AAAA 位置开始第11 组数据)，
			     CCCC 至 FFFF，JJJJ，为系统默认数值无意义。 UVWX 位
			     **/
			ushort* _value = (ushort*)&rev_buf[3];
			_biotdata->_temperature = ((float)htobe16(_value[0]) - 2000.0) / 100.0;
			_biotdata->_humidity = (float)htobe16(_value[1]) / 100.0;
			_biotdata->_smog = htobe16(_value[10]);
			_biotdata->_PM1_0 =  htobe16(_value[6]);
			_biotdata->_PM2_5 =  htobe16(_value[7]);
			_biotdata->_PM10 = htobe16(_value[8]);
		
			result = 0;
			echo_app("read %d success 0", fd);
		}
		else 
		{
			result = -4;
			PrintHEX("数据错误", rev_buf, rev_len);
			echo_app("read %d error -4", fd);
		
		}
	}
	close(fd);
	return result;
}