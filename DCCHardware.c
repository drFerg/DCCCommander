#include <Arduino.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "DCCHardware.h"

#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(__AVR_AT90CAN128__) || defined(__AVR_AT90CAN64__) || defined(__AVR_AT90CAN32__) || defined(__AVR_ATmega32U4__)
  #define PINBN PINB6 /* On Arduino MEGA, OC1A is digital pin 11 or Port B/Pin 5 */
  #if !defined(SINGLE_OUTPUT)
    #define init_rail_pins() DDRB |= (1 << DDB5) | (1 << DDB6); /* OC1B Port B/Pin 6 */
  #else
    #define init_rail_pins() DDRB |= (1 << DDB5);
  #endif // !SINGLE_OUTPUT
#else
  #define PINBN PINB1 /*On Arduino UNO, OC1A is digital pin 9 or Port B/Pin 1 */
  #define init_rail_pins() DDRB |= (1 << DDB1) | (1 << DDB2); /* and OC1B Port B/Pin 2 */
#endif
#define PKT_SIZE 6
#define PREAMBLE_BIT_CNT 14
#define BYTE_BIT_CNT 8
#define current_bit() ((pkt[pkt_size - byte_counter]) >> (bit_counter - 1))

/* Whenever we set OCR1A, we must also set OCR1B, or else OC1B will get out of phase with OC1A! */
#if !defined(SINGLE_OUTPUT)
  #define send_bit(bit) OCR1A = OCR1B = bit
#else
  #define send_bit(bit) OCR1A = bit
#endif // !SINGLE_OUTPUT

/** S 9.1 A - Specifies that bits are represented by a square wave composed by
    two half bit periods on opposite polarities.
      - 1 bit = 58us half-period (116us total) (valid range: 55-61us)
      - 0 bit = >=100us half-period (>=200us total) (valid range: 95-9900us)
    DCC 0 bit stretching is discussed, enabling DC locos to run on DCC lines, 
    however, this is purposely not implemented here due to its limitations and 
    risks.
*/
typedef enum {
#if F_CPU == 16000000
  ONE_BIT = 115, /* 58us */
  ZERO_BIT = 199 /* 100us */
#elif F_CPU == 8000000
  ONE_BIT = 57, /* 58us */
  ZERO_BIT = 99 /* 100us */
#endif
} BITS; /* Timer1 TOP values for one and zero bits */
/** Calculating the timer1 counter values (from ATMega168 datasheet, 15.9.2):
    f_{OC1A} = \frac{f_{clk_I/O}}{2*N*(1+OCR1A)})
    where N = prescalar, and OCR1A is the TOP we need to calculate.
    We know the desired half period for each case, 58us and >100us.
    So:
      for ones:
      58us = (8*(1+OCR1A)) / (16MHz)
      58us * 16MHz = 8*(1+OCR1A)
      58us * 2MHz = 1+OCR1A
      OCR1A = 115

      for zeros:
      100us * 2MHz = 1+OCR1A
      OCR1A = 199

    Thus, we also know that the valid range for stretched-zero operation is something like this:
    9900us = (8*(1+OCR1A)) / (16MHz)
    9900us * 2MHz = 1+OCR1A
    OCR1A = 19799
*/

/** Given the structure of a DCC packet, the ISR can be in one of 5 states.
    DCC_IDLE: there is nothing to put on the rails. In this case, the only legal thing
               to do is to put a '1' on the rails.  The ISR should almost never be in this state.
    DCC_PREAMBLE: A packet has been made available, and so we should broadcast the preamble: 14 '1's in a row.
    DCC_BYTE_START: Each byte of data is preceded by a 0.
    DCC_BYTE_SEND: Sending the current byte.
    DCC_POSTAMBLE: After the final byte is sent, send a 0.
*/
  typedef enum {
  DCC_IDLE,
  DCC_PREAMBLE,
  DCC_BYTE_START,
  DCC_BYTE_SEND,
  DCC_POSTAMBLE
} DCC_state; /* Different states for DCC output state machine */

DCC_state dcc_state = DCC_IDLE; /* Init state machine */
uint8_t pkt[PKT_SIZE] = {0, 0, 0, 0, 0, 0}; /* Current packet being transmitted */
volatile uint8_t pkt_size = 0;
volatile uint8_t byte_counter = 0; /* Bytes left to transmit in packet */
volatile uint8_t bit_counter = PREAMBLE_BIT_CNT; /* init for preamble */

int (*pullNextPacket)(uint8_t **);
/// A fixed-content packet to send when idle
//uint8_t DCC_Idle_Packet[3] = {255,0,255};
/// A fixed-content packet to send to reset all decoders on layout
//uint8_t DCC_Reset_Packet[3] = {0,0,0};

