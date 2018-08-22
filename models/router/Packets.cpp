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

#include "Packets.h"

#include <stdint.h>

unsigned int header_decode(unsigned int header1, int *srcAddr, int *dstAddr,
		int noc_size) {

	*srcAddr = (header1 & FLIT_SCR_ADDR_MASK) >> (SRC_ADDR_OFFSET + 1);
	*dstAddr = (header1 & FLIT_DST_ADDR_MASK) >> (DST_ADDR_OFFSET + 1);

	*srcAddr = toLogicalAddress(*srcAddr, noc_size);
	*dstAddr = toLogicalAddress(*dstAddr, noc_size);

	if (*srcAddr > noc_size * noc_size) {
		return HEADER_INVALID_SRC_ADDR;

	} else if (*dstAddr > noc_size * noc_size) {
		return HEADER_INVALID_DST_ADDR;

	} else if (dstAddr == srcAddr) {
		return HEADER_SRC_ADDR_IS_DST_ADDR;

	} else {
		return HEADER_OK;
	}

}

/*
 * Returns the flit type
 *
 * flit: The flit to be analyzed
 *
 * return:   flit type
 */
unsigned int get_flit_type(unsigned int flit) {
	return (flit & FLIT_TYPE_MASK) >> FLIT_TYPE_OFFSET;
}

/*
 * Returns flit payload
 *
 * flit: The flit to be analyzed
 *
 * return:   flit payload
 */
unsigned int get_flit_payload(unsigned int flit) {
	return (flit & FLIT_PAYLOAD_MASK) >> PAYLOAD_OFFSET;
}

unsigned int toHardwareAddress(unsigned int ad, int noc_size) {
	uint32_t x = ad % noc_size;
	uint32_t y = ad / noc_size;
	return x + (y << 7);
}

unsigned int toLogicalAddress(unsigned int ad, int noc_size) {
	uint32_t x = ad & 0x7f;
	uint32_t y = (ad & 0x3f80) >> 7;
	return x + (y * noc_size);
}
