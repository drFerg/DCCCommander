#include "DCCPkt.h"

typedef struct dccpacket {
    uint16_t addr;
    uint8_t data[DATA_SIZE];
    uint8_t crc;
    uint8_t size;
    uint8_t type;
    uint8_t repeat;
    uint8_t bytes[PKT_SIZE];
} DCCPkt;

void dccpkt_init(DCCPkt *pkt, DCCAddrType addr_type, uint16_t address, 
                 DCCPktType pkt_type, uint8_t data, uint8_t size, uint8_t repeat) {
    pkt->addr_type = addr_type;
    pkt->addr = addr;
    pkt->type = pkt_type;
    memcpy(pkt->data, data, size);
    pkt->size = size;
    pkt->repeat = repeat;
    convert_bytes(pkt, pkt->bytes);
}

void convert_bytes(DCCPkt *pkt, uint8_t *bytes) {
    int i = 0; //minimum size

    if (pkt->type & PKT_MULTIFUNCTION_MASK) {
        if (pkt->type == PKT_IDLE) {
            bytes[i++] = 0xFF;
        } 
        else if (pkt->addr_type == DCC_ADDR_SHORT) { /* 7 bit address */
            bytes[i++] = (uint8_t)(pkt->addr & 0x7F);
        }
        else { /* 14 bit address */
            bytes[i++] = (uint8_t) ((pkt->addr >> 8) | 0xC0);
            bytes[i++] = (uint8_t) (pkt->addr & 0xFF);
        } 
        
    } else if (pkt->type & PKT_ACCESSORY_MASK) {
        if (pkt->type == PKT_BASIC_ACCESSORY) {
            // Basic Accessory Packet looks like this:
            // {preamble} 0 10AAAAAA 0 1AAACDDD 0 EEEEEEEE 1
            // or this:
            // {preamble} 0 10AAAAAA 0 1AAACDDD 0 (1110CCVV 0 VVVVVVVV 0 DDDDDDDD) 0 EEEEEEEE 1 (if programming)

            bytes[i++] = 0x80 /* set up address byte 0 */
                       | (address & 0x03F); 
            bytes[i++] = 0x88 /* set up address byte 1 */
                       | (~(address >> 2) & 0x70)
                       | (data[0] & 0x07); 
        }
    }
    else return 0;

    memcpy(bytes + i, pkt->data, pkt->size);
    i += pkt->size;
    pkt->crc = 0;
    for (j = 0; j < i; ++j) {
        pkt->crc ^= bytes[j];
    }
    bytes[i++] = pkt->crc;
    return i;

}

uint8_t *dccpkt_get_bytes(DCCPkt *pkt) {
    return pkt->bytes;
}