#include "DCCPktQ.h"
#define FREECOUNT 10

typedef struct dccpktelem {
  DCCPacket pkt;
  struct dccpktelem *next;
  struct dccpktelem *prev;
} DCCPktElem;

typedef struct dccpktq {
  DCCPktElem *read; /* read sentinel, rotates around linkedlist */
  DCCPktElem *lastRead; /* remember last packet read, use for repeat removal */
  DCCPktElem *freeList; /* free list to store removed elems, saves some mallocing */
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
  cli();
  if (q->read == e && e != e->next) q->read = e->next;
  else q->read = NULL;
  unlink(e);
  q->count--;
  if (q->freeCount < FREECOUNT) {
    e->next = q->freeList;
    e->prev = NULL;
    q->freeList = e;
    q->freeCount++;
  }
  else free(e);
  sei();
}
/**********************/

DCCPktQ *dccpktq_create(int size) {
  DCCPktQ *q = (DCCPktQ *) malloc(sizeof(DCCPktQ));
  if (q != NULL){
    q->size = size;
    q->count = q->freeCount = 0;
    q->read = NULL;
  }
  return NULL;
}

int dccpktq_insert(DCCPktQ *q, DCCPacket *pkt) {
  DCCPktElem *e = NULL;
  cli();
  if (q->count == q->size) return 0;
  if (q->freeCount){
    e = q->freeList;
    q->freeList = e->next;
    q->freeCount--;
  } 
  else {
    e = (DCCPktElem *) malloc(sizeof(DCCPktElem));
  }
  memcpy(&(e->pkt), pkt, sizeof(DCCPacket));
  q->count++;
  if (q->read == NULL) {
    q->read = e;
    e->next = e->prev = NULL;
  }
  else {
    link(q->read->prev, e, q->read);
    q->read = e;
  }
  sei();
  return 1;
}

int dccpktq_hasNext(DCCPktQ *q) {
  return q->read != NULL;
}
  
int dccpktq_next(DCCPktQ *q, DCCPacket **pkt) {
  cli();
  *pkt = &(q->read->pkt);
  (*pkt)->repeat--;
  q->lastRead = q->read;
  q->read = q->read->next;
  if (q->lastRead->pkt.repeat == 0) remove(q, q->lastRead); 
  sei();
  return (*pkt)->size;
}

int dccpktq_remove(DCCPktQ *q, uint16_t addr) {
  DCCPktElem *e;
  int n;
  for (n = 0, e = q->read; n < q->count && e->pkt.addr != addr; e = e->next, n++)
    ;
  if (e->pkt.addr == addr) {
    remove(q, e);
    return 1;
  }
  return 0;
}

void dccpktq_clear(DCCPktQ *q) {
  DCCPktElem *e;
  while(e = q->read) remove(q, e);
}

void dccpktq_destroy(DCCPktQ *q) {
  DCCPktElem *e;
  if (q == NULL) return;
  while(e = q->read) remove(q, e);
  while(e = q->freeList) free(e);
  free(q);
}