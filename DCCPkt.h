#ifndef DCCPKT_H
#define DCCPKT_H

#define PKT_SIZE 6
#define PKT_MULTIFUNCTION_MASK   0x10
#define PKT_ACCESSORY_MASK     0x40

typedef enum {
    PKT_IDLE            0x10,
    PKT_E_STOP          0x11,
    PKT_SPEED           0x12,
    PKT_FUNCTION_1      0x13,
    PKT_FUNCTION_2      0x14,
    PKT_FUNCTION_3      0x15,
    PKT_ACCESSORY       0x16,
    PKT_RESET           0x17,
    PKT_OPS_MODE        0x18,
    PKT_BASIC_ACCESSORY    0x40,
    PKT_EXTENDED_ACCESSORY 0x41,
    PKT_OTHER           0x00
} DCCPktType;

typedef enum {
    DCC_ADDR_SHORT 0x00,
    DCC_ADDR_LONG 0x01
} DCCAddrType;

typedef struct dccpacket DCCPkt;

void dccpkt_init(DCCAddrType addr_type, uint16_t address, DCCPktType pkt_type,
                 uint8_t data, uint8_t size, uint8_t repeat);

#endif /* DCCPKT_H */