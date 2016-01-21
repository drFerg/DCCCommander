#include "DCCPacket.h"

int convert_bytes(DCCPacket *pkt, uint8_t *bytes) {
  int i = 0;
  int j;

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

  }
  else if (pkt->type & PKT_ACCESSORY_MASK) {
    if (pkt->type == PKT_BASIC_ACCESSORY) {
      // Basic Accessory Packet looks like this:
      // {preamble} 0 10AAAAAA 0 1AAACDDD 0 EEEEEEEE 1
      // or this:
      // {preamble} 0 10AAAAAA 0 1AAACDDD 0 (1110CCVV 0 VVVVVVVV 0 DDDDDDDD) 0 EEEEEEEE 1 (if programming)

      bytes[i++] = 0x80 /* set up address byte 0 */
                 | (pkt->addr & 0x03F);
      bytes[i++] = 0x88 /* set up address byte 1 */
                 | (~(pkt->addr >> 2) & 0x70)
                 | (pkt->data[0] & 0x07);
      }
  }
  else return 0;

  memcpy(bytes + i, pkt->data, pkt->data_size);
  i += pkt->data_size;

  /* CRC byte calculation */
  bytes[i] = 0;
  for (j = 0; j < i; j++) {
    bytes[i] ^= bytes[j];
  }
  return ++i;
}

void dccpkt_init(DCCPacket *pkt, DCCAddrType addr_type, uint16_t addr,
                 DCCPktType pkt_type, uint8_t *data, uint8_t size,
                 uint8_t repeat) {
  if (pkt == NULL) return;
  pkt->addr_type = addr_type;
  pkt->addr = addr;
  pkt->type = pkt_type;
  memcpy(pkt->data, data, size);
  pkt->data_size = size;
  pkt->repeat = repeat;
  pkt->size = convert_bytes(pkt, pkt->bytes);
}
