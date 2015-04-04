#ifndef DCC_SHED_H
#define DCC_SHED_H
#include "DCCPacket.h"

typedef enum {
  DCC_HIPRI,
  DCC_LOPRI,
  DCC_EPRI
} DCCPriority;

int dccshed_init();
int dccshed_send(DCCPriority pri, DCCPacket *pkt);

#endif /* DCC_SHED_H */