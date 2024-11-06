#define HTTPSERVER_IMPL

#include <sys/stat.h>
#include <pthread.h>
#include <ctype.h>
#include <semaphore.h>
#include <sys/reboot.h>
#include <signal.h>
#include "httpserver.h"
#include "pixiuserver.h"
#include  "mimetype.h"
#include <parson_ex.h>
#include  <app.h>


#define RESPONSE "Hello, World!"
#define RESPONSE_ERR_404 "Oops. 404, you can contact 100860505 with QQ."
#define  FILE_NETWORK_FORMAT "#!/bin/sh\n/sbin/ifconfig lo 127.0.0.1\nifconfig eth0 up\nifconfig eth0 %s netmask %s broadcast %s\nroute add default gw %s\n"
#define  FILE_S04APP_FORMAT  "#!/bin/sh\n\necho %d > /sys/class/gpio/export\necho in > /sys/class/gpio/GPIO%d/direction\necho %d > /sys/class/gpio/export\necho in > /sys/class/gpio/GPIO%d/direction\ncd /app/\n./Run.sh &\n\n"
#define  FILE_RUN_SH  "#!/bin/bash\nwhile true\ndo\n chmod 777 ./pixiu\n ./pixiu\n sleep 1\ndone\n"

int request_target_is(struct http_request_s* request, char const* target) {
	http_string_t url = http_request_target(request);
	int len = strlen(target);
	return len == url.len && memcmp(url.buf, target, url.len) == 0;
}

int request_target_like(struct http_request_s* request, char const* target) {
	http_string_t url = http_request_target(request);
	int len = strlen(target);
	return url.len > len && memcmp(url.buf, target, len) == 0;
}
int chunk_count = 0;

void chunk_cb(struct http_request_s* request) {
	chunk_count++;
	struct http_response_s* response = http_response_init();
	http_response_body(response, RESPONSE, sizeof(RESPONSE) - 1);
	if (chunk_count < 3) {
		http_respond_chunk(request, response, chunk_cb);
	}
	else {
		http_response_header(response, "Foo-Header", "bar");
		http_respond_chunk_end(request, response);
	}
}

typedef struct {
	char* buf;
	struct http_response_s* response;
	int index;
} chunk_buf_t;

void chunk_req_cb(struct http_request_s* request) {
	http_string_t str = http_request_chunk(request);
	chunk_buf_t* chunk_buffer = (chunk_buf_t*)http_request_userdata(request);
	if (str.len > 0) {
		memcpy(chunk_buffer->buf + chunk_buffer->index, str.buf, str.len);
		chunk_buffer->index += str.len;
		http_request_read_chunk(request, chunk_req_cb);
	}
	else {
		http_response_body(chunk_buffer->response, chunk_buffer->buf, chunk_buffer->index);
		http_respond(request, chunk_buffer->response);
		free(chunk_buffer->buf);
		free(chunk_buffer);
	}
}

struct http_server_s* poll_server;

