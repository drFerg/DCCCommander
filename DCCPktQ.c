#include "DCCPktQ.h"
#include <stdlib.h>
#include <stdio.h>
#define FREECOUNT 10

typedef struct dccpktelem {
  DCCPacket pkt;
  struct dccpktelem *next;
  struct dccpktelem *prev;
} DCCPktElem;

typedef struct dccpktq {
  DCCPktElem *read; /* read pointer, rotates around linked list */
  DCCPktElem *lastRead; /* remember last packet read, use for repeat removal */
  DCCPktElem *freeList; /* free list to store removed elements */
  int size;
  int count;
  int freeCount;
} DCCPktQ;

/* Linked-List help functions */
void link(DCCPktElem *before, DCCPktElem *e, DCCPktElem *after) {
  before->next = e;
  e->prev = before;
  after->prev = e;
  e->next = after;
}

void unlink(DCCPktElem *e) {
  e->prev->next = e->next;
  e->next->prev = e->prev;
}

void remove(DCCPktQ *q, DCCPktElem *e) {
  if (q->read == e) {
    if (e != e->next) q->read = e->next;
    else q->read = NULL;
  }
  unlink(e);
  q->count--;
  if (q->freeCount < FREECOUNT) {
    e->next = q->freeList;
    e->prev = NULL;
    q->freeList = e;
    q->freeCount++;
  }
  else free(e);
}

int remove_addr_pkt(DCCPktQ *q, uint16_t addr) {
  DCCPktElem *e;
  int n, succ = 0;
  for (n = 0, e = q->read; n < q->count && e->pkt.addr != addr; e = e->next, n++)
    ; /* Loop through queue to find packet matching address */
  if (e->pkt.addr == addr) {
    remove(q, e);
    succ = 1;
  }
  return succ;
}
/**********************/

DCCPktQ *dccpktq_create(int size) {
  DCCPktQ *q = (DCCPktQ *) malloc(sizeof(DCCPktQ));
  if (q == NULL) return NULL;
  q->size = size;
  q->count = q->freeCount = 0;
  q->read = NULL;
  return q;
}

int dccpktq_insert(DCCPktQ *q, DCCPacket *pkt) {
  DCCPktElem *e = NULL;
  cli(); /* Disable interrupts to prevent packet removal from interfering */
  if (q->count == q->size) return 0;
  if (q->freeCount){
    e = q->freeList;
    q->freeList = e->next;
    q->freeCount--;
  } 
  else {
    e = (DCCPktElem *) malloc(sizeof(DCCPktElem));
    if (e == NULL) return 0;
  }
  memcpy(&(e->pkt), pkt, sizeof(DCCPacket));
  q->count++;
  if (q->read == NULL) {
    q->read = e;
    e->next = e->prev = e;
  }
  else {
    link(q->read->prev, e, q->read);
    q->read = e;
  }
  sei();
  return 1;
}

int dccpktq_hasNext(DCCPktQ *q) {
  int exists = 0;
  exists = (q->read != NULL); 
  return exists;
}
  
int dccpktq_next(DCCPktQ *q, DCCPacket **pkt) {
  if (q->read == NULL) {
    return 0;
  }
  *pkt = &(q->read->pkt);
  (*pkt)->repeat--;
  q->lastRead = q->read;
  q->read = q->read->next;
  if (q->lastRead->pkt.repeat == 0) 
    remove(q, q->lastRead); /* Remove last packet read if finished repeating */
  return (*pkt)->size;
}

int dccpktq_remove(DCCPktQ *q, uint16_t addr) {
  int succ = 0;
  cli();
  succ = remove_addr_pkt(q, addr);
  sei();
  return succ;
}

void dccpktq_clear(DCCPktQ *q) {
  DCCPktElem *e;
  cli();
  while((e = q->read) != NULL) remove_addr_pkt(q, e);
  sei();
}

void dccpktq_destroy(DCCPktQ *q) {
  DCCPktElem *e;
  cli();
  if (q == NULL) return;
  while((e = q->read) != NULL) remove_addr_pkt(q, e);
  while((e = q->freeList) != NULL) free(e);
  free(q);
  sei();
}
