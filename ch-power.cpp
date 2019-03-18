#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#include "experim.h"
#include "runcontrol.h"
#include "ch-power.h"

/* Global variables */

extern RunControl RChandle;
extern pthread_mutex_t mutex;

typedef struct {
   bool power[32];
   DWORD last_update;
} CHANNEL_POWER_INFO;

INT ch_power_rall(CHANNEL_POWER_INFO *info);

/*---- support routines --------------------------------------------*/

INT ch_power_rall(CHANNEL_POWER_INFO *info) {
 
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
      RChandle.read_reg(0x2, value);
   pthread_mutex_unlock(&mutex);
   for(int i=0; i<16; i++) 
      info->power[i] = ~(GETBIT(value, i));

   pthread_mutex_lock(&mutex);
      RChandle.read_reg(0x3, value);
   pthread_mutex_unlock(&mutex);
   for(int i=0; i<16; i++)
      info->power[i+16] = ~(GETBIT(value, i));

   return FE_SUCCESS;
}

/*---- device driver routines --------------------------------------*/

INT ch_power_init(HNDLE hkey, void **pinfo) {

   HNDLE hDB;
   CHANNEL_POWER_INFO *info;

   info = (CHANNEL_POWER_INFO *) calloc(1, sizeof(CHANNEL_POWER_INFO));
   *pinfo = info;

   cm_get_experiment_database(&hDB, NULL);
  
   info->last_update = 0;
   ch_power_rall(static_cast<CHANNEL_POWER_INFO*>(info));
   info->last_update = ss_time();

   return FE_SUCCESS; 
}

INT ch_power_get(CHANNEL_POWER_INFO *info, INT channel, float *pvalue) {

   if(channel == 0)
      ch_power_rall(info);

   info->power[channel]?(*pvalue = 1):(*pvalue = 0);

   return FE_SUCCESS;
}

INT ch_power_set(CHANNEL_POWER_INFO *info, INT channel, float value) {

   uint16_t reg;

   if( (value != 0) && (value != 1) )
      return FE_SUCCESS;

   if(channel < 16) {
      pthread_mutex_lock(&mutex);
         RChandle.read_reg(0x2, reg);
      pthread_mutex_unlock(&mutex);
      if(value)
         RSTBIT(reg, channel);
      else
         SETBIT(reg, channel);
      printf("SET 0x2 value:%f ch:%d reg:%X\n", value, channel, reg);
      pthread_mutex_lock(&mutex);
         RChandle.write_reg(0x2, reg);
      pthread_mutex_unlock(&mutex);
   } else {
      pthread_mutex_lock(&mutex);
         RChandle.read_reg(0x3, reg);
      pthread_mutex_unlock(&mutex);
      if(value)
         RSTBIT(reg, channel-16);
      else
         SETBIT(reg, channel-16);
      printf("SET 0x3 value:%f ch:%d reg:%X\n", value, channel, reg);
      pthread_mutex_lock(&mutex);
         RChandle.write_reg(0x3, reg);
      pthread_mutex_unlock(&mutex);
   }

   return FE_SUCCESS; 
}

INT ch_power_get_label(CHANNEL_POWER_INFO *info, INT channel, char *name) {

   sprintf(name, "Power enable CH%02d", channel);

   return FE_SUCCESS;
}

INT ch_power_exit(CHANNEL_POWER_INFO *info) {

   free(info);

   return FE_SUCCESS;
}

/*---- device driver entry point -----------------------------------*/

INT ch_power(INT cmd, ...)
{

   va_list argptr;
   HNDLE   hKey;
   INT     status, channel;
   float   *pvalue;
   double  value;
   void    *info;
   char	   *name;

   va_start(argptr, cmd);
   status = FE_SUCCESS;

   switch (cmd) {

   case CMD_INIT:
      hKey = va_arg(argptr, HNDLE);
      info = va_arg(argptr, void *);
      status = ch_power_init(hKey, static_cast<void **>(info));
      break;

   case CMD_EXIT:
      info = va_arg(argptr, CHANNEL_POWER_INFO *);
      status = ch_power_exit(static_cast<CHANNEL_POWER_INFO*>(info));
      break;

   case CMD_GET_LABEL:
      info = va_arg(argptr, CHANNEL_POWER_INFO *);
      channel = va_arg(argptr, INT);
      name = va_arg(argptr, char *);
      status = ch_power_get_label(static_cast<CHANNEL_POWER_INFO*>(info), channel, name);
      break; 

   case CMD_GET:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      pvalue = va_arg(argptr, float *);
      status = ch_power_get(static_cast<CHANNEL_POWER_INFO*>(info), channel, pvalue);
      break;

   case CMD_SET:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      value = va_arg(argptr, double);
      status = ch_power_set(static_cast<CHANNEL_POWER_INFO*>(info), channel, value);

   default:
      break;
   }

   va_end(argptr);

   return status;
}

/*------------------------------------------------------------------*/

