#ifndef DCC_SHED_H
#define DCC_SHED_H
#include "DCCPacket.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Used to specify which queue to place a packet in for sending */
typedef enum {
  DCC_HIPRI,
  DCC_LOPRI,
  DCC_EPRI
} DCCPriority;

/* Initialises local state including packet queues, idle packet
 * and dcc hardware, ready for sending packets 
 * returns 1 if successful, 0 otherwise 
 */
int dccshed_init();

/* Prints out the status of the scheduler queues */
void dccshed_status();

/* Adds the pkt to the appropriate queue based on the specified priority 
 * returns 1 if successfully added, 0 if queue was full & packet dropped 
 */
int dccshed_send(DCCPriority pri, DCCPacket *pkt);

#ifdef __cplusplus
}
#endif
#endif /* DCC_SHED_H */