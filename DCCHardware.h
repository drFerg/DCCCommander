#ifndef __DCCHARDWARE_H__
#define __DCCHARDWARE_H__

#ifdef __cplusplus
extern "C"
{
#endif
void dcc_init(int (*grabNextPacketfunc)(uint8_t **));
int dcc_bytes_left();
void dcc_send_bytes(uint8_t *bytes, uint8_t len);

#ifdef __cplusplus
}
#endif

#endif //__DCCHARDWARE_H__