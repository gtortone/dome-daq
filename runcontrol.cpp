#include "runcontrol.h"

#include <stdio.h>

//#define DEBUG_RC

RunControl::RunControl(void) {

   initok = false;
   devh = NULL;
}

void RunControl::init(libusb_device_handle **d) {

   devh = *d;
   initok = true;
}

bool RunControl::read_reg(uint16_t addr, uint16_t &value) {

   uint16_t pkt_wr[2];
   uint8_t *buf_wr;
   uint8_t buf_rd[4];
   uint16_t *pkt_rd;

   uint8_t seq;
   int retval, actual;

   seq = addr % 0xF;

   pkt_wr[0] = (OPC_RD << 12) | seq;
   pkt_wr[1] = addr;

   buf_wr = reinterpret_cast<uint8_t*>(pkt_wr);

#ifdef DEBUG_RC
   printf("buf_wr[0] = %.2X\n", buf_wr[0]);
   printf("buf_wr[1] = %.2X\n", buf_wr[1]);
   printf("buf_wr[2] = %.2X\n", buf_wr[2]);
   printf("buf_wr[3] = %.2X\n", buf_wr[3]);
#endif

   retval = libusb_bulk_transfer(devh, EP_RCWR, buf_wr, 4, &actual, USB_TIMEOUT);

   if (retval == 0 && actual > 0) { // we write successfully

      retval = libusb_bulk_transfer(devh, EP_RCRD, buf_rd, 4, &actual, USB_TIMEOUT);

      if (retval == 0 && actual > 0) { // we read successfully

         if(buf_rd[0] == seq) {

             if(buf_rd[1] == (OPC_RD << 4)) {

                pkt_rd = reinterpret_cast<uint16_t*>(buf_rd);

#ifdef DEBUG_RC
                printf("actual = %d\n", actual);
                printf("pkt_rd[0] = %.4X\n", pkt_rd[0]);
                printf("pkt_rd[1] = %.4X\n", pkt_rd[1]);
#endif
                value = pkt_rd[1];
                return true;

             } else {

                printf("usb_rc_regread: read error - error code received");
                return false;
             }

          } else {
#ifdef DEBUG_RC
             printf("usb_rc_regread: response out of sequence error\n");
#endif
             return false;
          }
      }
   } else {
#ifdef DEBUG_RC
      printf("usb_rc_regread: request write error\n");
#endif
      return false;
   }

#ifdef DEBUG_RC 
   printf("buf_rd[0] = %.2X\n", buf_rd[0]);
   printf("buf_rd[1] = %.2X\n", buf_rd[1]);
   printf("buf_rd[2] = %.2X\n", buf_rd[2]);
   printf("buf_rd[3] = %.2X\n", buf_rd[3]);
#endif

   pkt_rd = reinterpret_cast<uint16_t*>(buf_rd);

#ifdef DEBUG_RC
   printf("pkt_rd[0] = %.4X\n", pkt_rd[0]);
   printf("pkt_rd[1] = %.4X\n", pkt_rd[1]);
#endif

   value = pkt_rd[1];
   return true;
}

bool RunControl::write_reg(uint16_t addr, uint16_t value) {
   
   uint16_t pkt_wr[3];
   uint8_t *buf_wr;
   uint8_t buf_rd[2];

   uint8_t seq;
   int retval, actual;

   seq = addr % 0xF;

   pkt_wr[0] = (OPC_WR << 12) | seq;
   pkt_wr[1] = addr;
   pkt_wr[2] = value;

   buf_wr = reinterpret_cast<uint8_t*>(pkt_wr);

   retval = libusb_bulk_transfer(devh, EP_RCWR, buf_wr, 6, &actual, USB_TIMEOUT);

#ifdef DEBUG_RC
   printf("actual = %d\n", actual);
   printf("buf_wr[0] = %.2X\n", buf_wr[0]);
   printf("buf_wr[1] = %.2X\n", buf_wr[1]);
   printf("buf_wr[2] = %.2X\n", buf_wr[2]);
   printf("buf_wr[3] = %.2X\n", buf_wr[3]);
   printf("buf_wr[4] = %.2X\n", buf_wr[4]);
   printf("buf_wr[5] = %.2X\n", buf_wr[5]);
#endif

   if (retval == 0 && actual > 0) { // we write successfully

      retval = libusb_bulk_transfer(devh, EP_RCRD, buf_rd, 2, &actual, USB_TIMEOUT);

      if (retval == 0 && actual > 0) { // we read successfully

         if(buf_rd[0] == seq) {

            if(buf_rd[1] == (OPC_WR << 4)) {

               return true;

            } else {

#ifdef DEBUG_RC
               printf("usb_rc_regwrite: write error - error code received\n");
#endif
               return false;
            }

         } else {

#ifdef DEBUG_RC
            printf("usb_rc_regwrite: write error - out of sequence\n");
#endif
            return false;
         }

      } else {

#ifdef DEBUG_RC
         printf("usb_rc_regwrite: read (step 2) error code received\n");
#endif
         return false;
      }

   } else {

#ifdef DEBUG_RC
       printf("usb_rc_regwrite: write (step 1) error code received\n");
#endif
       return false;
   }

   return true;
}
