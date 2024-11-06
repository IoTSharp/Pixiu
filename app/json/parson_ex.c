#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <errno.h>
#include <stddef.h>   /* size_t */
#include <stdarg.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include "parson.h"
#include "parson_ex.h"
#define MAX_NESTING       20480

JSON_Status json_object_set_string_args(JSON_Object* object, const char* name, char* Format, ...)
{
    char buffer[MAX_NESTING] = { 0 };
    va_list args;
    va_start(args, Format);
    vsnprintf(buffer, MAX_NESTING, Format, args);
    va_end(args);
    return json_object_set_string(object, name, buffer);
}
JSON_Status json_array_append_string_args(JSON_Array* array,char* Format, ...)
{
    char buffer[MAX_NESTING] = { 0 };
    va_list args;
    va_start(args, Format);
    vsnprintf(buffer, MAX_NESTING, Format, args);
    va_end(args);
    return json_array_append_string(array, buffer);
}

JSON_Status json_object_dotset_string_args(JSON_Object* object, const char* name, char* Format, ...) {
    char buffer[MAX_NESTING] = { 0 };
    va_list args;
    va_start(args, Format);
    vsnprintf(buffer, MAX_NESTING, Format, args);
    va_end(args);
    return json_object_dotset_string(object, name, buffer);
}

JSON_Status json_object_dotset_string_args_with_len(JSON_Object* object, const char* name, size_t len, char* Format, ...) {
    char buffer[MAX_NESTING] = { 0 };
    va_list args;
    va_start(args, Format);
    vsnprintf(buffer, MAX_NESTING, Format, args);
    va_end(args);
    return json_object_dotset_string_with_len(object, name, buffer, len);
}

JSON_Status json_object_dotset_string_ex(JSON_Object* object, const char* name, const char* string, int len)
{
    char* _jsontmp = calloc((size_t)1, (size_t)(len + 1));
    if (_jsontmp == NULL)
    {
        return  JSONFailure;
    }
    strncpy(_jsontmp, string, strnlen(string, len));
    JSON_Status result = json_object_dotset_string(object, name, _jsontmp);
    free(_jsontmp);
    return result;
}

JSON_Status json_object_set_string_ex(JSON_Object* object, const char* name, const char* string, int len)
{
    char* _jsontmp = calloc((size_t)1, (size_t)(len + 1));
    if (_jsontmp == NULL)
    {
        return  JSONFailure;
    }
    strncpy(_jsontmp, string, strnlen(string, len));
    JSON_Status result = json_object_set_string(object, name, _jsontmp);
    free(_jsontmp);
    return result;
}



ssize_t format_timeval(struct timeval* tv, char* buf, size_t sz)
{
    ssize_t written = -1;
    struct tm* gm = gmtime(&tv->tv_sec);

    if (gm)
    {
        written = (ssize_t)strftime(buf, sz, "%Y-%m-%dT%H:%M:%S", gm);
        if ((written > 0) && ((size_t)written < sz))
        {
            int w = snprintf(buf + written, sz - (size_t)written, ".%06ldZ", tv->tv_usec);
            written = (w > 0) ? written + w : -1;
        }
    }
    return written;
}

JSON_Status json_object_dotset_string_iso8601(JSON_Object* object, const char* name, struct timeval* tv)
{
	char _buffer[100] = { 0 };
	if (format_timeval(tv, (char*)&_buffer, sizeof(_buffer)) > 0)
	{
        JSON_Status result = json_object_dotset_string(object, name, _buffer);
        return result;
	}
    else
    {
        return  JSONFailure;
    }
}

JSON_Status json_object_set_string_iso8601(JSON_Object* object, const char* name, struct timeval* tv)
{
    char _buffer[100] = { 0 };
    if (format_timeval(tv, (char*)&_buffer, sizeof(_buffer)) > 0)
    {
        JSON_Status result = json_object_set_string(object, name, _buffer);
        return result;
    }
    else
    {
        return  JSONFailure;
    }
}