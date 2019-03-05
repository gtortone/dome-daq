#include <stdio.h>
#include <stdlib.h>
#include <libusb-1.0/libusb.h>

#include "midas.h"
#include "msystem.h"

/* make frontend functions callable from the C framework */
#ifdef __cplusplus
extern "C" {
#endif

/*-- Globals -------------------------------------------------------*/

//#define DEBUG
//#define DEBUG_MORE

/* USB - begin */ 

#define USB_VENDOR_ID   0x04B4
#define USB_PRODUCT_ID  0x8613
#define EP_DAQRD     	(LIBUSB_ENDPOINT_IN  | 8)
#define USB_TIMEOUT     3000
#define LEN_IN_BUFFER	1024*8

struct libusb_device_handle *devh = NULL;
uint8_t in_buffer[LEN_IN_BUFFER];

struct libusb_transfer *transfer_daq_in = NULL;
libusb_context *ctx = NULL;

bool cancel_done = false;
bool running = false;
bool paused = false;

/* USB - end */

//Global variables:

#define FE_NAME		"dfe"

extern HNDLE hDB; 

/* The frontend name (client name) as seen by other MIDAS clients   */
const char *frontend_name = (char*)FE_NAME;
/* The frontend file name, don't change it */
const char *frontend_file_name = __FILE__;

/* frontend_loop is called periodically if this variable is TRUE    */
BOOL frontend_call_loop = FALSE;

/* a frontend status page is displayed with this frequency in ms */
INT display_period = 0;

/* maximum event size for fragmented events (EQ_FRAGMENTED) */
INT max_event_size_frag = 5 * 1024 * 1024;

/* maximum event size produced by this frontend */
INT max_event_size = 16000;

/* ring buffer size to hold events */
INT event_buffer_size = 16000000;	// 16 Megabytes

/* ring buffer handler for events */
int rb_handle;

/* tid for usb thread */
pthread_t tid;

#define SOE	0xBAAB
#define EOE	0xFEEF

/*-- Function declarations -----------------------------------------*/

INT frontend_init();
INT frontend_exit();
INT begin_of_run(INT run_number, char *error);
INT end_of_run(INT run_number, char *error);
INT pause_run(INT run_number, char *error);
INT resume_run(INT run_number, char *error);
INT frontend_loop();

INT interrupt_configure(INT cmd, INT source, POINTER_T adr);
INT poll_event(INT source, INT count, BOOL test);
INT read_event(char *pevent, INT off);

void *usb_events_thread(void *);

/*-- Equipment list ------------------------------------------------*/

EQUIPMENT equipment[] = {

   {"Dome%04d-ev",                   /* equipment name */
    {1, 0,                   /* event ID, trigger mask */
     "SYSTEM",               /* event buffer */
     EQ_POLLED,              /* equipment type */
     0,                      /* event source (not used) */
     "MIDAS",                /* format */
     TRUE,                   /* enabled */
     RO_RUNNING,	     /* readout on running */
     500,                    /* poll for 500ms */
     0,                      /* stop run after this event limit */
     0,                      /* number of sub events */
     0,                      /* log history */
     "", "", "",},
    read_event,          /* readout routine */
    },

   {""}
};

#ifdef __cplusplus
}
#endif

/* USB callback ----------------------------------------------------*/

void cb_in(struct libusb_transfer *transfer)
{
   void *wp;
   int nb;

   rb_get_buffer_level(rb_handle, &nb);

#ifdef DEBUG   
   printf("callback: status = %d - length: %d / rb: level %d / %.2f%%\n", transfer->status, transfer->actual_length, nb, (float)nb/(float)event_buffer_size*100);
#endif

   if(transfer->status == LIBUSB_TRANSFER_CANCELLED) {

      cancel_done = true;
   
   } else if(transfer->status == LIBUSB_TRANSFER_COMPLETED) {

      int status = rb_get_wp(rb_handle, &wp, 100);

      if (status == DB_TIMEOUT) {

         printf("wp timeout. ring buffer full?\n");

      } else {

         int tlen = transfer->actual_length;

         memcpy(wp, transfer->buffer, tlen);
         rb_increment_wp(rb_handle, tlen);

#ifdef DEBUG_MORE
         unsigned short int *bufusint;
         int buflen;

         bufusint = reinterpret_cast<unsigned short int*>(transfer->buffer);
         buflen = ((tlen/2 * 2) == tlen)?tlen/2:tlen/2 + 1;      

         for(int i=0; i<buflen; i++)
            printf("%X ", bufusint[i]);
#endif

         // submit the next transfer
         libusb_submit_transfer(transfer_daq_in);
      }
   }
}

