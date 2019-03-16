#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#include "experim.h"
#include "runcontrol.h"
#include "ch-freq.h"

/* Global variables */

extern RunControl RChandle;

#define FREQ_REG_OFFSET		0x09	// first frequency register

typedef struct {
   uint16_t freq[CHANNEL_NUM];
   DWORD last_update;
} CHANNEL_FREQ_INFO;

INT ch_freq_rall(CHANNEL_FREQ_INFO *info);

/*---- support routines --------------------------------------------*/

INT ch_freq_rall(CHANNEL_FREQ_INFO *info) {
 
   DWORD nowtime = ss_time();
   DWORD difftime = nowtime - info->last_update;
   uint16_t value;

   // only update once every 1 second
   if (difftime < 1) {
      sleep(1);
      return FE_SUCCESS;
   }
   
   //printf("ch_freq_all difftime: %d\n", difftime);
   info->last_update = nowtime;

   int i=0;
   bool retval = false;
   for(int addr=FREQ_REG_OFFSET; addr<CHANNEL_NUM; addr++) {
      retval = RChandle.read_reg(addr, value);
      if(retval)
         info->freq[i] = value;
      i++;
   }

   return FE_SUCCESS;
}

/*---- device driver routines --------------------------------------*/

INT ch_freq_init(HNDLE hkey, void **pinfo) {

   HNDLE hDB;
   CHANNEL_FREQ_INFO *info;

   info = (CHANNEL_FREQ_INFO *) calloc(1, sizeof(CHANNEL_FREQ_INFO));
   *pinfo = info;

   cm_get_experiment_database(&hDB, NULL);
  
   info->last_update = 0;
   ch_freq_rall(static_cast<CHANNEL_FREQ_INFO*>(info));
   info->last_update = ss_time();

   return FE_SUCCESS; 
}

INT ch_freq_get(CHANNEL_FREQ_INFO *info, INT channel, float *pvalue) {

   if(channel == 0)
      ch_freq_rall(info);

   *pvalue = info->freq[channel];

   return FE_SUCCESS;
}

INT ch_freq_get_label(CHANNEL_FREQ_INFO *info, INT channel, char *name) {

   sprintf(name, "Frequency CH%02d", channel);

   return FE_SUCCESS;
}

INT ch_freq_exit(CHANNEL_FREQ_INFO *info) {

   free(info);

   return FE_SUCCESS;
}

/*---- device driver entry point -----------------------------------*/

INT ch_freq(INT cmd, ...)
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
      status = ch_freq_init(hKey, static_cast<void **>(info));
      break;

   case CMD_EXIT:
      info = va_arg(argptr, CHANNEL_FREQ_INFO *);
      status = ch_freq_exit(static_cast<CHANNEL_FREQ_INFO*>(info));
      break;

   case CMD_GET_LABEL:
      info = va_arg(argptr, CHANNEL_FREQ_INFO *);
      channel = va_arg(argptr, INT);
      name = va_arg(argptr, char *);
      status = ch_freq_get_label(static_cast<CHANNEL_FREQ_INFO*>(info), channel, name);
      break; 

   case CMD_GET:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      pvalue = va_arg(argptr, float *);
      status = ch_freq_get(static_cast<CHANNEL_FREQ_INFO*>(info), channel, pvalue);
      break;

   default:
      break;
   }

   va_end(argptr);

   return status;
}

/*------------------------------------------------------------------*/

