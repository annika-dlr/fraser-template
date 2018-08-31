#include "flit_utils.h"

#include <stdio.h>
#include <stdint.h>

// Clear a bit at position 'p' in the value '*val'
void clear_bit(uint32_t *val, uint32_t p)
{
    DBG_PRINT(("DEBUG: clear_bit val: 0x%x, pos %d\n", *val, p));
    uint32_t mask = 1<<p; //Set the bit to be cleared
    *val = *val & ~mask;
}

// Clear 'range' bits starting at position 'start' in the value '*val'
void clear_bits(uint32_t *val, uint32_t start, uint32_t range)
{
    uint32_t i;
    for(i=0 ; i<range; i++)
    {
        clear_bit(val, start+i);
    }
}

// Return bit-range from 'start' in the value 'val'
uint32_t get_bit_range(uint32_t val, uint32_t start, uint32_t range)
{
    uint32_t i, result=0;

    //for each interation, extract bit at (start+i)
    //and insert at left-shifted position of 'i' in result.
    for(i=0 ; i<range; i++)
    {
        result |= ((val >> (start+i)) & 1) << i;
    }

    return result;
}


//Compute even parity of value '*f' and set bit[0]
void set_parity_bit(uint32_t *f)
{
    //clear the parity bit at location [0]
    //so that it is not taken in later computation
    clear_bit(f, 0);

    //www.graphics.stanford.edu/~seander/bithacks.html#ParityParallel
    //Compute parity in parallel
    uint32_t v = *f;  // word value to compute the parity of
    v ^= v >> 16;
    v ^= v >> 8;
    v ^= v >> 4;
    v &= 0xf;

    //Set bit[0]
    *f |= (0x6996 >> v) & 1;
}

///////////////////////////////////////////////////
//////                                     ////////
////// Methods to get each field of a flit ////////
//////                                     ////////
///////////////////////////////////////////////////

int get_flit_type(uint32_t flit)
{
    int flit_type = get_bit_range(flit, FLIT_TYPE_OFFSET, FLIT_TYPE_WIDTH);

    return flit_type;      //return the flit time
}

///////////////////////////////////////////////////
//////                                     ////////
////// Methods to set each field of a flit ////////
//////                                     ////////
///////////////////////////////////////////////////

void set_flit_type(uint32_t *f, uint32_t v)
{
    int loc = FLIT_TYPE_OFFSET;             //field location to update
    clear_bits(f, loc, FLIT_TYPE_WIDTH);    //clear field location
    *f |= v << loc;                         //insert field value

    set_parity_bit(f);                      //update parity bit
}

void set_headerflit_dest_adrs(uint32_t *f, uint32_t v)
{
    int loc = DESTINATION_OFFSET;           //field location to update
    clear_bits(f, loc, DESTINATION_WIDTH);  //clear field location
    *f |= v << loc;                         //insert field value

    set_parity_bit(f);                      //update parity bit
}
void set_headerflit_src_adrs(uint32_t *f, uint32_t v)
{
    int loc = SOURCE_OFFSET;            //field location to update
    clear_bits(f, loc, SOURCE_WIDTH);   //clear field location
    *f |= v << loc;                     //insert field value

    set_parity_bit(f);                  //update parity bit
}

void set_first_bodyflit_id(uint32_t *f, uint32_t v)
{
    int loc = PACKET_ID_OFFSET;             //field location to update
    clear_bits(f, loc, PACKET_ID_WIDTH);    //clear field location
    *f |= v << loc;                         //insert field value

    set_parity_bit(f);                      //update parity bit
}

void set_first_bodyflit_len(uint32_t *f, uint32_t v)
{
    int loc = PACKET_LENGTH_OFFSET;             //field location to update
    clear_bits(f, loc, PACKET_LENGTH_WIDTH);    //clear field location
    *f |= v << loc;                             //insert field value

    set_parity_bit(f);                          //update parity bit
}

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
uint32_t make_header_flit(uint32_t dest_adrs, uint32_t src_adrs)
{

    uint32_t *h_flit = new uint32_t(0);

    set_flit_type(h_flit, HEADER_FLIT);

    set_headerflit_dest_adrs(h_flit, dest_adrs);
    set_headerflit_src_adrs(h_flit, src_adrs);

    return *h_flit;
} //end void make_header_flit()

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
uint32_t make_first_body_flit(uint32_t len, uint32_t id)
{
    uint32_t *fb_flit = new uint32_t(0);

    set_flit_type(fb_flit, BODY_FLIT);

    set_first_bodyflit_len(fb_flit, len);
    set_first_bodyflit_id(fb_flit, id);

    return *fb_flit;
} //end void make_first_body_flit()