sem_t _handle;  
void handle_request(struct http_request_s* request) {
	chunk_count = 0;
	sem_wait(&_handle);
	http_request_connection(request, HTTP_CLOSE);
	struct http_response_s* response = http_response_init();
	http_response_status(response, 200);
	if (request_target_is(request, "/heartbeat"))
	{
		Http_Resp_Json(request, response, "OK", NULL);
	}
	else  if (request_target_like(request, "/api/"))
	{
		http_string_t _head = http_request_header(request, "Authorization");
		http_string_t  reqbody = http_request_body(request);
		char* _json = (char*)calloc(1, reqbody.len * 2);
		memcpy(_json, reqbody.buf, reqbody.len);
		JSON_Value*  _json_request_value = json_parse_string(_json);
		JSON_Object* _json_request = json_value_get_object(_json_request_value);
		if (request_target_is(request, "/api/config"))
		{
			JSON_Value* root_value =  NULL;
			JSON_Object* root_object = NULL;
			if (root_value == NULL) root_value = json_value_init_object();
			root_object = json_value_get_object(root_value);
			JSON_Value* cfg_value =  json_parse_file(CFG_JSON);
			json_object_set_value(root_object, "iot", json_value_deep_copy(cfg_value));
			FILE *fp  = fopen("/etc/init.d/S03network", "r");
			if (fp != NULL)
			{
				char _ip[20] = { 0 };
				char _gw[20] = { 0 };
				char _netmask[20] = { 0 };
				char _broadcast[20] = { 0 };
				int fcf = fscanf(fp, FILE_NETWORK_FORMAT, _ip, _netmask, _broadcast, _gw);
				fclose(fp);
				echo_httpserver("fcf=%d  _ip=%s", fcf, _ip);
				json_object_dotset_string(root_object, "ip.ipaddress", _ip);
				json_object_dotset_string(root_object, "ip.gateway", _gw);
				json_object_dotset_string(root_object, "ip.netmask", _netmask);
				json_object_dotset_string(root_object, "ip.broadcast", _broadcast);
			}
			FILE *fp2 =  fopen("/etc/init.d/S04App", "r");
			if (fp2 != NULL)
			{
				int in1 = 0;
				int in12;
				int in2 = 0;
				int in22;
				fscanf(fp2, FILE_S04APP_FORMAT, &in1, &in12, &in2, &in22);
				fclose(fp2);
				json_object_dotset_number(root_object, "env.in1", in1);
				json_object_dotset_number(root_object, "env.in2", in2);
			}
			Http_Resp_Json_Code(request, response, "读取成功", root_value, 200);	
			json_value_free(root_value);
			json_value_free(cfg_value);
		}
		else if (_head.len > 0 && strncmp("kissme", _head.buf, _head.len))
		{
			if (request_target_is(request, "/api/iot_config"))
			{
				
				JSON_Value* root_value =  json_parse_file(CFG_JSON);
				JSON_Object* root_object = NULL;
				if (root_value == NULL) root_value = json_value_init_object();
				root_object = json_value_get_object(root_value);
				if (json_object_has_value_of_type(_json_request, "server", JSONString))
				{
					const char* server = json_object_get_string(_json_request, "server");
					json_object_set_string(root_object, "server", server);
					echo_httpserver("server=%s", server);
				}
				if (json_object_has_value_of_type(_json_request, "accessToken", JSONString))
				{
					const char* accessToken = json_object_get_string(_json_request, "accessToken");
					json_object_set_string(root_object, "accessToken", accessToken);
					echo_httpserver("accessToken=%s", accessToken);
				}
				JSON_Status return_code = json_serialize_to_file_pretty(root_value, CFG_JSON);
				if (return_code == JSONSuccess)
				{
					Http_Resp_Json_Code(request, response, "物联网配置成功", root_value, 200);	
				}
				else 
				{
					Http_Resp_Text(request, response, 500, return_code, "物联网配置失败");
				}
				json_value_free(root_value);
		 
			}
			else if (request_target_is(request, "/api/reboot"))
			{
				sync(); 
				Http_Resp_Text(request, response, 200, 0, "即将重启");	
				reboot(RB_AUTOBOOT);
			}
			else if (request_target_is(request, "/api/ip_config"))
			{
				char  _network_text[1024] = { 0 };
				if (json_object_has_value_of_type(_json_request, "ipaddress", JSONString)
					&& json_object_has_value_of_type(_json_request, "gateway", JSONString)
					&& json_object_has_value_of_type(_json_request, "netmask", JSONString)
					&& json_object_has_value_of_type(_json_request, "broadcast", JSONString))
				{
					const char* ip = json_object_get_string(_json_request, "ipaddress");
					const char* gw = json_object_get_string(_json_request, "gateway");
					const char* netmask = json_object_get_string(_json_request, "netmask");
					const char* broadcast = json_object_get_string(_json_request, "broadcast");
					FILE *fp  = fopen("/etc/init.d/S03network", "w");
					if (fp != NULL)
					{
						fprintf(fp, FILE_NETWORK_FORMAT, ip, netmask, broadcast, gw);
						fclose(fp);
						chmod("/etc/init.d/S03network", S_IRWXU | S_IRWXG | S_IRWXO);
						Http_Resp_Json(request, response, "保存完成", NULL);
					
					}
					else 
					{
						Http_Resp_Text(request, response, 500, -1, "网络配置失败");
						
					}
				}
				else 
				{
					Http_Resp_Text(request, response, 500, -4, "请检查设置项ipaddress,gateway,netmask,broadcast");
					
				}
			}
			else if (request_target_is(request, "/api/env_config"))
			{
				if (json_object_has_value_of_type(_json_request, "in1", JSONNumber)
					&& json_object_has_value_of_type(_json_request, "in2", JSONNumber))
				{
				
					const int _in1 = json_object_get_number(_json_request, "in1");
					const int _in2 = json_object_get_number(_json_request, "in2");
					echo_httpserver("env_config %d in1=%d in2=%d \r\n%s\r\n   ", _json_request != NULL ? 1 : -1, json_object_get_number(_json_request, "in1"), json_object_get_number(_json_request, "in2"), _json);
					FILE *fp =  fopen("/etc/init.d/S04App", "w");
					if (fp != NULL)
					{
						fprintf(fp, FILE_S04APP_FORMAT, _in1, _in1, _in2, _in2);
						fclose(fp);
						chmod("/etc/init.d/S04App", S_IRWXU | S_IRWXG | S_IRWXO);
						FILE *fprim  = fopen("/app/Run.sh", "w");
						if (fprim != NULL)
						{
							fputs(FILE_RUN_SH, fprim);
							fclose(fprim);
							chmod("/app/Run.sh", S_IRWXU | S_IRWXG | S_IRWXO);
							Http_Resp_Json(request, response, "环境配置完成", NULL);
						}
						else 
						{
							Http_Resp_Text(request, response, 500, -1, "环境启动配置失败");
						}
					
					}
					else 
					{
						Http_Resp_Text(request, response, 500, -2, "环境配置失败");
					}
				}
				else 
				{
					Http_Resp_Text(request, response, 500, -3, "环境配置失败，请检查设置项in1,in2");
					
				}
			}
			
			else 
			{
				Http_Resp_Text(request, response, 404, -1, "未找到此页面");
			}
		}
		else
		{
			Http_Resp_Text(request, response, 401, -1, "无权限，验证码信息%s错误", _head.buf);
		}
		json_value_free(_json_request_value);
		free(_json);
	}
	
	else if (access("./wwwroot", R_OK) == 0) 
	{
		http_string_t url = http_request_target(request);
		char  _urlpath[255] = { 0 };
		strncpy(_urlpath, url.buf, url.len);
		char  filename[1024] = { 0 };
		if (url.len == 0 ||  (url.len == 1 &&  strncmp(_urlpath, "/", 1) == 0) || strncmp(_urlpath, ".", 1) == 0)
		{
			sprintf(_urlpath, "%s", "/index.html");
		}
		char path[PATH_MAX]; //PATH_MAX is defined in limits.h
		getcwd(path, sizeof(path));
		sprintf(filename, "%s/wwwroot%s", path, _urlpath);
		echo_httpserver("要访问的文件路径是:%s  %s %s", path, _urlpath, filename);
		if (access(filename, R_OK) == 0) 
		{
			http_response_mimetype(response, filename);
			FILE *fp  = fopen(filename, "r"); //打开文件
			if (fp != NULL)
			{ 
				fseek(fp, 0, SEEK_END); //将文件指针指向该文件的最后
				int	file_size = ftell(fp); //根据指针位置，此时可以算出文件的字符数
				char *_filebody = (char *)malloc(file_size*sizeof(char)); //根据文件大小为tmp动态分配空间
				if (_filebody != NULL)
				{
					memset(_filebody, '\0', file_size*sizeof(char)); //初始化此控件内容，否则可能会有乱码
					fseek(fp, 0, SEEK_SET); //重新将指针指向文件首部
					fread(_filebody, sizeof(char), file_size, fp); //开始读取整个文件
					http_response_body(response, _filebody, file_size);
					http_response_status(response, 200);
					http_respond(request, response);
					free(_filebody);
				}
				else 
				{
					Http_Resp_Msg(request, response, 500, "错误代码 -3,处理文件%s时内存不足。", _urlpath);
				}
			}
			else 
			{
				
				Http_Resp_Msg(request, response, 500, "错误代码 -2,文件%s无法读取", _urlpath);
			}
		}
		else 
		{
			Http_Resp_Msg(request, response, 404, "错误代码 -1, 文件%s找不到", _urlpath);
		}
		
	}
	else
	{
 
		Http_Resp_Msg(request, response, 404, RESPONSE_ERR_404);
	}
	sem_post(&_handle);
} 
 
 

