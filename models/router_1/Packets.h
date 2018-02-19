/*--------------------------------------------------------------------
 * TITLE: NoC packetization / de-packetization
 * AUTHOR: Karl Janson (karl.janson@ati.ttu.ee)
 * DATE CREATED: 02.12.16
 * FILENAME: ni.h
 * PROJECT: Project Bonfire
 * COPYRIGHT: Software placed into the public domain by the author.
 *    Software 'as is' without warranty. Author liable for nothing.
 * DESCRIPTION:
 *    NoC packetization / de-packetization related functions
 *--------------------------------------------------------------------*/

#include <cstdint>

#ifndef __PACKETS_H__
#define __PACKETS_H__

/* Offset defintions */
#define TX_OFFSET           0   // Data to be sent (32-bit, out)
#define SEND_ENABLE_OFFSET  4   // Enable the transmition of the data written to TX_OFFSET (1-bit, out)
#define RX_OFFSET           8   // Data to be read (32-bit, in)
#define DATA_VALID_OFFSET   12  // Data is valid (1-bit, out)

/* Header flit data offsets */
#define PACKET_ID_OFFSET        0
#define SRC_ADDR_OFFSET 		14
#define DST_ADDR_OFFSET			0
#define PACKET_LENGTH_OFFSET	14

#define PACKET_TYPE_OFFSET      30

/* Flit offsets */
#define PAYLOAD_OFFSET			1
#define FLIT_TYPE_OFFSET		29

/* Flit type definitions */
#define FLIT_TYPE_HEADER	0b001
#define FLIT_TYPE_BODY		0b010
#define FLIT_TYPE_TAIL		0b100

/* Masks for decoding flits */
#define FLIT_CONTENT_MASK		0b11111111111111111111111111111110
#define FLIT_PARITY_BIT_MASK	0b00000000000000000000000000000001
#define FLIT_TYPE_MASK			0b11100000000000000000000000000000
#define FLIT_PAYLOAD_MASK		0b00011111111111111111111111111110

/* Flit receiving errors */
#define FLIT_OK				0
#define FLIT_INVALID_PARITY	1
#define FLIT_INVALID_TYPE	2

/* Masks for decoding the header */
#define FLIT_ID_MASK		0b00000000000000000111111111111110
#define FLIT_SCR_ADDR_MASK	0b00011111111111111000000000000000
#define FLIT_DST_ADDR_MASK	0b00000000000000000111111111111110
#define FLIT_LENGHT_MASK	0b00011111111111111000000000000000

/* Header decoding errors */
#define HEADER_OK					0
#define HEADER_INVALID_ID			1
#define HEADER_INVALID_SRC_ADDR		2
#define HEADER_INVALID_DST_ADDR		3
#define HEADER_SRC_ADDR_IS_DST_ADDR	4
#define HEADER_INVALID_LENGTH		5

#define SIZEOF_PACKET_ID		8

/*
* Decode the flit header.
*
* header:  incoming header
* packetID:	 Counter specifying the packet ID (one-hot, 8-bit)
* srcAddr:		 Address of the source node (4-bit)
* dstAddr:		 Address of the destination node (4-bit)
* packetLength: Length of the packet in flits (including the header and tail, 12 bits)
*
* return:		 Error value
*/

unsigned int header_decode(unsigned int header1,unsigned int header2, int *packetID, int *srcAddr, int *dstAddr, int *packetLength, int noc_size);

/*
* Returns the flit type
*
* flit: The flit to be analyzed
*
* return:   flit type
 */
unsigned int get_flit_type(unsigned int flit);

/*
* Returns flit payload
*
* flit: The flit to be analyzed
*
* return:   flit payload
 */
unsigned int get_flit_payload(unsigned int flit);

unsigned int toHardwareAddress(unsigned int ad, int noc_size);
unsigned int toLogicalAddress(unsigned int ad, int noc_size);

#endif //__PACKETS_H__