void *usb_events_thread(void *arg) 
{
   static struct timeval tv;
   tv.tv_sec = 0;
   tv.tv_usec = 0;
   //tv.tv_usec = 200 * 1000;     // 200 msec

   while(running)
      libusb_handle_events_timeout_completed(ctx, &tv, NULL);

   return NULL;
}

/*-- Frontend Init -------------------------------------------------*/

INT frontend_init()
{
   INT status, size, state;
   int r;
   char sEpath[64];

   size = sizeof(state);
   status = db_get_value(hDB, 0, "Runinfo/State", &state, &size, TID_INT, TRUE);

   if (status != DB_SUCCESS) {
      cm_msg(MERROR, "dfe", "cannot get Runinfo/State in database");
      return FE_ERR_HW;
   }

   if(state == STATE_RUNNING) {
      cm_msg(MERROR, "dfe", "Error: DAQ in running state, please stop and launch again frontend\n");
      return FE_ERR_HW;
   }

   int feIndex = get_frontend_index();
   if(feIndex < 0){
      cm_msg(MERROR,"Init", "Must specify the frontend index (ie use -i X command line option)");
      return FE_ERR_HW;
   }

   // set EventID with Frontend Index (Dome number)
   equipment[0].info.event_id = feIndex;
   snprintf(sEpath, sizeof(sEpath), "Equipment/%s/Common/Event ID", equipment[0].name);
   db_set_value(hDB, 0, sEpath, &(equipment[0].info.event_id), sizeof(WORD), 1, TID_WORD);

   r = libusb_init(NULL);
   if(r < 0) {
      cm_msg(MERROR, "dfe", "failed to initialise libusb");
      return(FE_ERR_HW);
   }

   devh = libusb_open_device_with_vid_pid(ctx, USB_VENDOR_ID, USB_PRODUCT_ID);
   if (!devh) {
      cm_msg(MERROR, "dfe", "usb device not found");
      return(FE_ERR_HW);
   }

   r = libusb_claim_interface(devh, 0);
   if(r < 0) {
      cm_msg(MERROR, "dfe", "usb claim interface error");
      return(FE_ERR_HW);
   }

   r = libusb_set_interface_alt_setting(devh, 0, 1);
   if(r != 0) {
      cm_msg(MERROR, "dfe", "usb setting interface alternate settings error");
      return(FE_ERR_HW);
   } 

   transfer_daq_in  = libusb_alloc_transfer(0);
   libusb_fill_bulk_transfer(transfer_daq_in, devh, EP_DAQRD, in_buffer, LEN_IN_BUFFER, cb_in, NULL, USB_TIMEOUT);

   cm_msg(MINFO,"dfe","Dome FE initialized");

   return SUCCESS;
}

/*-- Frontend Exit -------------------------------------------------*/

INT frontend_exit()
{
   libusb_close(devh);
   libusb_exit(NULL);

   return SUCCESS;
}

/*-- Begin of Run --------------------------------------------------*/

INT begin_of_run(INT run_number, char *error)
{
   int r;

   running = true;

   libusb_submit_transfer(transfer_daq_in);
   pthread_create(&tid, NULL, usb_events_thread, NULL);

   r = rb_create(event_buffer_size, max_event_size, &rb_handle);
   if(r != DB_SUCCESS) {
      cm_msg(MERROR, "dfe", "failed to initialise ring buffer");
      return(FE_ERR_HW);
   }

   return SUCCESS;
}

/*-- End of Run ----------------------------------------------------*/

INT end_of_run(INT run_number, char *error)
{
   static struct timeval tv;
   tv.tv_usec = 0;
   tv.tv_sec = 0;
   //tv.tv_usec = 200 * 1000;	// 200 msec

   running = false;
   
   pthread_join(tid, NULL);

   libusb_cancel_transfer(transfer_daq_in);
 
   while(libusb_handle_events_timeout_completed(ctx, &tv, NULL)) ;

   cancel_done = false;

   rb_delete(rb_handle);

   return SUCCESS;
}

/*-- Pause Run -----------------------------------------------------*/

INT pause_run(INT run_number, char *error)
{
   paused = true;
   return SUCCESS;
}

/*-- Resume Run ----------------------------------------------------*/

INT resume_run(INT run_number, char *error)
{
   paused = false;
   return SUCCESS;
}

/*-- Frontend Loop -------------------------------------------------*/

INT frontend_loop()
{
   /* if frontend_call_loop is true, this routine gets called when
      the frontend is idle or once between every event */
   return SUCCESS;
}

