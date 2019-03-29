#include <stdio.h>
#include <stdlib.h>
#include <libusb-1.0/libusb.h>

#include "midas.h"
#include "msystem.h"

#include "experim.h"
#include "runcontrol.h"

/* make frontend functions callable from the C framework */
#ifdef __cplusplus
extern "C" {
#endif

/*-- Globals -------------------------------------------------------*/

//#define DEBUG
//#define DEBUG_MORE
//#define DEBUG_RC

/* USB - begin */ 

#define USB_VENDOR_ID   0x04B4
#define USB_PRODUCT_ID  0x8613
#define EP_DAQRD     	(0x08 | LIBUSB_ENDPOINT_IN)
#define LEN_IN_BUFFER	1024*8

libusb_device_handle *devh = NULL;
uint8_t in_buffer[LEN_IN_BUFFER];

struct libusb_transfer *transfer_daq_in = NULL;
libusb_context *ctx = NULL;

bool rc_transfer = false;
bool cancel_done = false;
bool running = false;
bool paused = false;

RunControl RChandle;

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
INT event_buffer_size = 1600000;	// 1.6 Megabytes

/* ring buffer handler for events */
int rb_handle;

/* tid for usb thread */
pthread_t tid;
pthread_mutex_t mutex;

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
INT read_data(char *pevent, INT off);
INT read_monitor(char *pevent, INT off);
void read_dome_status(void);
void enable_update(INT hDB, INT hkey, void *info);
void power_update(INT hDB, INT hkey, void *info);
void time_update(INT hDB, INT hkey, void *info);
void setup_update(INT hDB, INT hkey, void *info);

void *usb_events_thread(void *);

HNDLE frequency_handle;
HNDLE overcurrent_handle;
HNDLE counters_handle;
HNDLE pll_handle;
HNDLE enable_handle;
HNDLE power_handle;
HNDLE time_handle;
HNDLE setup_handle;

struct {
   DWORD Channel[CHANNEL_NUM];
} frequency_settings;

struct {
   BOOL Channel[CHANNEL_NUM];
} overcurrent_settings;

struct {
   DWORD pps;
   DWORD oop;
} counters_settings;

struct {
   BOOL locked;
   BOOL failed;
} pll_settings;

struct {
   BOOL Channel[CHANNEL_NUM];
} enable_settings;

struct {
   BOOL Channel[CHANNEL_NUM];
} power_settings;

struct {
   DWORD dark;
   DWORD peak;
} time_settings;

struct {
   BOOL calibration;
   BOOL autotrigger;
   BOOL clockout;
} setup_settings;

const char *frequency_config_str[] = {\
    "Channel = DWORD[19] :",\
    "[0] 0",\
    "[1] 0",\
    "[2] 0",\
    "[3] 0",\
    "[4] 0",\
    "[5] 0",\
    "[6] 0",\
    "[7] 0",\
    "[8] 0",\
    "[9] 0",\
    "[10] 0",\
    "[11] 0",\
    "[12] 0",\
    "[13] 0",\
    "[14] 0",\
    "[15] 0",\
    "[16] 0",\
    "[17] 0",\
    "[18] 0",\
    NULL
};

const char *overcurrent_config_str[] = {\
    "Channel = BOOL[19] :",\
    "[0] 0",\
    "[1] 0",\
    "[2] 0",\
    "[3] 0",\
    "[4] 0",\
    "[5] 0",\
    "[6] 0",\
    "[7] 0",\
    "[8] 0",\
    "[9] 0",\
    "[10] 0",\
    "[11] 0",\
    "[12] 0",\
    "[13] 0",\
    "[14] 0",\
    "[15] 0",\
    "[16] 0",\
    "[17] 0",\
    "[18] 0",\
    NULL
};

const char *counters_config_str[] = {\
   "PPS counter = DWORD : 0",\
   "OOP counter = DWORD : 0",\
   NULL
};

const char *pll_config_str[] = {\
   "PLL locked = BOOL : 0",\
   "PLL failed = BOOL : 0",\
   NULL
};

const char *enable_config_str[] = {\
    "Channel = BOOL[19] :",\
    "[0] 0",\
    "[1] 0",\
    "[2] 0",\
    "[3] 0",\
    "[4] 0",\
    "[5] 0",\
    "[6] 0",\
    "[7] 0",\
    "[8] 0",\
    "[9] 0",\
    "[10] 0",\
    "[11] 0",\
    "[12] 0",\
    "[13] 0",\
    "[14] 0",\
    "[15] 0",\
    "[16] 0",\
    "[17] 0",\
    "[18] 0",\
    NULL
};

const char *power_config_str[] = {\
    "Channel = BOOL[19] :",\
    "[0] 0",\
    "[1] 0",\
    "[2] 0",\
    "[3] 0",\
    "[4] 0",\
    "[5] 0",\
    "[6] 0",\
    "[7] 0",\
    "[8] 0",\
    "[9] 0",\
    "[10] 0",\
    "[11] 0",\
    "[12] 0",\
    "[13] 0",\
    "[14] 0",\
    "[15] 0",\
    "[16] 0",\
    "[17] 0",\
    "[18] 0",\
    NULL
};

const char *time_config_str[] = {\
   "Dark delay = DWORD : 0",\
   "Peak delay = DWORD : 0",\
   NULL
};

const char *setup_config_str[] = {\
   "Calibration  = BOOL : 0",\
   "Auto Trigger = BOOL : 0",\
   "Clock Out = BOOL : 0",\
   NULL
};

/*-- Equipment list ------------------------------------------------*/

EQUIPMENT equipment[] = {

   {"Dome%02d_events",           /* equipment name */
    {1, 1,                   /* event ID, trigger mask */
     "SYSTEM",               /* event buffer */
     EQ_POLLED,              /* equipment type */
     0,                      /* event source (not used) */
     "MIDAS",                /* format */
     TRUE,                   /* enabled */
     RO_RUNNING,	     /* readout on running */
     500,                    /* poll every period (ms) */
     0,                      /* stop run after this event limit */
     0,                      /* number of sub events */
     0,                      /* log history */
     "", "", "",},
    read_event,              /* readout routine */
    NULL, NULL, NULL, NULL, },
  {"Dome%02d_data",          /* equipment name */
    {2, 2,                   /* event ID, trigger mask */
     "SYSTEM",               /* event buffer */
     EQ_PERIODIC,            /* equipment type */
     0,                      /* event source (not used) */
     "MIDAS",                /* format */
     TRUE,                   /* enabled */
     RO_ALWAYS | RO_ODB,     /* readout always */
     1000,                   /* poll every period (ms) */
     0,                      /* stop run after this event limit */
     0,                      /* number of sub events */
     1,                      /* log history */
     "", "", ""},
     read_data,              /* readout routine */
     NULL, NULL, NULL, NULL,},
   {""}
};

#ifdef __cplusplus
}
#endif