/***************************
** BODY flit encoding
****************************
Bits        Field
31,30,29    010 (body flit)
28-1        Payload
0           Parity
****************************/
void set_bodyflit_payload(uint32_t *f, uint32_t v)
{
    int loc = 1;            //field location to update
    clear_bits(f, loc, 28); //clear field location
    *f |= v << loc;         //insert field value

    set_parity_bit(f);      //update parity bit
}
//Make a body-flit in '*b_flit' with the given payload
uint32_t make_body_flit(uint32_t payld)
{
    uint32_t *b_flit = new uint32_t(0);

    set_flit_type(b_flit, BODY_FLIT);

    set_bodyflit_payload(b_flit, payld);

    return *b_flit;
} //end void make_body_flit()

/***************************
** TAIL flit encoding
****************************
Tail flit:
Bits        Field
31,30,29    100 (tail flit)
28-1        Payload
0           Parity
****************************/
void set_tailflit_payload(uint32_t *f, uint32_t v)
{
    int loc = 1;            //field location to update
    clear_bits(f, loc, 28); //clear field location
    *f |= v << loc;         //insert field value

    set_parity_bit(f);      //update parity bit
}
//Make a tail-flit in '*t_flit' with the given payload
uint32_t make_tail_flit(uint32_t payld)
{
    uint32_t *t_flit = new uint32_t(0);

    set_flit_type(t_flit, TAIL_FLIT);

    set_tailflit_payload(t_flit, payld);

    return *t_flit;
} //end void make_tail_flit()

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
uint32_t parse_header_flit(uint32_t flit, uint16_t *dest_adrs, uint16_t *src_adrs, uint8_t *p)
{

    //check flit-type field(bits 31,30,29) to indicate HEADER_FLIT,
    //return 1 on error
    uint16_t flit_type = get_bit_range(flit, FLIT_TYPE_OFFSET, FLIT_TYPE_WIDTH);
    if(flit_type != HEADER_FLIT)
    {
        printf("Error! Got unexpected flit-type(%d) for header-flit type(%d)\n", flit_type, HEADER_FLIT);
        return 1;
    }

    *dest_adrs  = get_bit_range(flit, DESTINATION_OFFSET, DESTINATION_WIDTH);
    *src_adrs   = get_bit_range(flit, SOURCE_OFFSET, SOURCE_WIDTH);
    *p          = get_bit_range(flit, PARITY_OFFSET, PARITY_WIDTH);

    return 0; //No error
} //end parse_header_flit()


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
uint32_t parse_first_body_flit(uint32_t flit, uint16_t *len, uint16_t *id, uint8_t *p)
{

    //check flit-type field(bits 31,30,29) to indicate HEADER_FLIT,
    //return 1 on error
    uint32_t flit_type = get_bit_range(flit, FLIT_TYPE_OFFSET, FLIT_TYPE_WIDTH);
    if(flit_type != BODY_FLIT)
    {
        printf("Error! Got unexpected flit-type(%d) for body-flit type(%d)\n", flit_type, HEADER_FLIT);
        return 1;
    }

    *len        = get_bit_range(flit, PACKET_LENGTH_OFFSET, PACKET_LENGTH_WIDTH);
    *id         = get_bit_range(flit, PACKET_ID_OFFSET, PACKET_ID_WIDTH);
    *p          = get_bit_range(flit, PARITY_OFFSET, PARITY_WIDTH);

    return 0; //No error
} //end parse_header_flit()

/***************************
** BODY flit encoding
****************************
Bits        Field
31,30,29    010 (body flit)
28-1        Payload
0           Parity
****************************/
//From body 'flit' return error=1 otherwise get payload and parity
uint32_t parse_body_flit(uint32_t flit, uint32_t *payload, uint8_t *p)
{
    //check flit-type field(bits 31,30,29) to indicate BODY_FLIT,
    //return 1 on error
    uint32_t flit_type = get_bit_range(flit, 29, 3);
    if(flit_type != BODY_FLIT)
    {
        printf("Error! Got unexpected flit-type(%d) for body-flit type(%d)\n", flit_type, BODY_FLIT);
        return 1;
    }

    *payload = get_bit_range(flit, 1, 28-1+1);
    *p       = get_bit_range(flit, 0, 1);

    return 0; //No error
} //end parse_body_flit()

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
uint32_t parse_tail_flit(uint32_t flit, uint32_t *payload, uint8_t *p)
{
    //check flit-type field(bits 31,30,29) to indicate TAIL_FLIT,
    //return 1 on error
    uint32_t flit_type = get_bit_range(flit, 29, 3);
    if(flit_type != TAIL_FLIT)
    {
        printf("Error! Got unexpected flit-type(%d) for tail-flit type(%d)\n", flit_type, TAIL_FLIT);
        return 1;
    }

    *payload = get_bit_range(flit, 1, 28-1+1);
    *p       = get_bit_range(flit, 0, 1);

    return 0; //No error
} //end parse_tail_flit()
