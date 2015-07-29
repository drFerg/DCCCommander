#include <Arduino.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "DCCScheduler.h"
#include "DCCPktQ.h"
#include "DCCHardware.h"
#include "cprint.h"

#define RATE 3 /*Ratio of HIPRI packets to 1 LOPRI packets */
#define HIGHQ_SIZE 20
#define LOWQ_SIZE 20
#define EQ_SIZE 5

DCCPktQ *highQ; /* High priority q for trains and turnouts */
DCCPktQ *lowQ; /* Low priority q for lighting etc */
DCCPktQ *eQ; /* Emergency q for stops */
uint8_t clock; /* Clock for keeping track of HIGH:LOW packet ratio */
DCCPacket idle;

/* nextPacket will balance transmissions between the high and low priority
 * queues based on the predefined RATIO. Emergency packets will always be 
 * sent first, ahead of all other packets.
 * If a low priority queue is due to clock out a packet and none are
 * available, then the high priority queue will get a chance to send in
 * the low priority's slot. This does occur in the opposite case, and isn't
 * important for low priority packets.
 */
int nextPacket(uint8_t **bytes) {
  DCCPacket *pkt = NULL;
  int size = 0;
  if (dccpktq_hasNext(eQ)) { /* Always send all emergency packets first */
    size = dccpktq_next(eQ, &pkt);
  } /* Otherwise send a low priority packet if ratio clock permits */
  else if (clock % RATE == 0 && dccpktq_hasNext(lowQ)) {
    size = dccpktq_next(lowQ, &pkt);
    clock = 0;
  }
  else { /* Otherwise send high priority packets, if ratio clock permits */
    size = dccpktq_next(highQ, &pkt);
    clock++;
  }

  if (size) *bytes = pkt->bytes;
  else { 
    *bytes = idle.bytes;
    size = idle.size;
  }
  return size;
}

int dccshed_init() {
  uint8_t data[1] = {0x00};
  clock = 0;
  cprint_init();
  dccpkt_init(&idle, DCC_ADDR_SHORT, 0xFF, PKT_IDLE, data, 1, 1);
  highQ = dccpktq_create(HIGHQ_SIZE);
  lowQ = dccpktq_create(LOWQ_SIZE);
  eQ = dccpktq_create(EQ_SIZE);
  if (highQ && lowQ && eQ) /* Only init dcc if queues are alloc'd */
    dcc_init(&nextPacket);
  return (highQ && lowQ && eQ);
}

void dccshed_status() {
  printf(">> DCC Scheduler\n\t-HighQ(%d): %s\
                          \n\t-LowQ(%d): %s\
                          \n\t-EQ(%d): %s\n",
          HIGHQ_SIZE, (highQ ? "PASS" : "FAIL"),
          LOWQ_SIZE, (lowQ ? "PASS" : "FAIL"),
          EQ_SIZE, (eQ ? "PASS" : "FAIL"));
}

int dccshed_send(DCCPriority pri, DCCPacket *pkt) {
  switch(pri) {
    case DCC_HIPRI: return dccpktq_insert(highQ, pkt);
    case DCC_LOPRI: return dccpktq_insert(lowQ, pkt);
    case DCC_EPRI:  return dccpktq_insert(eQ, pkt);
    default: return 0;
  }
}