/* USB callback ----------------------------------------------------*/

void cb_daq_in(struct libusb_transfer *transfer)
{
   void *wp;
   int nb;

   rb_get_buffer_level(rb_handle, &nb);

#ifdef DEBUG   
   printf("callback: status = %d - length: %d / rb: level %d / %.2f%%\n", transfer->status, transfer->actual_length, nb, (float)nb/(float)event_buffer_size*100);
#endif

   if(transfer->status == LIBUSB_TRANSFER_TIMED_OUT) {

      // submit the next transfer
      libusb_submit_transfer(transfer_daq_in);

   } else if(transfer->status == LIBUSB_TRANSFER_CANCELLED) {

      cancel_done = true;
   
   } else if(transfer->status == LIBUSB_TRANSFER_COMPLETED) {

      if(running) {

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
      }	// end if(running)
   }
}

void *usb_events_thread(void *arg)
{
   struct timeval tv;
   tv.tv_sec = 0;
   tv.tv_usec = 100000;

   while(1) {
#ifdef DEBUG
      printf("START loop\n");
#endif
      libusb_handle_events_timeout(ctx, &tv);
#ifdef DEBUG
      printf("END loop\n");
#endif
   }

   return NULL;
}

/*-- Frontend Init -------------------------------------------------*/

INT frontend_init()
{
   INT status, size, state;
   int r;
   char path[64];

   set_equipment_status(equipment[0].name, "Initializing...", "#FFFF00");
   set_equipment_status(equipment[1].name, "Initializing...", "#FFFF00");
 
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

   // try to flush RC read endpoint
   uint8_t data[4];
   int len;
   while( libusb_bulk_transfer(devh, EP_RCRD, data, 4, &len, 250) == 0 )
      ;

   transfer_daq_in = libusb_alloc_transfer(0);
   libusb_fill_bulk_transfer(transfer_daq_in, devh, EP_DAQRD, in_buffer, LEN_IN_BUFFER, cb_daq_in, NULL, USB_TIMEOUT);

   if( pthread_mutex_init(&mutex, NULL) != 0 ) {
      cm_msg(MERROR, "dfe", "mutex initialization error");
      return(FE_ERR_HW);
   }

   // create USB event thread
   pthread_create(&tid, NULL, usb_events_thread, NULL);

   // initialize run control object
   RChandle.init(&devh);

   /* initialize ODB */

   // set EventID with Frontend Index (Dome number)
   equipment[0].info.event_id = feIndex;
   snprintf(path, sizeof(path), "Equipment/%s/Common/Event ID", equipment[0].name);
   db_set_value(hDB, 0, path, &(equipment[0].info.event_id), sizeof(WORD), 1, TID_WORD);

   // read actual run control parameters... 
   read_dome_status();

   // Frequency
   snprintf(path, sizeof(path), "/Equipment/%s/Settings/Frequency/", equipment[1].name);
   status = db_create_record(hDB, 0, path, strcomb(frequency_config_str));
   status = db_find_key(hDB, 0, path, &frequency_handle);
   if (status != DB_SUCCESS) {
      cm_msg(MINFO,"dfe","Key %s not found. Return code: %d", path, status);
   }

   status = db_open_record(hDB, frequency_handle, &frequency_settings, sizeof(frequency_settings), MODE_WRITE, NULL, NULL);
   if (status != DB_SUCCESS){
      printf("Couldn't create hotlink for %s. Return code: %d", path, status);
      cm_msg(MERROR,"dfe","Couldn't create hotlink for %s. Return code: %d", path, status);
      return status;
   }

   // Overcurrent
   snprintf(path, sizeof(path), "/Equipment/%s/Settings/Overcurrent/", equipment[1].name);
   status = db_create_record(hDB, 0, path, strcomb(overcurrent_config_str));
   status = db_find_key(hDB, 0, path, &overcurrent_handle);
   if (status != DB_SUCCESS) {
      cm_msg(MINFO,"dfe","Key %s not found. Return code: %d", path, status);
   }

   status = db_open_record(hDB, overcurrent_handle, &overcurrent_settings, sizeof(overcurrent_settings), MODE_WRITE, NULL, NULL);
   if (status != DB_SUCCESS){
      printf("Couldn't create hotlink for %s. Return code: %d", path, status);
      cm_msg(MERROR,"dfe","Couldn't create hotlink for %s. Return code: %d", path, status);
      return status;
   }

   // Counters
   snprintf(path, sizeof(path), "/Equipment/%s/Settings/Counters/", equipment[1].name);
   status = db_create_record(hDB, 0, path, strcomb(counters_config_str));
   status = db_find_key(hDB, 0, path, &counters_handle);
   if (status != DB_SUCCESS) {
      cm_msg(MINFO,"dfe","Key %s not found. Return code: %d", path, status);
   }

   status = db_open_record(hDB, counters_handle, &counters_settings, sizeof(counters_settings), MODE_WRITE, NULL, NULL);
   if (status != DB_SUCCESS){
      printf("Couldn't create hotlink for %s. Return code: %d", path, status);
      cm_msg(MERROR,"dfe","Couldn't create hotlink for %s. Return code: %d", path, status);
      return status;
   }

   // PLL
   snprintf(path, sizeof(path), "/Equipment/%s/Settings/PLL/", equipment[1].name);
   status = db_create_record(hDB, 0, path, strcomb(pll_config_str));
   status = db_find_key(hDB, 0, path, &pll_handle);
   if (status != DB_SUCCESS) {
      cm_msg(MINFO,"dfe","Key %s not found. Return code: %d", path, status);
   }

   status = db_open_record(hDB, pll_handle, &pll_settings, sizeof(pll_settings), MODE_WRITE, NULL, NULL);
   if (status != DB_SUCCESS){
      printf("Couldn't create hotlink for %s. Return code: %d", path, status);
      cm_msg(MERROR,"dfe","Couldn't create hotlink for %s. Return code: %d", path, status);
      return status;
   }

   // Enable
   snprintf(path, sizeof(path), "/Equipment/%s/Settings/Enable/", equipment[1].name);
   status = db_create_record(hDB, 0, path, strcomb(enable_config_str));
   status = db_find_key(hDB, 0, path, &enable_handle);
   if (status != DB_SUCCESS) {
      cm_msg(MINFO,"dfe","Key %s not found. Return code: %d", path, status);
   }

   status = db_set_record(hDB, enable_handle, &enable_settings, sizeof(enable_settings), 0);
   if (status != DB_SUCCESS){
      printf("Couldn't update ODB for %s. Return code: %d", path, status);
      cm_msg(MERROR,"dfe","Couldn't update ODB for %s. Return code: %d", path, status);
      return status;
   }

   status = db_open_record(hDB, enable_handle, &enable_settings, sizeof(enable_settings), MODE_READ, enable_update, NULL);
   if (status != DB_SUCCESS){
      printf("Couldn't create hotlink for %s. Return code: %d", path, status);
      cm_msg(MERROR,"dfe","Couldn't create hotlink for %s. Return code: %d", path, status);
      return status;
   }

   // Power
   snprintf(path, sizeof(path), "/Equipment/%s/Settings/Power/", equipment[1].name);
   status = db_create_record(hDB, 0, path, strcomb(power_config_str));
   status = db_find_key(hDB, 0, path, &power_handle);
   if (status != DB_SUCCESS) {
      cm_msg(MINFO,"dfe","Key %s not found. Return code: %d", path, status);
   }

   status = db_set_record(hDB, power_handle, &power_settings, sizeof(power_settings), 0);
   if (status != DB_SUCCESS){
      printf("Couldn't update ODB for %s. Return code: %d", path, status);
      cm_msg(MERROR,"dfe","Couldn't update ODB for %s. Return code: %d", path, status);
      return status;
   }

   status = db_open_record(hDB, power_handle, &power_settings, sizeof(power_settings), MODE_READ, power_update, NULL);
   if (status != DB_SUCCESS){
      printf("Couldn't create hotlink for %s. Return code: %d", path, status);
      cm_msg(MERROR,"dfe","Couldn't create hotlink for %s. Return code: %d", path, status);
      return status;
   }

   // Time
   snprintf(path, sizeof(path), "/Equipment/%s/Settings/Time/", equipment[1].name);
   status = db_create_record(hDB, 0, path, strcomb(time_config_str));
   status = db_find_key(hDB, 0, path, &time_handle);
   if (status != DB_SUCCESS) {
      cm_msg(MINFO,"dfe","Key %s not found. Return code: %d", path, status);
   }

   status = db_set_record(hDB, time_handle, &time_settings, sizeof(time_settings), 0);
   if (status != DB_SUCCESS){
      printf("Couldn't update ODB for %s. Return code: %d", path, status);
      cm_msg(MERROR,"dfe","Couldn't update ODB for %s. Return code: %d", path, status);
      return status;
   }
   
   status = db_open_record(hDB, time_handle, &time_settings, sizeof(time_settings), MODE_READ, time_update, NULL);
   if (status != DB_SUCCESS){
      printf("Couldn't create hotlink for %s. Return code: %d", path, status);
      cm_msg(MERROR,"dfe","Couldn't create hotlink for %s. Return code: %d", path, status);
      return status;
   }

   // Setup
   snprintf(path, sizeof(path), "/Equipment/%s/Settings/Setup/", equipment[1].name);
   status = db_create_record(hDB, 0, path, strcomb(setup_config_str));
   status = db_find_key(hDB, 0, path, &setup_handle);
   if (status != DB_SUCCESS) {
      cm_msg(MINFO,"dfe","Key %s not found. Return code: %d", path, status);
   }

   status = db_set_record(hDB, setup_handle, &setup_settings, sizeof(setup_settings), 0);
   if (status != DB_SUCCESS){
      printf("Couldn't update ODB for %s. Return code: %d", path, status);
      cm_msg(MERROR,"dfe","Couldn't update ODB for %s. Return code: %d", path, status);
      return status;
   }

   status = db_open_record(hDB, setup_handle, &setup_settings, sizeof(setup_settings), MODE_READ, setup_update, NULL);
   if (status != DB_SUCCESS){
      printf("Couldn't create hotlink for %s. Return code: %d", path, status);
      cm_msg(MERROR,"dfe","Couldn't create hotlink for %s. Return code: %d", path, status);
      return status;
   }

   set_equipment_status(equipment[0].name, "Initialized", "#00ff00");
   set_equipment_status(equipment[1].name, "Initialized", "#00ff00");

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

   set_equipment_status(equipment[0].name, "Starting run...", "#FFFF00");

   r = rb_create(event_buffer_size, max_event_size, &rb_handle);
   if(r != DB_SUCCESS) {
      cm_msg(MERROR, "dfe", "failed to initialise ring buffer");
      return(FE_ERR_HW);
   }

   libusb_submit_transfer(transfer_daq_in);

   set_equipment_status(equipment[0].name, "Started run", "#00ff00");

   return SUCCESS;
}

