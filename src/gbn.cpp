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

//global variables
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
	/*
	outputfile.open ("output_data.txt", std::ios_base::app);
	outputfile << "A output called\n";
	outputfile << "Input data: ";
	outputfile << *(message.data);
	outputfile << "\n";
	outputfile.close();
	*/
	

	// if this is a new message then increment seq and ack nums
	if (!(timeout)) {
			seqnum++;
			acknum++;
	}

	// make packet
	uint16_t checksum = 0;
	struct pkt packet;
	packet.seqnum = seqnum;
	packet.acknum = acknum;
	packet.checksum = checksum;
	memcpy(packet.payload, message.data, sizeof(message));
	int sum = gen_crc16((uint8_t*)&packet, sizeof(packet));
	packet.checksum = sum;
	
	// if timeout or wrongack send entire inflight_queue
	if (timeout) {
		/*
		outputfile.open ("output_data.txt", std::ios_base::app);
		outputfile << "Timeout\n";
		*/
		timeout = false;
		//wrongack = false;
		
		//refill queue
		while ((inflight_queue.size() < winsize) && !(pkt_queue.empty())) {
			pkt temp = pkt_queue.front();
			pkt_queue.pop();
			inflight_queue.push_back(temp);
		}
		
		//if queue's not empty restart timer
		if (!(inflight_queue.empty())) {
			/*
			outputfile << "Resend timer restart \n";
			*/
			starttimer(0, 15);
		}
		
		//resend all packets inflight queue
		for (int i = 0; i < inflight_queue.size(); i++) {
			/*
			outputfile << "Resend (timeout) message sent from A: ";
			outputfile << *inflight_queue[i].payload;
			outputfile << "\n";
			*/
			tolayer3(0, inflight_queue[i]);
		}
		/*
		outputfile.close();
		*/
	}
	
	// else if nextmsg, if inflight < windowsize pop from pkt_queue, add to inflight_queue and send
	else if (true == false) {
		/*
		outputfile.open ("output_data.txt", std::ios_base::app);
		outputfile << "Next message\n";
		outputfile.close();
		*/
		nextmsg = false;
		
		// if there are messages in flight or messages to be put in flight restart timer
		if (!(inflight_queue.empty()) || !(pkt_queue.empty())) {
			/*
			outputfile.open ("output_data.txt", std::ios_base::app);
			outputfile << "Messages in flight; restart timer\n";
			outputfile.close();
			*/
			starttimer(0, 15);
		}
		
		// refill inflight queue (send each new message)
		while ((inflight_queue.size() < winsize) && !(pkt_queue.empty())) {
			/*
			outputfile.open ("output_data.txt", std::ios_base::app);
			outputfile << "Next message refill loop\n";
			*/
			pkt temp = pkt_queue.front();
			pkt_queue.pop();
			tolayer3(0, temp);
			inflight_queue.push_back(temp);
			
			/*
			outputfile << "Next message send from A: ";
			outputfile << *temp.payload;
			outputfile << "\n";			
			outputfile.close();
			*/
		}
	}
	
	// else if new packet
	else {
		//  if inflight_queue < windowsize add to inflight_queue and send
		/*
		outputfile.open ("output_data.txt", std::ios_base::app);
		outputfile << "New packet from A: ";
		outputfile << *packet.payload;
		outputfile << "\n";
		*/
		if (inflight_queue.size() < winsize) {
			// if inflight queue isn't full double check to see if there aren't queued packets
			while ((inflight_queue.size() < winsize) && !(pkt_queue.empty())) {
				pkt temp = pkt_queue.front();
				pkt_queue.pop();
				tolayer3(0, temp);
				inflight_queue.push_back(temp);
			}
			// if it's empty timer needs to be started
			if (inflight_queue.empty()) {
				starttimer(0, 15);
				inflight_queue.push_back(packet);
				tolayer3(0, packet);
			}
			// if it's still not full add to queue and send;
			else {
				inflight_queue.push_back(packet);
				tolayer3(0, packet);
			}			
			/*
			outputfile << "New send from A: ";
			outputfile << *packet.payload;
			outputfile << "\n";
			*/
		}
		//  else add to pkt_queue
		else {
			pkt_queue.push(packet);
		}
		/*
		outputfile.close();
		*/
	}
	
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet) {
	//right ack
	if ((packet.acknum == a_acknum) || (packet.seqnum == a_seqnum)) {
		/*
		outputfile.open ("output_data.txt", std::ios_base::app);
		outputfile << "Successful transmission!\n";
		outputfile << "(Final Check)\n";
		outputfile << "packet.acknum: ";
		outputfile << packet.acknum;
		outputfile << "\n";
		outputfile << "a_acknum: ";
		outputfile << a_acknum;
		outputfile << "\n";
		outputfile << "packet.seqnum: ";
		outputfile << packet.seqnum;
		outputfile << "\n";
		outputfile << "a_seqnum: ";
		outputfile << a_seqnum;
		outputfile << "\n";
		
		*/
		
		outputfile << "packet being popped: ";
		outputfile << *((inflight_queue.front()).payload);
		outputfile << "\n";
		inflight_queue.pop_front();
		
		a_acknum++;
		a_seqnum++;
		nextmsg = true;
		stoptimer(0);
		outputfile.close();
		
		//maybe add to inflight and send from here? then reset timer (without calling a agian) and restart timer
		
		if ((!inflight_queue.empty() && inflight_queue.size() < winsize) || !pkt_queue.empty()) {
			starttimer(0, 15);
			while ((inflight_queue.size() < winsize) && !(pkt_queue.empty())) {
				/*
				outputfile.open ("output_data.txt", std::ios_base::app);
				outputfile << "Next message refill loop\n";
				*/
				pkt temp = pkt_queue.front();
				pkt_queue.pop();
				tolayer3(0, temp);
				inflight_queue.push_back(temp);
			
				/*
				outputfile << "Next message send from A: ";
				outputfile << *temp.payload;
				outputfile << "\n";			
				outputfile.close();
				*/
			}	
		}
		else {
			return;
		}
	}
	else {
		stoptimer(0);
	}
}

