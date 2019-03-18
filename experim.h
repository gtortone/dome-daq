#ifndef EXPERIM_H_
#define EXPERIM_H_

#define GETBIT(byte,bit) (((byte) & (1UL << (bit))) >> (bit))
#define SETBIT(byte, bit) ((byte) |= (1UL << (bit)))
#define RSTBIT(byte,bit) ((byte) &= ~(1UL << (bit)))

#define CHANNEL_NUM     19

#endif
