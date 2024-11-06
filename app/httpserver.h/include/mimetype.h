#ifndef MIMETYPE_H
#define MIMETYPE_H
#include  "httpserver.h"
struct _context_item
{
	
	char* extname;
	char* _type;
};
#define  _MIMETYPE_MAX  559
int  http_response_mimetype(struct http_response_s* response, char* filename);

#define echo_mimer(fmt, args...)	  printf("MIME %s[%s:%d]: " fmt "\n", __func__,__FILE__, __LINE__ , ## args) 

#endif
