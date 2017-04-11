#include "../include/simulator.h"
#include <queue>
#include <deque>
#include <vector>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include "stdint.h"
#include <fstream>
#include <ctime>

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
int timer_acknum = -1;
int timer_seqnum = -1;
int a_expected_seqnum = 0;
int a_expected_acknum = 0;
int b_expected_seqnum = 0;
int b_expected_acknum = 0;
int usn;
int winsize;
int winstart
int timeout_int = 50;
float current_time;
float start_time;

struct timeoutStruct {
	timeoutStruct(): intransit(false), timeout_value(get_sim_time() + timeout_int) {}
	bool intransit;
	float timeout_value;
	pkt timeout_packet;
};


std::queue<timeoutStruct> pkt_queue;
std::deque<timeoutStruct> timeout_queue;
std::vector<pkt> arcv_buffer;
std::vector<pkt> brcv_buffer;
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

	// increment seq and ack nums
	if (timer_seqnum == 2*winsize + 1) {
		timer_seqnum = 0;
		timer_acknum = 0;
	}
	else {
		timer_seqnum++;
		timer_acknum++;
	}
	
	// make packet
	uint16_t checksum = 0;
	struct pkt packet;
	packet.seqnum = timer_seqnum;
	packet.acknum = timer_acknum;
	packet.checksum = checksum;
	memcpy(packet.payload, message.data, sizeof(message));
	int sum = gen_crc16((uint8_t*)&packet, sizeof(packet));
	packet.checksum = sum;
	
	struct timeoutStruct newdata;
	newdata.timeout_packet = packet;
	
	// if queue index(seqnum) is full add to pkt_queue
	if ((timeout_queue[newdata.timeout_packet.seqnum]).intransit) {
		pkt_queue.push(newdata);
	}
	// else set intransit to true (need to set this to false when "popping")
	// add to queue (by seqnum index) and send
	else {
		newdata.intransit = true;
		timeout_queue[newdata.timeout_packet.seqnum] = newdata;
		outputfile.open ("output_data.txt", std::ios_base::app);
		outputfile << "Data sent by A: ";
		outputfile << *(newdata.timeout_packet.payload);
		outputfile << "\n";
		outputfile.close();
		tolayer3(0, newdata.timeout_packet);
	}
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet) {
	outputfile.open ("output_data.txt", std::ios_base::app);
	outputfile << "A input called\n";
	if ((packet.seqnum == a_expected_seqnum) && (packet.acknum == a_expected_acknum)) {
		(timeout_queue[packet.seqnum]).intransit = false;
		outputfile << "Packet 'nullified': ";
		outputfile << *((timeout_queue[packet.seqnum]).timeout_packet.payload);
		outputfile << "\n";
		outputfile.close();
		if (a_expected_seqnum == 2*winsize + 1) {
			a_expected_seqnum = 0;
			a_expected_acknum = 0;
		}
		else {
			a_expected_seqnum++;
			a_expected_acknum++;
		}
	}
	// if expected (timeout_queue[packet.seqnum]).inflight to false (increment usn(not on a side?) and expected?)
	// while (rcv buffer contains next expected) { pop next expected;}
	// refill/send all queued messages into the timeoutQueue?
	else {
		arcv_buffer.push_back(packet);
	}
	
	bool more_expected = true;
	while (more_expected) {
	more_expected = false;
		for (int i = 0; i < arcv_buffer.size(); i++) {
			if (arcv_buffer[i].seqnum == a_expected_seqnum) {
				more_expected = true;
				if (timer_seqnum == 2*winsize + 1) {
					a_expected_seqnum = 0;
					a_expected_acknum = 0;
				}
				else {
					a_expected_seqnum++;
					a_expected_acknum++;
				}
				arcv_buffer.erase(arcv_buffer.begin() + i);
			}
		}
	}
	
}

