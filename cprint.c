#include "cprint.h"
#include <Arduino.h>
#include <stdio.h>

/* Uno, Duemillenove, Mega etc. */
#if defined(__AVR_ATmega168__) ||defined(__AVR_ATmega168P__) || defined(__AVR_ATmega328P__) \
 || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  #define UCSRXA UCSR0A
  #define UDREX  UDRE0
  #define UDRX   UDR0
  #define RXCX   RXC0
#elif defined(__AVR_ATmega32U4__) /* Leonardo, Esplora, Yun etc. */
  #define UCSRXA UCSR1A
  #define UDREX  UDRE1
  #define UDRX   UDR1
  #define RXCX   RXC1
#endif

int USARTSendByte(char byte, FILE *stream) {
  if(byte == '\n') USARTSendByte('\r', 0);
  /* Wait while previous byte is completed */
  while(!(UCSRXA & (1 << UDREX))){};
  UDRX = byte; /* Send byte */
  return 0;
}

int USARTReceiveByte(FILE *stream) {
  uint8_t byte;
  /* Wait until a byte is received */
  while (!(UCSRXA & (1 << RXCX))){};
  byte = UDRX; /* Copy byte */
  USARTSendByte(byte,stream); /* Echo byte */
  return byte;
}

/* set stream pointer */
FILE usart_str = FDEV_SETUP_STREAM(USARTSendByte, USARTReceiveByte, _FDEV_SETUP_RW);

void cprint_init() {
  stdin = stdout = &usart_str;
}