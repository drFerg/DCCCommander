#ifndef __DCCHARDWARE_H__
#define __DCCHARDWARE_H__

#ifdef __cplusplus
extern "C"
{
#endif
/* Just call dcc_init to manage the dcc transmissions with a packet providing
 * function and let it do it's magic in the background (using interrupts) */
void dcc_init(int (*getNextPacketfunc)(uint8_t **));
#ifdef __cplusplus
}
#endif

#endif //__DCCHARDWARE_H__