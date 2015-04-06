#include "cprint.h"
#include <Arduino.h>
#include <stdio.h>

int USART0SendByte(char u8Data, FILE *stream) {
  if(u8Data == '\n') USART0SendByte('\r', 0);
  //wait while previous byte is completed
  while(!(UCSR0A&(1<<UDRE0))){};
  // Transmit data
  UDR0 = u8Data;
  return 0;
}
int USART0ReceiveByte(FILE *stream) {
  uint8_t u8Data;
  // Wait for byte to be received
  while(!(UCSR0A&(1<<RXC0))){};
  u8Data=UDR0;
  //echo input data
  USART0SendByte(u8Data,stream);
  // Return received data
  return u8Data;
}
//set stream pointer
FILE usart0_str = FDEV_SETUP_STREAM(USART0SendByte, USART0ReceiveByte, _FDEV_SETUP_RW);

void cprint_init() {
  stdin = stdout = &usart0_str;
}