#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#include "experim.h"
#include "runcontrol.h"
#include "ch-enable.h"

/* Macro */

#define GETBIT(byte,bit) (((byte) & (1UL << (bit))) >> (bit))
#define SETBIT(byte, bit) ((byte) |= (1UL << (bit)))
#define RSTBIT(byte,bit) ((byte) &= ~(1UL << (bit)))

/* Global variables */

extern RunControl RChandle;

typedef struct {
   bool enable[32];
   DWORD last_update;
} CHANNEL_ENABLE_INFO;

INT ch_enable_rall(CHANNEL_ENABLE_INFO *info);

/*---- support routines --------------------------------------------*/

INT ch_enable_rall(CHANNEL_ENABLE_INFO *info) {
 
   DWORD nowtime = ss_time();
   DWORD difftime = nowtime - info->last_update;
   uint16_t value;

   // only update once every 5 second
   if (difftime < 5) {
      sleep(5);
      return FE_SUCCESS;
   }
   
   info->last_update = nowtime;

   RChandle.read_reg(0x6, value);
   for(int i=0; i<16; i++) 
      info->enable[i] = GETBIT(value, i);

   RChandle.read_reg(0x7, value);
   for(int i=0; i<16; i++)
      info->enable[i+16] = GETBIT(value, i);

   return FE_SUCCESS;
}

/*---- device driver routines --------------------------------------*/

INT ch_enable_init(HNDLE hkey, void **pinfo) {

   HNDLE hDB;
   CHANNEL_ENABLE_INFO *info;

   info = (CHANNEL_ENABLE_INFO *) calloc(1, sizeof(CHANNEL_ENABLE_INFO));
   *pinfo = info;

   cm_get_experiment_database(&hDB, NULL);
  
   info->last_update = 0;
   ch_enable_rall(static_cast<CHANNEL_ENABLE_INFO*>(info));
   info->last_update = ss_time();

   return FE_SUCCESS; 
}

INT ch_enable_get(CHANNEL_ENABLE_INFO *info, INT channel, float *pvalue) {

   if(channel == 0)
      ch_enable_rall(info);

   info->enable[channel]?(*pvalue = 1):(*pvalue = 0);

   return FE_SUCCESS;
}

INT ch_enable_set(CHANNEL_ENABLE_INFO *info, INT channel, float value) {

   uint16_t reg;

   if( (value != 0) && (value != 1) )
      return FE_ERR_ODB;

   if(channel < 16) {
      RChandle.read_reg(0x6, reg);
      if(value)
         SETBIT(reg, channel);
      else
         RSTBIT(reg, channel);
      printf("SET 0x6 value:%f ch:%d reg:%X\n", value, channel, reg);
      RChandle.write_reg(0x6, reg);
   } else {
      RChandle.read_reg(0x7, reg);
      if(value)
         SETBIT(reg, channel-16);
      else
         RSTBIT(reg, channel-16);
      printf("SET 0x7 value:%f ch:%d reg:%X\n", value, channel, reg);
      RChandle.write_reg(0x7, reg);
   }

   return FE_SUCCESS; 
}

INT ch_enable_get_label(CHANNEL_ENABLE_INFO *info, INT channel, char *name) {

   sprintf(name, "Data enable CH%02d", channel);

   return FE_SUCCESS;
}

INT ch_enable_exit(CHANNEL_ENABLE_INFO *info) {

   free(info);

   return FE_SUCCESS;
}

/*---- device driver entry point -----------------------------------*/

INT ch_enable(INT cmd, ...)
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
      status = ch_enable_init(hKey, static_cast<void **>(info));
      break;

   case CMD_EXIT:
      info = va_arg(argptr, CHANNEL_ENABLE_INFO *);
      status = ch_enable_exit(static_cast<CHANNEL_ENABLE_INFO*>(info));
      break;

   case CMD_GET_LABEL:
      info = va_arg(argptr, CHANNEL_ENABLE_INFO *);
      channel = va_arg(argptr, INT);
      name = va_arg(argptr, char *);
      status = ch_enable_get_label(static_cast<CHANNEL_ENABLE_INFO*>(info), channel, name);
      break; 

   case CMD_GET:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      pvalue = va_arg(argptr, float *);
      status = ch_enable_get(static_cast<CHANNEL_ENABLE_INFO*>(info), channel, pvalue);
      break;

   case CMD_SET:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      value = va_arg(argptr, double);
      status = ch_enable_set(static_cast<CHANNEL_ENABLE_INFO*>(info), channel, value);

   default:
      break;
   }

   va_end(argptr);

   return status;
}

/*------------------------------------------------------------------*/

