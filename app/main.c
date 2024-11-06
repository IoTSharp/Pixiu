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
#include  "parson.h"
#include "parson_ex.h"
#include  "app.h"
#include "pixiuserver.h"

int main(int argc, char **argv)
{
	CreateHttpServerThread();
	int result =App_Init();
	int __update_time = 0;
	int __upload_evnet_data = 0;

	do
	{
	
		if (result == 0)
		{
			
		
			time_t current_time;
			char* c_time_string;
			BITS_IOT _biotdata = { 0 };
			__update_time++;
			if (__update_time == 1)
			{
				SyncDateTime();
			}
			else if (__update_time > 300)
			{
				__update_time = 0;
			}
			__upload_evnet_data++;
			if (__upload_evnet_data > 5)
			{
				current_time = time(NULL);
				// 转换为本地时间字符串形式
				c_time_string = ctime(&current_time);
				int result = read_bits_iot_rs485_data(GetRS485(), 9600, &_biotdata);
				if (result == 0)
				{
					echo_app("%s 温度:%5.2f℃  湿度:%5.2f%%  烟雾浓度:%dppm PM1.0:%dug/m3 PM2.5%dug/m3 PM10:%dug/m3\n", c_time_string, _biotdata._temperature, _biotdata._humidity, _biotdata._smog, _biotdata._PM1_0, _biotdata._PM2_5, _biotdata._PM10);	
					UploadEvnData((BITS_IOT*)&_biotdata);
				}
				else 
				{
					echo_app("读传感器数据出错:%d", result);
				}
				__upload_evnet_data = 0;
			}
		
			CheckDoorStatus();
		}
		else 
		{
			echo_app("配置错误:%d，请配置设备。", result);
		}
		sleep(1);
	} while (1);
	return 0;
}