/*-- End of Run ----------------------------------------------------*/

INT end_of_run(INT run_number, char *error)
{
   running = false;
   int timer = 0;
   
   set_equipment_status(equipment[0].name, "Ending run...", "#FFFF00");

   cancel_done = false;
   int r = libusb_cancel_transfer(transfer_daq_in);
 
   if(r != LIBUSB_ERROR_NOT_FOUND) {
      while( (!cancel_done) && (timer < 2000) ) {
         usleep(200000);
         timer += 200;	// 200 ms
      }
   }

   rb_delete(rb_handle);

   set_equipment_status(equipment[0].name, "Ended run", "#00ff00");

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

extern "C" INT poll_event(INT source, INT count, BOOL test)
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

extern "C" INT interrupt_configure(INT cmd, INT source, POINTER_T adr)
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

         bk_create(pevent, "CHAN", TID_DWORD, (void **)&pdata);
         *pdata++ = channel;
         bk_close(pevent, pdata);

         bk_create(pevent, "TIME", TID_DWORD, (void **)&pdata);
         *pdata++ = time;
         bk_close(pevent, pdata);

         bk_create(pevent, "TIHR", TID_DWORD, (void **)&pdata);
         *pdata++ = time_hres;
         bk_close(pevent, pdata);

         bk_create(pevent, "WIDT", TID_DWORD, (void **)&pdata);
         *pdata++ = width;
         bk_close(pevent, pdata);

         bk_create(pevent, "WIHR", TID_DWORD, (void **)&pdata);
         *pdata++ = width_hres;
         bk_close(pevent, pdata);

         bk_create(pevent, "ENER", TID_DWORD, (void **)&pdata);
         *pdata++ = energy;
         bk_close(pevent, pdata);

         return bk_size(pevent);
      }
   }	// end loop
}

