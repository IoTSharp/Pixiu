#pragma once

#include  <bits_iot_rs485_.h>
#define FALSE  -1
#define TRUE   0
#define true  -1
#define false   0
#define  CFG_JSON  "/etc/pixiu.cfg"

#define echo_app(fmt, args...)	  printf("app %s[%s:%d]: " fmt "\n", __func__,__FILE__, __LINE__ , ## args) 

void PrintHEX(char * titel, const unsigned char * data, int datlen);
void UploadEvnData(BITS_IOT* _biotdata);
void SyncDateTime(void);
void CheckDoorStatus(void);
int App_Init(void);

char * GetRS485(void);