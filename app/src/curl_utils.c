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

//代码转换:从一种编码转为另一种编码
 


CURLcode _global_init = CURLE_HTTP_POST_ERROR;
CURLcode check_curl(void)
{
	if (_global_init != CURLE_OK)
	{
		_global_init = curl_global_init(CURL_GLOBAL_ALL);
	}
	if (CURLE_OK != _global_init)
	{
		echo_curl("curl init failed");
	}
	return _global_init;
}

size_t WriteMemoryCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
	size_t realsize = size * nmemb;
	struct MemoryStruct* mem = (struct MemoryStruct*)userp;
	char* ptr = (char*)realloc(mem->memory, mem->size + realsize + 1);
	if (ptr == NULL) {
		/* out of memory! */
		echo_curl("not enough memory (realloc returned NULL) need %zu  bytes", mem->size + realsize + 1);
		return 0;
	}
	mem->memory = ptr;
	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;
	return realsize;
}
void FreeMemoryStruct(struct MemoryStruct* fc)
{
	if (fc->memory!=NULL )
	{
		free(fc->memory);
	}
	fc->size = 0;
}


int  RequestURL_RawOAuth(char* requestAddr, char* api, char* requestMode, char* jsonStr, long* respondCode, struct MemoryStruct* responseStr, int timeOut, char* _Bearer)
{
	int  result = 0;
	if (check_curl() == CURLE_OK)
	{
		CURL* hnd = curl_easy_init();
		curl_easy_setopt(hnd, CURLOPT_CUSTOMREQUEST, requestMode);
		char _tmpcurl1[300] = { 0 };
		sprintf(_tmpcurl1, "http://%s/%s", requestAddr, api);
		echo_curl("request-addr[%s]", _tmpcurl1);
		curl_easy_setopt(hnd, CURLOPT_URL, _tmpcurl1);
		struct curl_slist* headers = NULL;
		headers = curl_slist_append(headers, "cache-control: no-cache");
		headers = curl_slist_append(headers, "Connection: keep-alive");
		if (_Bearer != NULL)
		{
			char _temp_bearer[512] = { 0 };
			sprintf(_temp_bearer, "Authorization: Bearer %s", _Bearer);
			headers = curl_slist_append(headers, _temp_bearer);
		}
		char  _tmpcontentlen[50] = { 0 };
		curl_easy_setopt(hnd, CURLOPT_POSTFIELDS, jsonStr);
		sprintf(_tmpcontentlen, "Content-Length: %d", strlen(jsonStr));
		headers = curl_slist_append(headers, _tmpcontentlen);
		headers = curl_slist_append(headers, "Content-Type: application/json");
		headers = curl_slist_append(headers, "Cache-Control: no-cache");
		headers = curl_slist_append(headers, "Accept: */*");
		headers = curl_slist_append(headers, "User-Agent: ya_an_iot");
		curl_easy_setopt(hnd, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(hnd, CURLOPT_WRITEDATA, responseStr);
		curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
		curl_easy_setopt(hnd, CURLOPT_VERBOSE, 1L); //启用时会汇报所有的信息
		curl_easy_setopt(hnd, CURLOPT_TIMEOUT, timeOut); //10秒超时
		CURLcode ret = curl_easy_perform(hnd);
			
		if (ret == CURLE_OK)
		{
			CURLcode res = curl_easy_getinfo(hnd, CURLINFO_RESPONSE_CODE, respondCode);
			if (*respondCode != 200)
			{
				echo_curl("请求[%s]的RESPONSE_CODE的是%ld", _tmpcurl1, *respondCode);
			}
			if (res == CURLE_OK)
			{
				result = (*respondCode >= 100 &&  *respondCode <= 600) ? 1 : 0;
			}
			else
			{
				echo_curl("获取请求[%s]RESPONSE_CODE时返回%d", _tmpcurl1, res);
			}

		}
		else
		{
			echo_curl("执行请求[%s]时返回%d - %s", _tmpcurl1, (int)ret, curl_easy_strerror(ret));
		}
		curl_easy_cleanup(hnd);
		curl_slist_free_all(headers);
	}
	return result;
}

int  RequestURL_Raw(char* requestAddr, char* api, char* requestMode, char* jsonStr, long* respondCode, struct MemoryStruct* responseStr, int timeOut)
{
	return RequestURL_RawOAuth(requestAddr, api, requestMode, jsonStr, respondCode, responseStr, timeOut, NULL);
}