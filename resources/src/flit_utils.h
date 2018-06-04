#ifndef __FLIT_UTILS_HPP__
#define __FLIT_UTILS_HPP__

#include <stdio.h>
#include <stdint.h>

#define HEADER_FLIT 1
#define BODY_FLIT   2
#define TAIL_FLIT   4

#define PARITY_OFFSET 0
#define DESTINATION_OFFSET 1
#define SOURCE_OFFSET 15
#define FLIT_TYPE_OFFSET 29
#define PACKET_ID_OFFSET 1
#define PACKET_LENGTH_OFFSET 15

#define PARITY_WIDTH 1
#define FLIT_TYPE_WIDTH 3
#define SOURCE_WIDTH 14
#define DESTINATION_WIDTH 14
#define PACKET_ID_WIDTH 14
#define PACKET_LENGTH_WIDTH 14

//#define DEBUG

#ifdef DEBUG
# define DBG_PRINT(x) printf x
#else
# define DBG_PRINT(x) do {} while (0)
#endif


// Clear a bit at position 'p' in the value '*val'
void clear_bit(uint32_t *val, uint32_t p);

// Clear 'range' bits starting at position 'start' in the value '*val'
void clear_bits(uint32_t *val, uint32_t start, uint32_t range);

// Return bit-range from 'start' in the value 'val'
uint32_t get_bit_range(uint32_t val, uint32_t start, uint32_t range);


//Compute even parity of value '*f' and set bit[0]
void set_parity_bit(uint32_t *f);

///////////////////////////////////////////////////
//////                                     ////////
////// Methods to get each field of a flit ////////
//////                                     ////////
///////////////////////////////////////////////////

int get_flit_type(uint32_t *flit);

///////////////////////////////////////////////////
//////                                     ////////
////// Methods to set each field of a flit ////////
//////                                     ////////
///////////////////////////////////////////////////

void set_flit_type(uint32_t *f, uint32_t v);

void set_headerflit_dest_adrs(uint32_t *f, uint32_t v);
void set_headerflit_src_adrs(uint32_t *f, uint32_t v);

void set_first_bodyflit_id(uint32_t *f, uint32_t v);

void set_first_bodyflit_len(uint32_t *f, uint32_t v);

/////////////////////////////////////////////////////
////                                             ////
//// Methods to make a header, body or tail flit ////
////                                             ////
/////////////////////////////////////////////////////

/***************************
** HEADER flit encoding
****************************
Bits        Field
31,30,29    001 (header flit)
28-15       Destination router address
14-1        Source router address.
0           Parity
****************************/
//Make a header-flit in '*h_flit' with the given
//destination address and source address
uint32_t make_header_flit(uint32_t dest_adrs, uint32_t src_adrs);

/***************************
** FIRST BODY flit encoding
****************************
Bits        Field
31,30,29    010 (first body flit)
28-15       Length of the packet, upto 16384 flits.
14-1         Packet ID
0           Parity
****************************/
//Make the first body flit in '*fb_flit' with the given
//length and packet ID
uint32_t make_first_body_flit(uint32_t len, uint32_t id);

/***************************
** BODY flit encoding
****************************
Bits        Field
31,30,29    010 (body flit)
28-1        Payload
0           Parity
****************************/
void set_bodyflit_payload(uint32_t *f, uint32_t v);

//Make a body-flit in '*b_flit' with the given payload
uint32_t make_body_flit(uint32_t payld);

/***************************
** TAIL flit encoding
****************************
Tail flit:
Bits        Field
31,30,29    100 (tail flit)
28-1        Payload
0           Parity
****************************/
void set_tailflit_payload(uint32_t *f, uint32_t v);

//Make a tail-flit in '*t_flit' with the given payload
uint32_t make_tail_flit(uint32_t payld);

////////////////////////////////////////////////////////////////
////                                                        ////
//// Methods to get fields from a header, body or tail flit ////
////                                                        ////
////////////////////////////////////////////////////////////////

/***************************
** HEADER flit encoding
****************************
Bits        Field
31,30,29    001 (header flit)
28-15       Destination router address
15-1        Source router address.
0           Parity
****************************/
//From header 'flit' return error=1 otherwise get
//destination address, source address, and parity
uint32_t parse_header_flit(uint32_t flit, uint16_t *dest_adrs, uint16_t *src_adrs, uint8_t *p);

/***************************
** FIRST BODY flit encoding
****************************
Bits        Field
31,30,29    010 (body flit)
28-15       Length of the packet, upto 4096 flits.
14-1        Packet ID
0           Parity
****************************/
//From header 'flit' return error=1 otherwise get
//length, packet ID and parity
uint32_t parse_first_body_flit(uint32_t flit, uint16_t *len, uint16_t *id, uint8_t *p);

/***************************
** BODY flit encoding
****************************
Bits        Field
31,30,29    010 (body flit)
28-1        Payload
0           Parity
****************************/
//From body 'flit' return error=1 otherwise get payload and parity
uint32_t parse_body_flit(uint32_t flit, uint32_t *payload, uint8_t *p);

/***************************
** TAIL flit encoding
****************************
Tail flit:
Bits        Field
31,30,29    100 (tail flit)
28-1        Payload
0           Parity
****************************/
//From tail 'flit' return error=1 otherwise get payload and parity
uint32_t parse_tail_flit(uint32_t flit, uint32_t *payload, uint8_t *p);

#endif //__FLIT_UTILS_HPP__
