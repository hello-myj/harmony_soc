#ifndef __APP_HANDLER_H__
#define __APP_HANDLER_H__

#define APP_PARAM_SSID_LEN (32+1)
#define APP_PARAM_PASSWD_LEN (64+1)

typedef struct 
{
   char ssid[APP_PARAM_SSID_LEN];
   char passwd[APP_PARAM_PASSWD_LEN];
   
}app_wifi_param_t;


int app_proc_start();


#endif