/* called when A's timer goes off */
void A_timerinterrupt() {
	if (nextmsg) {
		nextmsg = false;
	}
	else {
		timeout = true;
		msg temp;
		A_output(temp);
	}
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init() {
	/*
	outputfile.open ("output_data.txt");
	outputfile << "~~~~~~~~~~~~~~~~~ A_init called ~~~~~~~~~~~~~~~~~~~~~~\n";
	outputfile.close();
	*/
	winsize = getwinsize();
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet) {

	/*
	outputfile.open ("output_data.txt", std::ios_base::app);
	outputfile << "Data received by B: ";
	outputfile << *packet.payload;
	outputfile << "\n";
	outputfile << "Actual Seqnum: ";
	outputfile << packet.seqnum;
	outputfile << "\n";
	outputfile << "Actual Acknum: ";
	outputfile << packet.acknum;
	outputfile << "\n";
	outputfile << "Expected Seqnum: ";
	outputfile << b_seqnum;
	outputfile << "\n";
	outputfile << "Expected Acknum: ";
	outputfile << b_acknum;
	outputfile << "\n";
	outputfile.close();
	*/

	uint16_t a_checksum = packet.checksum;
	uint16_t reset = 0;
	packet.checksum = reset;
	int sum = gen_crc16((uint8_t*)&packet, sizeof(packet));
	
	if (!(a_checksum == sum)) {
		//File is corrupted
		return;
	}
	
	// else if next expected
	else if (packet.seqnum == b_seqnum && packet.acknum == b_acknum) {
		//this is the one we expected
		/*
		outputfile.open ("output_data.txt", std::ios_base::app);
		outputfile << "(This is the next expected from B)\n";
		outputfile.close();
		*/
		b_seqnum++;
		b_acknum++;
		tolayer5(1, packet.payload);
		tolayer3(1, packet);
	}
	// if not next expected just send most recent expected
	else {
		pkt temp;
		temp.seqnum = b_seqnum;
		temp.acknum = b_acknum;
		tolayer3(1, packet);
	}
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init() {
	winsize = getwinsize();
}

