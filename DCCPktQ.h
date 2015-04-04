#ifndef DCCPKTQ_H
#define DCCPKTQ_H
#include "DCCPacket.h"

typedef struct dccpktq DCCPktQ;

DCCPktQ *dccpktq_create(int size);
int dccpktq_insert(DCCPktQ *q, DCCPacket *pkt);
int dccpktq_hasNext(DCCPktQ *q);
int dccpktq_next(DCCPktQ *q, DCCPacket **pkt);
int dccpktq_remove(DCCPktQ *q, uint16_t addr);
void dccpktq_clear(DCCPktQ *q);
void dccpktq_destroy(DCCPktQ *q);

#endif /* DCCPKTQ_H */