/*------------------------------------------------------------------*/

/*-- Trigger event routines ----------------------------------------*/

INT poll_event(INT source, INT count, BOOL test)
{
   if(running) {

      unsigned short int *rp=NULL;
      int status = rb_get_rp(rb_handle, (void**)&rp, 500);

      if(status == DB_TIMEOUT)
         return 0;

      while( (*rp != SOE) && (status != DB_TIMEOUT) ) {
         rb_increment_rp(rb_handle, 2);
         status = rb_get_rp(rb_handle, (void**)&rp, 500);
      }
 
      return (*rp == SOE);
   }

   return 0;
}

/*-- Interrupt configuration ---------------------------------------*/

INT interrupt_configure(INT cmd, INT source, POINTER_T adr)
{
   return SUCCESS;
}

/*-- Event readout -------------------------------------------------*/

INT read_event(char *pevent, INT off) {

   DWORD *pdata;
   WORD channel=0;
   DWORD time=0, time_hres=0, width=0, width_hres=0, energy=0;
   WORD rd_crc=0, calc_crc=0;
   WORD *rp = NULL;

   int status = 0;
   WORD cur_state = 0;
   WORD nwords = 0;

   while(1) {

      status = rb_get_rp(rb_handle, (void**)&rp, 100);

      if(status == DB_TIMEOUT) {
         printf("event parsing error: ring buffer read timeout\n");
         return 0;
      }

      if(nwords > 8) {
         printf("event parsing error: event too long\n");
         return 0;
      }

      if( (*rp) == 0x8000 ) { 	// skip padding word
         rb_increment_rp(rb_handle, 2);
         continue;
      }

      if( (cur_state > 0) && (cur_state < 7) && ( (*rp) & 0x8000 ) ) {  // MSB==1 inside event body
         printf("event parsing error: word with MSB=1 inside event body (%X)\n", (*rp));
         return 0;
      }

      if(cur_state == 0) {
         if( (*rp) == SOE ) {
            calc_crc = (*rp);  
            cur_state = 1;
            rb_increment_rp(rb_handle, 2);
            nwords = 1;
            continue;
         } else {
            printf("event parsing error: first word not equal to SOE (%X)\n", (*rp));
            return 0;
         }
      }

      if(cur_state == 1) {
         channel = *rp;
         calc_crc ^= *rp;
         rb_increment_rp(rb_handle, 2);
         cur_state = 2;
         nwords++;
         continue;
      }

      if(cur_state == 2) {
         time = (*rp) << 13;
         calc_crc ^= *rp;
         rb_increment_rp(rb_handle, 2);
         cur_state = 3;
         nwords++;
         continue;
      }

      if(cur_state == 3) {
         time = time + ((*rp) >> 2);
         time_hres = ((*rp) & 0x3) << 3;
         calc_crc ^= *rp;
         rb_increment_rp(rb_handle, 2);
         cur_state = 4;
         nwords++;
         continue;
      }

      if(cur_state == 4) {
         time_hres = time_hres + ((*rp) >> 11);
         width = ((*rp) & (0x07E0)) >> 5;
         width_hres = ((*rp) & (0x001F));
         calc_crc ^= *rp;
         rb_increment_rp(rb_handle, 2);
         cur_state = 5;
         nwords++;
         continue;
      }   

      if(cur_state == 5) {
         energy = *rp;
         calc_crc ^= *rp;
         rb_increment_rp(rb_handle, 2);
         cur_state = 6;
         nwords++;
         continue;
      }

      if(cur_state == 6) {
         rd_crc = *rp;
         rb_increment_rp(rb_handle, 2);
         cur_state = 7;
         nwords++;
         continue;
      } 

      if(cur_state == 7) {
         if( (*rp) != EOE) {
            printf("event parsing error: last word not equal to EOE (%X)\n", (*rp));
            return 0;
         } else {
            calc_crc ^= *rp;
            nwords++;
         }
         
         if(calc_crc != rd_crc) {
            printf("event CRC error, rd: %X calc: %X\n", rd_crc, calc_crc);
            return 0;
         }
         
         /* init bank structure */
         bk_init(pevent);

         bk_create(pevent, "DATA", TID_DWORD, (void **)&pdata);
         *pdata++ = channel;
         *pdata++ = time;
         *pdata++ = time_hres;
         *pdata++ = width;
         *pdata++ = width_hres;
         *pdata++ = energy;
         bk_close(pevent, pdata);

         return bk_size(pevent);
      }
   }	// end loop
}
