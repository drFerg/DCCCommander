#include "DCCScheduler.h"
#include "DCCPktQ.h"
#include "DCCHardware.h"
#define RATE 3 /*Ratio of HIPRI packets to 1 LOPRI packets */

DCCPktQ *highQ; /* High priority q for trains and turnouts */
DCCPktQ *lowQ; /* Low priority q for lighting etc */
DCCPktQ *eQ; /* Emergency q for stops */
uint8_t clock;

int nextPacket(uint8_t **bytes) {
  DCCPacket *pkt;
  int size = 0;
  if (dccpktq_hasNext(eQ))
    size = dccpktq_next(eQ, &pkt);
  else if (clock % RATE == 0) {
    size = dccpktq_next(lowQ, &pkt);
    clock = 0;
  }
  else {
    size = dccpktq_next(highQ, &pkt);
    clock++;
  }
  *bytes = pkt->bytes;
  return size;
}

int dccshed_init() {
  clock = 0;
  highQ = dccpktq_create(10);
  lowQ = dccpktq_create(10);
  eQ = dccpktq_create(5);
  dcc_init(&nextPacket);
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