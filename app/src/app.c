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
#include <curl/curl.h>
#include <curl/easy.h>
#include  <curl_utils.h>
#include <parson.h>
#include <parson_ex.h>
#include  <bits_iot_rs485_.h>
#include  "app.h"
#include  <uuid4.h>


char _iot_server[256] = { 0 };
char _accessToken[256] = { 0 };
 

char _rs485[256] = { 0 };

char * GetRS485(void)
{
	return _rs485;
}
int  App_Init(void)
{
	int result = 0;
	uuid4_init();
	
	if ((access(CFG_JSON, 0) == 0))
	{
		JSON_Value* _value = json_parse_file(CFG_JSON);
		if (_value != NULL)
		{
			JSON_Object* _object = json_value_get_object(_value);
			if (json_object_dothas_value_of_type(_object, "server", JSONString))
			{
				const char* server = json_object_dotget_string(_object, "server");
				strncpy(_iot_server, server, 256);
			}
			else 
			{
				strncpy(_iot_server, "host.iotsharp.net", 256);
			}
			if (json_object_dothas_value_of_type(_object, "rs485", JSONString))
			{
				const char* rs485 = json_object_dotget_string(_object, "rs485");
				strncpy(_rs485, rs485, 256);
			}
			else 
			{
				strncpy(_rs485, "/dev/ttyS1", 256);
			}
			if (json_object_dothas_value_of_type(_object, "accessToken", JSONString))
			{
				const char* accessToken = json_object_dotget_string(_object, "accessToken");
				strncpy(_accessToken, accessToken, 256);
			}
			else 
			{
				strcpy(_accessToken, "");
			}
		}
		else
		{
			perror("配置文件未能正常加载 ");
			result = -1;
		}
	}
	else
	{
		perror("未找到配置文件 ");
		result = -2;
	}
	return result;
}

void PrintHEX(char * titel, const unsigned char * data, int datlen)
{
	int i;
	fprintf(stderr, "%s LEN:%d DATA:[", titel, datlen);
	for (i = 0; i < datlen; i++)
	{
		fprintf(stderr, "%02X", data[i]);
	}
	fprintf(stderr, "]\n");
}

void UploadEvnData(BITS_IOT* _biotdata)
{
	JSON_Value* root_value = NULL;
	JSON_Object* root_object = NULL;
	root_value = json_value_init_object();
	root_object = json_value_get_object(root_value);
	json_object_set_number(root_object, "temp", _biotdata->_temperature);
	json_object_set_number(root_object, "humidity", _biotdata->_humidity);
	json_object_set_number(root_object, "pm25", _biotdata->_PM2_5);
	json_object_set_number(root_object, "pm10", _biotdata->_PM10);
	json_object_set_number(root_object, "pm1", _biotdata->_PM1_0);
	json_object_set_number(root_object, "smog", _biotdata->_smog);
	char* serialized_string = NULL;
	serialized_string = json_serialize_to_string_pretty(root_value);
	struct MemoryStruct  response = { NULL, 0 };
	long responseCode = -1;
	char _url_telemetry[255] = { 0 };
	sprintf(_url_telemetry, "api/devices/%s/telemetry", _accessToken);
	int  ret_1 = RequestURL_Raw(_iot_server, _url_telemetry, POST, serialized_string, &responseCode, &response, 5);
	if (ret_1 == 1  && response.size > 0)
	{
		JSON_Value* root_value_2;
		root_value_2 = json_parse_string(response.memory);
		if (json_value_get_type(root_value_2) == JSONObject)
		{
			JSON_Object* jsonobj = json_value_get_object(root_value_2);
			if (json_object_has_value_of_type(jsonobj, "code", JSONNumber) && json_object_has_value_of_type(jsonobj, "msg", JSONString))
			{
				int  _code = json_object_get_number(jsonobj, "code");
				char msg[256] = { 0 };
				strncpy(msg, json_object_get_string(jsonobj, "msg"), 255);
				echo_app("返回代码:%d  消息:%s", _code, msg);
			}
			else
			{
				echo_app("缺少指定字段:%d 响应代码:%ld", ret_1, responseCode);
			}
		}
		else
		{
			echo_app("返回格式不是Json:%d 响应代码:%ld", ret_1, responseCode);
		}
		if (root_value_2 != NULL)
		{
			json_value_free(root_value_2);
		}
	}
	else
	{
		echo_app("请求失败:%d 响应代码:%ld", ret_1, responseCode);
	}
	json_free_serialized_string(serialized_string);
	FreeMemoryStruct(&response);
}

