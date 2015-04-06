#include "DCCScheduler.h"
#include "DCCPktQ.h"
#include "DCCHardware.h"
#include <Arduino.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "cprint.h"
#define RATE 3 /*Ratio of HIPRI packets to 1 LOPRI packets */

DCCPktQ *highQ; /* High priority q for trains and turnouts */
DCCPktQ *lowQ; /* Low priority q for lighting etc */
DCCPktQ *eQ; /* Emergency q for stops */
uint8_t clock;
DCCPacket idle;

int nextPacket(uint8_t **bytes) {
  DCCPacket *pkt = NULL;
  int size = 0;
  if (dccpktq_hasNext(eQ))
    size = dccpktq_next(eQ, &pkt);
  else if (clock % RATE == 0 && dccpktq_hasNext(lowQ)) {
    size = dccpktq_next(lowQ, &pkt);
    clock = 0;
  }
  else {
    size = dccpktq_next(highQ, &pkt);
    clock++;
  }
  if (size) *bytes = pkt->bytes;
  else { 
    *bytes = &(idle.bytes);
    size = idle.size;
  }
  return size;
}

int dccshed_init() {
  uint8_t data[1] = {0x00};
  clock = 0;
  highQ = dccpktq_create(10);
  lowQ = dccpktq_create(10);
  eQ = dccpktq_create(5);
  dcc_init(&nextPacket);
  dccpkt_init(&idle, DCC_ADDR_SHORT, 0xFF, PKT_IDLE, data, 1, 1);
  cprint_init();
  printf("HIHHH %d\n", 1);
  return (highQ && lowQ && eQ);
}

int dccshed_send(DCCPriority pri, DCCPacket *pkt) {
  switch(pri) {
    case DCC_HIPRI: return dccpktq_insert(highQ, pkt);
    case DCC_LOPRI: return dccpktq_insert(lowQ, pkt);
    case DCC_EPRI:  return dccpktq_insert(eQ, pkt);
    default: return 0;
  }
}