/* setup configures timer1 to CTC mode (for waveform generation), 
   then sets it to toggle OC1A, OC1B, at /8 prescalar, 
   then run the ISR at each interval 
 */
void dcc_init(int (*getNextPacketfunc)(uint8_t **)) {
  pullNextPacket = getNextPacketfunc;

  init_rail_pins();   /* Set the Timer1 pins OC1A and OC1B pins to output mode */

  TCCR1A = (1 << COM1A0);  /* Toggle OC1A on compare match */

#if !defined(SINGLE_OUTPUT)
  TCCR1C |= (1 << FOC1B); /* Force compare match, a toggle OC1B so that it will complement pin OC1A */
  TCCR1A |= (1 << COM1B0); /* Toggle OC1B on compare match */
#endif // ! SINGLE_OUTPUT

  TCCR1B = (1 << WGM12)   /* Enable CTC mode */
         | (1 << CS11);   /* Divide the clock by 8 */

  TCNT1 = 0; /* Init timer to start at 0 */

  TIMSK1 |= (1 << OCIE1A); /* enable the compare match interrupt */

  /* Start outputting 1 bit */
  send_bit(ONE_BIT);
//  printf(">> DCC Hardware setup completed!\n");
}

/* This is the Interrupt Service Routine (ISR) for Timer1 compare match. */
/* In CTC mode, timer TCINT1 automatically resets to 0 when it matches OCR1A.
 * To switch between 1 waveform and 0 waveform, we assign a (timer) value to OCR1A and OCR1B using send_bit(time).
 * Anything we set for OCR1A takes effect IMMEDIATELY, so we are working within the cycle we are setting.
 * A full bit requires two time periods, one outputting a 1 and the second a 0.
 */

int dcc_bytes_left() {
  int bytes_left;
  cli();
  bytes_left = byte_counter;
  sei();
  return bytes_left;
}

int getNextPacket() {
  uint8_t *bytes;
  int size = pullNextPacket(&bytes);
  if (size > PKT_SIZE) return 0;
  memcpy(pkt, bytes, size);
  pkt_size = byte_counter = size;
  return size;
}

void dcc_send_bytes(uint8_t *bytes, uint8_t len) {
  cli();
  memcpy(pkt, bytes, len);
  pkt_size = byte_counter = len;
  sei();
}

ISR(TIMER1_COMPA_vect) {
  /* First check if previous bit fully sent, i.e., PINB goes HIGH -> LOW */
  if(PINB & (1 << PINBN)) return; /* transmit 2nd half of bit (LOW) */
  else { /* Last bit complete, send a new bit */ 
    switch(dcc_state) {
      case DCC_IDLE: /* Check if a new packet is ready, then send preamble, else send 1 bit. */
        if(byte_counter == 0) { /*if no new packet */
          if (getNextPacket() == 0) { /* See if a new packet is waiting to be grabbed */
            send_bit(ONE_BIT); /* Send ones if we don't know what else to do. */
            break;
          } /* else we found a new packet so start preamble */
        }
        dcc_state = DCC_PREAMBLE; //and fall through to DCC_PREAMBLE
      case DCC_PREAMBLE: /* Sending preamble bits(14) before switching to DCC_BYTE_START */
        send_bit(ONE_BIT);
        if(--bit_counter == 0) dcc_state = DCC_BYTE_START;
        break;
      case DCC_BYTE_START: /* About to send a byte, proceed the data with a 0, then move to DCC_BYTE_SEND */
        send_bit(ZERO_BIT);
        dcc_state = DCC_BYTE_SEND;
        bit_counter = BYTE_BIT_CNT;
        break;
      case DCC_BYTE_SEND: /* Sending a byte; current bit is tracked with bit_counter, and current byte with byte_counter */
        if(current_bit() & 1) send_bit(ONE_BIT); /* Check and send either 1 or 0 */
        else send_bit(ZERO_BIT);

        if(--bit_counter == 0) { /* If out of bits, send the next byte */
          if(--byte_counter == 0) /* If out of bytes, packet is sent, move to DCC_POSTAMBLE */
            dcc_state = DCC_POSTAMBLE;
          else /* More bytes left to send, back to DCC_BYTE_START */
            dcc_state = DCC_BYTE_START;
        }
        break;
      case DCC_POSTAMBLE: // Done with the packet. Send out a final '1', then head back to DCC_IDLE to check for a new packet.
        send_bit(ONE_BIT);
        dcc_state = DCC_IDLE;
        bit_counter = PREAMBLE_BIT_CNT; //in preparation for a preamble...
        break;
    }
  }
}