struct http_server_s* server;
static pthread_t ToHttpServerThread;
int CreateHttpServerThread(void)
{
	int ret;
	pthread_attr_t new_attr;
	pthread_attr_init(&new_attr);
	pthread_attr_setdetachstate(&new_attr, PTHREAD_CREATE_DETACHED);
	ret = pthread_create(&ToHttpServerThread, &new_attr, (void*)HttpServerMain, NULL);
	pthread_attr_destroy(&new_attr);

	return ret;
}

void  HttpServerMain(void) {

	sem_init(&_handle, 0, 1);
	server = http_server_init(10000, handle_request);
	echo_httpserver("HTTP服务已经初始化");
	http_server_listen(server);
	
}
void Http_Resp_Msg(struct http_request_s* request, struct http_response_s* response, int httpcode, char* Format, ...)
{
	char buffer[4096] = { 0 };
	va_list args;
	va_start(args, Format);
	vsprintf(buffer, Format, args);
	va_end(args);
	char _html_buffer[8096] = { 0 };
	sprintf(_html_buffer, "<!DOCTYPE html><html><head><title>Message</title><meta charset=\"utf-8\"></head><body><h1>Message</h1><p>%s</p></body></html>", buffer);
	http_response_header(response, "Content-Type", "text/html");
	http_response_body(response, _html_buffer, strnlen(_html_buffer, sizeof(_html_buffer)));
	http_response_status(response, httpcode);
	http_respond(request, response);
}

