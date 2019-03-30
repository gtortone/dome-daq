#ifndef EXPERIM_H_
#define EXPERIM_H_

#define GETBIT(byte,bit) (((byte) & (1UL << (bit))) >> (bit))
#define SETBIT(byte, bit) ((byte) |= (1UL << (bit)))
#define RSTBIT(byte,bit) ((byte) &= ~(1UL << (bit)))

#define TEMPERATURE_FILE	"/sys/bus/iio/devices/iio:device1/in_temp_input"
#define RELHUMIDITY_FILE	"/sys/bus/iio/devices/iio:device2/in_humidityrelative_input"
#define PRESSURE_FILE		"/sys/bus/iio/devices/iio:device1/in_pressure_input"

#define CHANNEL_NUM     19

#define FREQ_REG_OFFSET         0x09    // first frequency register

#endif
