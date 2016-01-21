#ifndef DCCPACKET_H
#define DCCPACKET_H
#include <Arduino.h>

#define PKT_SIZE 6
#define DATA_SIZE 3
#define PKT_MULTIFUNCTION_MASK  0x10
#define PKT_ACCESSORY_MASK      0x40
#define DCC_BROADCAST_ADDR      0x00

#ifdef __cplusplus
extern "C" {
#endif

typedef struct dccpacket {
  uint16_t addr;
  uint8_t addr_type;
  uint8_t data[DATA_SIZE];
  uint8_t crc;
  uint8_t data_size;
  uint8_t type;
  uint8_t repeat;
  uint8_t bytes[PKT_SIZE];
  uint8_t size;
} DCCPacket;

typedef enum {
    PKT_NULL =           0x00,
    PKT_IDLE =           0x10,
    PKT_E_STOP =         0x11,
    PKT_SPEED =          0x12,
    PKT_FUNCTION_1 =     0x13,
    PKT_FUNCTION_2 =     0x14,
    PKT_FUNCTION_3 =     0x15,
    PKT_ACCESSORY =      0x16,
    PKT_RESET =          0x17,
    PKT_OPS_MODE =       0x18,
    PKT_BASIC_ACCESSORY =    0x40,
    PKT_EXTENDED_ACCESSORY = 0x41,
    PKT_OTHER =          0x00
} DCCPktType;

typedef enum {
    DCC_ADDR_SHORT = 0x00,
    DCC_ADDR_LONG = 0x01,
    DCC_ADDR_ACCESS = 0x02
} DCCAddrType;


void dccpkt_init(DCCPacket *pkt, DCCAddrType addr_type, uint16_t addr,
                 DCCPktType pkt_type, uint8_t *data, uint8_t size,
                 uint8_t repeat);


#ifdef __cplusplus
}
#endif
#endif /* DCCPACKET_H */
