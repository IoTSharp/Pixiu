#ifndef __CurlCommon_H__
#define __CurlCommon_H__

#include <curl/curl.h>



#define POST  "POST"
#define GET  "GET"
#define PUT  "PUT"
#pragma pack(push,1)

struct MemoryStruct 
{
	char* memory;
	size_t size;
};

 
#pragma pack(pop)
CURLcode check_curl(void);
size_t WriteMemoryCallback(void* contents, size_t size, size_t nmemb, void* userp);
void FreeMemoryStruct(struct MemoryStruct* fc);
int  RequestURL_RawOAuth(char* requestAddr, char* api, char* requestMode, char* jsonStr, long* respondCode, struct MemoryStruct* responseStr, int timeOut, char* _Bearer);
int  RequestURL_Raw(char* requestAddr, char* api, char* requestMode, char* jsonStr, long* respondCode, struct MemoryStruct * responseStr, int timeOut);
#define echo_curl(fmt, args...)	  printf("curl %s[%s:%d]: " fmt "\n", __func__,__FILE__, __LINE__ , ## args) 
#endif