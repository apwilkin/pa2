#include "../include/simulator.h"
#include <queue>
#include <deque>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include "stdint.h"
#include <fstream>

#define CRC16 0x8005

/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for PA2, unidirectional data transfer 
   protocols (from A to B). Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).
**********************************************************************/

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/

bool timeout = false;
bool wrongack = false;
bool nextmsg = false;
bool reset = false;
int seqnum = -1;
int acknum = -1;
int a_seqnum = 0;
int a_acknum = 0;
int b_seqnum = 0;
int b_acknum = 0;
int winsize;
std::queue<pkt> pkt_queue;
std::deque<pkt> inflight_queue;

std::ofstream outputfile;

//Solution from stack overflow: http://stackoverflow.com/questions/10564491/function-to-calculate-a-crc16-checksum
uint16_t gen_crc16(const uint8_t *data, uint16_t size) {
	uint16_t out = 0;
	int bits_read = 0, bit_flag;

	/* Sanity check: */
	if(data == NULL)
		return 0;

	while(size > 0)	{
		bit_flag = out >> 15;

		/* Get next bit: */
		out <<= 1;
		out |= (*data >> bits_read) & 1; // item a) work from the least significant bits

		/* Increment bit counter: */
		bits_read++;
		if(bits_read > 7) {
		    bits_read = 0;
		    data++;
		    size--;
		}

		/* Cycle check: */
		if(bit_flag)
		    out ^= CRC16;
	}

	// item b) "push out" the last 16 bits
	int i;
	for (i = 0; i < 16; ++i) {
		bit_flag = out >> 15;
		out <<= 1;
		if(bit_flag)
		    out ^= CRC16;
	}

	// item c) reverse the bits
	uint16_t crc = 0;
	i = 0x8000;
	int j = 0x0001;
	for (; i != 0; i >>=1, j <<= 1) {
		if (i & out) crc |= j;
	}

	return crc;
}
//end solution


/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message) {
	
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet) {
	
}

/* called when A's timer goes off */
void A_timerinterrupt() {
	
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init() {
	winsize = getwinsize();
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet) {
	
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init() {
	winsize = getwinsize();
}