INT read_data(char *pevent, INT off) {

   bool retval = false;
   uint16_t value;
   int i = 0;

   /* Frequency */
   for(int addr=FREQ_REG_OFFSET; addr<CHANNEL_NUM; addr++) {
      pthread_mutex_lock(&mutex);
         retval = RChandle.read_reg(addr, value);
      pthread_mutex_unlock(&mutex);
      if(retval)
         frequency_settings.Channel[i] = value;
      i++;
   }
   /* Overcurrent */
   pthread_mutex_lock(&mutex);
      RChandle.read_reg(0x0, value);
   pthread_mutex_unlock(&mutex);
   for(i=0; i<16; i++)
      overcurrent_settings.Channel[i] = GETBIT(value, i)==0?true:false;

   pthread_mutex_lock(&mutex);
      RChandle.read_reg(0x1, value);
   pthread_mutex_unlock(&mutex);
   for(i=16; i<CHANNEL_NUM; i++)
      overcurrent_settings.Channel[i] = GETBIT(value, (i-16))==0?true:false;
 
   /* PPS and OOP counters */
   pthread_mutex_lock(&mutex);
      retval = RChandle.read_reg(0x1C, value);
      if(retval)
         counters_settings.pps = value;
      retval = RChandle.read_reg(0x1D, value);
      if(retval)
         counters_settings.oop = value;
   pthread_mutex_unlock(&mutex);

   /* PLL flags */
   pthread_mutex_lock(&mutex);
      retval = RChandle.read_reg(0x8, value);
      if(retval) {
         pll_settings.locked = ((value & 0x4000) > 0);
         pll_settings.failed = ((value & 0x8000) > 0);
      }
   pthread_mutex_unlock(&mutex);

   db_send_changed_records();

   return 0;
}

