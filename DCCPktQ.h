#ifndef DCCPKTQ_H
#define DCCPKTQ_H
#include "DCCPacket.h"

typedef struct dccpktq DCCPktQ;

/* Creates a queue of size packet elements */
DCCPktQ *dccpktq_create(int size);

/* Inserts a packet into the queue if space permits 
 * 
 * Packets are inserted in LIFO order (before the next packet to be read),
 * thus are first to be sent in the queue to prioritise new packets 
 * 
 * Interrupt safe
 */
int dccpktq_insert(DCCPktQ *q, DCCPacket *pkt);

/* Removes a packet matching destination addr
 * 
 * Interrupt safe
 */
int dccpktq_remove(DCCPktQ *q, uint16_t addr);

/* Removes all packets from the queue 
 * 
 * Interrupt safe 
 */
void dccpktq_clear(DCCPktQ *q);

/* Destroys queue freeing used resources
 *
 * Interrupt safe
 */
void dccpktq_destroy(DCCPktQ *q);

/* Checks if packet is available in the queue 
 *
 * Should only be run within the ISR routine for DCC hardware/scheduler,
 * other functions shouldn't need to query packets from the queue */
int dccpktq_hasNext(DCCPktQ *q);

/* Passes the next packet to be read back in the pointer provided, 
 * returns the size of the packet to be read 
 * 
 * Should only be run within the ISR routine for DCC hardware/scheduler,
 * other functions shouldn't need to read packets from the queue */
int dccpktq_next(DCCPktQ *q, DCCPacket **pkt);

#endif /* DCCPKTQ_H */