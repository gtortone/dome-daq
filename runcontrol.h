#ifndef RUNCONTROL_H_
#define RUNCONTROL_H_

#include <libusb-1.0/libusb.h>

#define EP_RCWR         (0x02 | LIBUSB_ENDPOINT_OUT)
#define EP_RCRD         (0x06 | LIBUSB_ENDPOINT_IN)
#define USB_TIMEOUT     500

#define OPC_WR          0x01
#define OPC_RD          0x02
#define OPC_ERR         0xF

class RunControl {

private:

   bool initok;
   libusb_device_handle *devh;

public:

   RunControl(void);

   void init(libusb_device_handle **d);

   bool read_reg(uint16_t addr, uint16_t &value);
   bool write_reg(uint16_t addr, uint16_t value);

};

#endif