INT read_monitor(char *pevent, INT off) {

   DWORD *pdata;
   static int i=0;
   static int j=1;
   static int k=2;

   i++; j++; k++;
   printf("%d \n", i);

   /* init bank structure */
   bk_init(pevent);

   bk_create(pevent, "ENV", TID_DWORD, (void **)&pdata);  
   *pdata++ = i;
   *pdata++ = j;
   *pdata++ = k;

   bk_close(pevent, pdata);

   return bk_size(pevent);
}

/*
 * function used to get run control R/W variable only first time
 */
void read_dome_status(void) {

   uint16_t value;

   // Enable
   pthread_mutex_lock(&mutex);
      RChandle.read_reg(0x6, value);
   pthread_mutex_unlock(&mutex);
   for(int i=0; i<16; i++)
      enable_settings.Channel[i] = (GETBIT(value, i)==0?false:true);

   pthread_mutex_lock(&mutex);
      RChandle.read_reg(0x7, value);
   pthread_mutex_unlock(&mutex);
   for(int i=16; i<CHANNEL_NUM; i++)
      enable_settings.Channel[i] = (GETBIT(value, (i-16))==0?false:true);

   // Power
   pthread_mutex_lock(&mutex);
      RChandle.read_reg(0x2, value);
   pthread_mutex_unlock(&mutex);
   for(int i=0; i<16; i++)
      power_settings.Channel[i] = (GETBIT(value, i)==0?true:false);

   pthread_mutex_lock(&mutex);
      RChandle.read_reg(0x3, value);
   pthread_mutex_unlock(&mutex);
   for(int i=16; i<CHANNEL_NUM; i++)
      power_settings.Channel[i] = (GETBIT(value, (i-16))==0?true:false);

   // Time
   pthread_mutex_lock(&mutex);
      RChandle.read_reg(0x4, value);
   pthread_mutex_unlock(&mutex);
   time_settings.dark = value;

   pthread_mutex_lock(&mutex);
      RChandle.read_reg(0x5, value);
   pthread_mutex_unlock(&mutex);
   time_settings.peak = value;

   // Setup
   pthread_mutex_lock(&mutex);
      RChandle.read_reg(0x8, value);
   pthread_mutex_unlock(&mutex);
   setup_settings.calibration = (GETBIT(value, 0)==0?false:true);
   setup_settings.autotrigger = (GETBIT(value, 1)==0?false:true);
   setup_settings.clockout = (GETBIT(value, 13)==0?false:true);
}

