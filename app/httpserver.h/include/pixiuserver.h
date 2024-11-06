#ifndef LaneSERVER_H
#define LaneSERVER_H
#include  "httpserver.h"
#include <parson.h>
int request_target_is(struct http_request_s* request, char const* target);
int request_target_like(struct http_request_s* request, char const* target);
void chunk_cb(struct http_request_s* request);
void chunk_req_cb(struct http_request_s* request);
void handle_request(struct http_request_s* request);
 
int CreateHttpServerThread(void);
void  HttpServerMain(void);
 
void Http_Resp_Json(struct http_request_s* request, struct http_response_s* response, char* msg, JSON_Value* jo);
void Http_Resp_Json_Code(struct http_request_s* request, struct http_response_s* response, char* msg, JSON_Value* jo, int code);
void Http_Resp_Text(struct http_request_s* request, struct http_response_s* response, int httpcode, int code, char* Format, ...);
void Http_Resp_Bot(struct http_request_s* request, struct http_response_s* response, int optStatus, char* Format, ...);

void Http_Resp_Msg(struct http_request_s* request, struct http_response_s* response, int httpcode, char* Format, ...);
enum opStatus_Code
{
	OPT_SUCCESS = 0,
	OPT_FAIL    = 1,
	OPT_REFUSE  = 2
};
#define echo_httpserver(fmt, args...)	  printf("httpserver %s[%s:%d]: " fmt "\n", __func__,__FILE__, __LINE__ , ## args) 


#endif