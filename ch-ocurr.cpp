#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#include "experim.h"
#include "runcontrol.h"
#include "ch-ocurr.h"

/* Global variables */

extern RunControl RChandle;
extern pthread_mutex_t mutex;

typedef struct {
   bool ocurr[32];
   DWORD last_update;
} CHANNEL_OVERCURR_INFO;

INT ch_ocurr_rall(CHANNEL_OVERCURR_INFO *info);

/*---- support routines --------------------------------------------*/

INT ch_ocurr_rall(CHANNEL_OVERCURR_INFO *info) {
 
   DWORD nowtime = ss_time();
   DWORD difftime = nowtime - info->last_update;
   uint16_t value;

   // only update once every 5 second
   if (difftime < 5) {
      sleep(5);
      return FE_SUCCESS;
   }
   
   info->last_update = nowtime;

   pthread_mutex_lock(&mutex);
      RChandle.read_reg(0x0, value);
   pthread_mutex_unlock(&mutex);
   for(int i=0; i<16; i++) 
      info->ocurr[i] = GETBIT(value, i);

   pthread_mutex_lock(&mutex);
      RChandle.read_reg(0x1, value);
   pthread_mutex_unlock(&mutex);
   for(int i=0; i<16; i++)
      info->ocurr[i+16] = GETBIT(value, i);

   return FE_SUCCESS;
}

/*---- device driver routines --------------------------------------*/

INT ch_ocurr_init(HNDLE hkey, void **pinfo) {

   HNDLE hDB;
   CHANNEL_OVERCURR_INFO *info;

   info = (CHANNEL_OVERCURR_INFO *) calloc(1, sizeof(CHANNEL_OVERCURR_INFO));
   *pinfo = info;

   cm_get_experiment_database(&hDB, NULL);
  
   info->last_update = 0;
   ch_ocurr_rall(static_cast<CHANNEL_OVERCURR_INFO*>(info));
   info->last_update = ss_time();

   return FE_SUCCESS; 
}

INT ch_ocurr_get(CHANNEL_OVERCURR_INFO *info, INT channel, float *pvalue) {

   if(channel == 0)
      ch_ocurr_rall(info);

   info->ocurr[channel]?(*pvalue = 1):(*pvalue = 0);

   return FE_SUCCESS;
}

INT ch_ocurr_get_label(CHANNEL_OVERCURR_INFO *info, INT channel, char *name) {

   sprintf(name, "Overcurrent CH%02d", channel);

   return FE_SUCCESS;
}

INT ch_ocurr_exit(CHANNEL_OVERCURR_INFO *info) {

   free(info);

   return FE_SUCCESS;
}

/*---- device driver entry point -----------------------------------*/

INT ch_ocurr(INT cmd, ...)
{

   va_list argptr;
   HNDLE   hKey;
   INT     status, channel;
   float   *pvalue;
   void    *info;
   char	   *name;

   va_start(argptr, cmd);
   status = FE_SUCCESS;

   switch (cmd) {

   case CMD_INIT:
      hKey = va_arg(argptr, HNDLE);
      info = va_arg(argptr, void *);
      status = ch_ocurr_init(hKey, static_cast<void **>(info));
      break;

   case CMD_EXIT:
      info = va_arg(argptr, CHANNEL_OVERCURR_INFO *);
      status = ch_ocurr_exit(static_cast<CHANNEL_OVERCURR_INFO*>(info));
      break;

   case CMD_GET_LABEL:
      info = va_arg(argptr, CHANNEL_OVERCURR_INFO *);
      channel = va_arg(argptr, INT);
      name = va_arg(argptr, char *);
      status = ch_ocurr_get_label(static_cast<CHANNEL_OVERCURR_INFO*>(info), channel, name);
      break; 

   case CMD_GET:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      pvalue = va_arg(argptr, float *);
      status = ch_ocurr_get(static_cast<CHANNEL_OVERCURR_INFO*>(info), channel, pvalue);
      break;

   default:
      break;
   }

   va_end(argptr);

   return status;
}

/*------------------------------------------------------------------*/

