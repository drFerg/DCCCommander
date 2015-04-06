#ifndef DCC_SHED_H
#define DCC_SHED_H
#include "DCCPacket.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef enum {
  DCC_HIPRI,
  DCC_LOPRI,
  DCC_EPRI
} DCCPriority;

int dccshed_init();
int dccshed_send(DCCPriority pri, DCCPacket *pkt);

#ifdef __cplusplus
}
#endif
#endif /* DCC_SHED_H */