/* called when A's timer goes off */
void A_timerinterrupt() {
	for (int i = 0; i < timeout_queue.size(); i++) {
		if (timeout_queue[i].intransit) {
			if (get_sim_time() >= (timeout_queue[i]).timeout_value) {
				(timeout_queue[i]).timeout_value = get_sim_time() + timeout_int;
				outputfile.open ("output_data.txt", std::ios_base::app);
				outputfile << "Data resend by A: ";
				outputfile << *(timeout_queue[i].timeout_packet.payload);
				outputfile << "\n";
				outputfile.close();
				tolayer3(0, timeout_queue[i].timeout_packet);
			}
		}
	}
	starttimer(0, 20);
	// go through timeout_queue
	// if intransit
	// if get_sim_timer() >= timeout_value resend that packet
	// starttimer(0);
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init() {
	outputfile.open ("output_data.txt");
	outputfile << "~~~~~~~~~~~~~~~~~ A_init called ~~~~~~~~~~~~~~~~~~~~~~\n";
	outputfile.close();
	start_time = get_sim_time();
	winsize = getwinsize();
	winstart = 0;
	usn = 2*winsize + 1;
	timeout_queue.resize(usn);
	starttimer(0, 20);
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet) {
	outputfile.open ("output_data.txt", std::ios_base::app);
	outputfile << "Data received by B: ";
	outputfile << *packet.payload;
	outputfile << "\n";
	outputfile << "Packet Seqnum: ";
	outputfile << packet.seqnum;
	outputfile << "\n";
	outputfile << "Packet Acknum: ";
	outputfile << packet.acknum;
	outputfile << "\n";
	outputfile << "Expected Seqnum: ";
	outputfile << b_expected_seqnum;
	outputfile << "\n";
	outputfile << "Expected Acknum: ";
	outputfile << b_expected_acknum;
	outputfile << "\n";
	outputfile.close();
	
	uint16_t a_checksum = packet.checksum;
	uint16_t reset = 0;
	packet.checksum = reset;
	int sum = gen_crc16((uint8_t*)&packet, sizeof(packet));
	
	if (!(a_checksum == sum)) {
		//File is corrupted
		return;
	}
	
	// if packet outside window return
	if (packet.seqnum == usn) {
		outputfile.open ("output_data.txt", std::ios_base::app);
		outputfile << "Packet outside window! : ";
		outputfile << *packet.payload;
		outputfile << "\n";
		outputfile.close();
		return;
	}
	// else if next expected send up
	else if (packet.seqnum == b_expected_seqnum) {
		outputfile.open ("output_data.txt", std::ios_base::app);
		outputfile << "B sending packet back to A : ";
		outputfile << *packet.payload;
		outputfile << "\n";
		outputfile.close();
		if (timer_seqnum == 2*winsize + 1) {
			b_expected_seqnum = 0;
			b_expected_acknum = 0;
			usn = 0;
		}
		else {
			b_expected_seqnum++;
			b_expected_acknum++;
			usn++;
		}
		tolayer5(1, packet.payload);
		tolayer3(1, packet);
	}
	// else if (!(packet.seqnum == b_seqnum)) buffer, send ack
	else {
		brcv_buffer.push_back(packet);
	}
	// loop through buffer checking for more expected seqnums
	bool more_expected = true;
	while (more_expected) {
	more_expected = false;
		for (int i = 0; i < brcv_buffer.size(); i++) {
			if (brcv_buffer[i].seqnum == b_expected_seqnum) {
				more_expected = true;
				if (timer_seqnum == 2*winsize + 1) {
					b_expected_seqnum = 0;
					b_expected_acknum = 0;
					usn = 0;
				}
				else {
					b_expected_seqnum++;
					b_expected_acknum++;
					usn++;
				}
				tolayer5(1, (arcv_buffer[i]).payload);
				tolayer3(1, arcv_buffer[i]);
				arcv_buffer.erase(arcv_buffer.begin() + i);
			}
		}
	}
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init() {
	winsize = getwinsize();
	usn = 2*winsize + 1;
}


