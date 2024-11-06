#ifndef parson_parson_ex_h
#define parson_parson_ex_h

JSON_Status json_array_append_string_args(JSON_Array* array, char* Format, ...);
JSON_Status json_object_set_string_args(JSON_Object* object, const char* name, char* Format, ...);
JSON_Status json_object_dotset_string_args(JSON_Object* object, const char* name, char* Format, ...);
JSON_Status json_object_dotset_string_ex(JSON_Object* object, const char* name, const char* string, int len);
JSON_Status json_object_set_string_ex(JSON_Object* object, const char* name, const char* string, int len);
JSON_Status json_object_dotset_string_args_with_len(JSON_Object* object, const char* name, size_t len, char* Format, ...);
JSON_Status json_object_dotset_string_iso8601(JSON_Object* object, const char* name, struct timeval* tv);
JSON_Status json_object_set_string_iso8601(JSON_Object* object, const char* name, struct timeval* tv);
#define  json_object_dotset_string_struct(__object__,__name__,__string__)  json_object_dotset_string_ex(__object__,__name__, __string__,sizeof(__string__))
#define  json_object_set_string_struct(__object__,__name__,__string__)  json_object_set_string_ex(__object__,__name__, __string__,sizeof(__string__))
#define  json_object_dotset_char_struct(__object__,__name__,__char__)  json_object_dotset_string_args_with_len(__object__, __name__,1, "%c",__char__);
#define  json_object_dotset_string_struct_iso8601(__object__,__name__,__structname_, __structmembername__)   char __tmp_##__structmembername__##_[100]={0}; \
                                                                                                              GetFormatTime_TS(DATETIME_FORMAT_ISO8601, __structname_->__structmembername__,(char*)&__tmp_##__structmembername__##_); \
                                                                                                                 json_object_dotset_string_ex(__object__,__name__, __tmp_##__structmembername__##_, strnlen(__tmp_##__structmembername__##_,sizeof(__tmp_##__structmembername__##_)))    


#define  json_object_set_string_struct_iso8601(__object__,__name__,__structname_, __structmembername__)   char __tmp_##__structmembername__##_[100]={0}; \
                                                                                                              GetFormatTime_TS(DATETIME_FORMAT_ISO8601, __structname_->__structmembername__,(char*)&__tmp_##__structmembername__##_); \
                                                                                                                 json_object_set_string_ex(__object__,__name__, __tmp_##__structmembername__##_, strnlen(__tmp_##__structmembername__##_,sizeof(__tmp_##__structmembername__##_)))    


#endif