void enable_update(INT hDB, INT hkey, void *info) {

   uint16_t value;

   value = 0;
   for(int i=0; i<16; i++) {
      value |= (enable_settings.Channel[i] << i);
   }
   pthread_mutex_lock(&mutex);
      RChandle.write_reg(0x6, value);
   pthread_mutex_unlock(&mutex);

   value = 0;
   for(int i=16; i<CHANNEL_NUM; i++) {
      value |= (enable_settings.Channel[i] << (i-16));
   }
   pthread_mutex_lock(&mutex);
      RChandle.write_reg(0x7, value);
   pthread_mutex_unlock(&mutex);
}

void power_update(INT hDB, INT hkey, void *info) {

   uint16_t value;

   value = 0;
   for(int i=0; i<16; i++) {
      value |= (!power_settings.Channel[i] << i);
   }
   pthread_mutex_lock(&mutex);
      RChandle.write_reg(0x2, value);
   pthread_mutex_unlock(&mutex);

   value = 0;
   for(int i=16; i<CHANNEL_NUM; i++) {
      value |= (!power_settings.Channel[i] << (i-16));
   }
   pthread_mutex_lock(&mutex);
      RChandle.write_reg(0x3, value);
   pthread_mutex_unlock(&mutex);

}

void time_update(INT hDB, INT hkey, void *info) {

   pthread_mutex_lock(&mutex);
      RChandle.write_reg(0x4, time_settings.dark);
   pthread_mutex_unlock(&mutex);

   pthread_mutex_lock(&mutex);
      RChandle.write_reg(0x5, time_settings.peak);
   pthread_mutex_unlock(&mutex);
}

void setup_update(INT hDB, INT hkey, void *info) {
   
   uint16_t value = 0;

   if(setup_settings.calibration)
      SETBIT(value, 0);

   if(setup_settings.autotrigger)
      SETBIT(value, 1);

   if(setup_settings.clockout)
      SETBIT(value, 13);

   pthread_mutex_lock(&mutex);
      RChandle.write_reg(0x8, value);
   pthread_mutex_unlock(&mutex);
}