void SyncDateTime(void)
{
	struct MemoryStruct  response = { NULL, 0 };
	long responseCode = -1;
	int  ret_1 = RequestURL_Raw(_iot_server, "api/home/now", GET, "{}", &responseCode, &response, 5);
	if (ret_1 == 1  && response.size > 0)
	{
		JSON_Value* root_value_2 = json_parse_string(response.memory);
		if (json_value_get_type(root_value_2) == JSONObject)
		{
			JSON_Object* jsonobj = json_value_get_object(root_value_2);
			if (json_object_has_value_of_type(jsonobj, "code", JSONNumber) && json_object_has_value_of_type(jsonobj, "msg", JSONString))
			{
				int  _code = json_object_get_number(jsonobj, "code");
				char msg[256] = { 0 };
				strncpy(msg, json_object_get_string(jsonobj, "msg"), 255);
				echo_app("返回代码:%d  消息:%s", _code, msg);
				if (json_object_dothas_value_of_type(jsonobj, "data.nowString", JSONString))
				{
					struct tm tm;
					const char* _data= json_object_dotget_string(jsonobj, "data.nowString");
					// "2024-07-23 10:25:00"
					// 将日期字符串解析成 struct tm
					if (strptime(_data, "%Y-%m-%d %H:%M:%S", (struct tm*)&tm) == NULL) 
					{
						echo_app("解析日期%s失败。", _data);
					}
					else 
					{
					
						char buf[255] = { 0 };
						// 使用 strftime 打印易读格式的日期
						strftime(buf, sizeof(buf), "%A, %B %d, %Y", (struct tm*) &tm);
						echo_app("已解析的日期: %s\n", buf);
						time_t timep = mktime(&tm);
						struct timeval tv;  
						tv.tv_sec = timep;  
						tv.tv_usec = 0;  
						if (settimeofday(&tv, NULL) < 0)  
						{  
							echo_app("设置系统时间错误！");  
						}
						else 
						{
							echo_app("设置系统时间成功！");  
							
						}
					}
				}
				else 
				{
					echo_app("缺少时间字段:%d 响应代码:%ld", ret_1, responseCode);
					
				}
			}
			else
			{
				echo_app("缺少指定字段:%d 响应代码:%ld", ret_1, responseCode);
			}
		}
		else
		{
			echo_app("返回格式不是Json:%d 响应代码:%ld", ret_1, responseCode);
		}
		if (root_value_2 != NULL)
		{
			json_value_free(root_value_2);
		}
	}
	else
	{
		echo_app("请求失败:%d 响应代码:%ld", ret_1, responseCode);
	}
	FreeMemoryStruct(&response);
}
	



void UploadGpioValue(char* gpioname,int gpiovalue)
{
	JSON_Value* root_value = NULL;
	JSON_Object* root_object = NULL;
	root_value = json_value_init_object();
	root_object = json_value_get_object(root_value);
	json_object_dotset_number(root_object, gpioname, gpiovalue);
	char* serialized_string = NULL;
	serialized_string = json_serialize_to_string_pretty(root_value);
	struct MemoryStruct  response = { NULL, 0 };
	long responseCode = -1;
	char _url_telemetry[255] = { 0 };
	sprintf(_url_telemetry, "api/devices/%s/telemetry", _accessToken);
	int  ret_1 = RequestURL_Raw(_iot_server, _url_telemetry, POST, serialized_string, &responseCode, &response, 5);
	if (ret_1 == 1  && response.size > 0)
	{
		JSON_Value* root_value_2;
		root_value_2 = json_parse_string(response.memory);
		if (json_value_get_type(root_value_2) == JSONObject)
		{
			JSON_Object* jsonobj = json_value_get_object(root_value_2);
			if (json_object_has_value_of_type(jsonobj, "code", JSONNumber) && json_object_has_value_of_type(jsonobj, "msg", JSONString))
			{
				int  _code = json_object_get_number(jsonobj, "code");
				char msg[256] = { 0 };
				strncpy(msg, json_object_get_string(jsonobj, "msg"), 255);
				echo_app("返回代码:%d  消息:%s", _code, msg);
				if (json_object_has_value_of_type(jsonobj, "response_code_string", JSONString))
				{
					char response_code_string[256] = { 0 };
					strncpy(response_code_string, json_object_get_string(jsonobj, "msg"), 255);
					echo_app("回复内容:%s", response_code_string);
				}
			}
			else
			{
				echo_app("缺少指定字段:%d 响应代码:%ld", ret_1, responseCode);
			}
		}
		else
		{
			echo_app("返回格式不是Json:%d 响应代码:%ld", ret_1, responseCode);
		}
		if (root_value_2 != NULL)
		{
			json_value_free(root_value_2);
		}
	}
	else
	{
		echo_app("请求失败:%d 响应代码:%ld", ret_1, responseCode);
	}
	json_free_serialized_string(serialized_string);
	FreeMemoryStruct(&response);
}

int __door_status = -1;

void CheckDoorStatus(void)
{
	 char buf[2] = { 0 };
	char _gpio_path[100] = { 0 };
	sprintf(_gpio_path, "/sys/class/gpio/GPIO%d/value" , _gpio_door_id);
	if ((access(_gpio_path, 0) == 0))
	{
		FILE *pf = fopen(_gpio_path, "r");
		if (pf == NULL)
		{
			echo_app("无法读取GPIO文件%s", _gpio_path);
		}
		else 
		{
			int readlen = fread(buf, 1, 1, pf);  
			fclose(pf);
			if (readlen > 0)
			{
				int _value = buf[0] - (char)'0';
				if (__door_status != _value)
				{
					__door_status = _value;
					echo_app("门状态切换为:%d", __door_status);
					UploadGpioValue("door",__door_status);
				}
			}
			else 
			{
				echo_app("GPIO%d读取失败:%d", _gpio_door_id, readlen);
			}
		}
	}
	else 
	{
		echo_app("GPIO%d不存在", _gpio_door_id);
	}
}