void Http_Resp_Bot(struct http_request_s* request, struct http_response_s* response, int optStatus, char* Format, ...)
{
	char buffer[4096] = { 0 };
	va_list args;
	va_start(args, Format);
	vsprintf(buffer, Format, args);
	va_end(args);
	Http_Resp_Text(request, response, 200, optStatus, buffer);
}
void Http_Resp_Json(struct http_request_s* request, struct http_response_s* response, char* msg, JSON_Value* jo)
{
	JSON_Value* root_value = json_value_init_object();
	JSON_Object* root_object = json_value_get_object(root_value);
	char* serialized_string = NULL;
	http_response_header(response, "Content-Type", "application/json");
	json_object_set_number(root_object, "code", 200);
	json_object_set_string(root_object, "msg", msg);
	if (jo != NULL)
	{
		json_object_set_value(root_object, "data", json_value_deep_copy(jo));
	}
	serialized_string = json_serialize_to_string_pretty(root_value);
	http_response_body(response, serialized_string, strlen(serialized_string));
	echo_httpserver("http_response_body:%s", serialized_string);
	http_response_status(response, 200);
	http_respond(request, response);
	free(serialized_string);
	json_value_free(root_value);

}




void Http_Resp_Json_Code(struct http_request_s* request, struct http_response_s* response, char* msg, JSON_Value* jo, int code)
{
	JSON_Value* root_value = json_value_init_object();
	JSON_Object* root_object = json_value_get_object(root_value);
	char* serialized_string = NULL;
	http_response_header(response, "Content-Type", "application/json");
	json_object_set_number(root_object, "code", code);
	json_object_set_string(root_object, "msg", msg);
	if (jo != NULL)
	{
		json_object_set_value(root_object, "data", json_value_deep_copy(jo));
	}
	serialized_string = json_serialize_to_string_pretty(root_value);
	http_response_body(response, serialized_string, strlen(serialized_string));
	echo_httpserver("http_response_body:%s", serialized_string);
	http_response_status(response, 200);
	http_respond(request, response);
	free(serialized_string);
	json_value_free(root_value);

}

void Http_Resp_Text(struct http_request_s* request, struct http_response_s* response, int httpcode, int code, char* Format, ...) {
	char buffer[4096] = { 0 };
	va_list args;
	va_start(args, Format);
	vsprintf(buffer, Format, args);
	va_end(args);
	http_response_header(response, "Content-Type", "application/json");
	JSON_Value* root_value = json_value_init_object();
	JSON_Object* root_object = json_value_get_object(root_value);
	char* serialized_string = NULL;
	json_object_set_number(root_object, "code", code);
	json_object_set_string(root_object, "msg", buffer);
	serialized_string = json_serialize_to_string_pretty(root_value);
	http_response_body(response, serialized_string, strlen(serialized_string));
	echo_httpserver("http_response_body:%s", serialized_string);
	http_response_status(response, httpcode);
	http_respond(request, response);
	free(serialized_string);
	json_value_free(root_value);
}

 