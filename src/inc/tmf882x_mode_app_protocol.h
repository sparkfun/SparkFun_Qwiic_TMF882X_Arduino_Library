/*
 *****************************************************************************
 * Copyright by ams AG                                                       *
 * All rights are reserved.                                                  *
 *                                                                           *
 * IMPORTANT - PLEASE READ CAREFULLY BEFORE COPYING, INSTALLING OR USING     *
 * THE SOFTWARE.                                                             *
 *                                                                           *
 * THIS SOFTWARE IS PROVIDED FOR USE ONLY IN CONJUNCTION WITH AMS PRODUCTS.  *
 * USE OF THE SOFTWARE IN CONJUNCTION WITH NON-AMS-PRODUCTS IS EXPLICITLY    *
 * EXCLUDED.                                                                 *
 *                                                                           *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS       *
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT         *
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS         *
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT  *
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,     *
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT          *
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,     *
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY     *
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT       *
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE     *
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.      *
 *****************************************************************************
 */

/** @file */

#ifndef __TMF882X_MODE_APP_PROTOCOL_H
#define __TMF882X_MODE_APP_PROTOCOL_H

#include "tmf8x2x_application_registers.h"
#include "tmf8x2x_config_page_common.h"
#include "tmf8x2x_config_page_SPAD.h"
#include "tmf8x2x_config_page_factory.h"
#include "tmf8x2x_electrical_calibration.h"
#include "tmf8x2x_histogram_registers.h"
#include "tmf8x2x_result_registers.h"
#include "tmf8x2x_statistic_registers.h"

//
// mask and shift macros.
//
#define BITFIELD(R, V) (((V)&((1UL<<R##__WIDTH)-1))<<R##__SHIFT)
#define BITMASKSHIFTED(R) ((R##__MASK)>>(R##__SHIFT))
#define SET_BITFIELD(R, D, V) (((D)&~((((1UL<<R##__WIDTH)-1)<<R##__SHIFT)))|(((V)&((1UL<<R##__WIDTH)-1))<<R##__SHIFT))
#define GET_BITFIELD(R, D) (((D)>>R##__SHIFT)&((1UL<<R##__WIDTH)-1))


#define TMF8X2X_BIT_MASK(BF) 	( ( ( 1UL << (BF##__WIDTH) ) - 1 ) << (BF##__SHIFT) )

#define TMF8X2X_BIT_ONLY(BF)	( 1UL << (BF##__SHIFT) )

/** Protocol Structure for results/readout are:
  If the size fits into one packet (i.e. is less than 0xC0, than there
  is no payload field).

  +-----+-----+------+------+-------+-------+-...-+--------------+

  | RID | TID | Size | Size | Data0 | Data1 | ... | Data[Size-1] |

  |  a  |     | LSB  | MSB  |       |       |     |              |

  +-----+-----+------+------+-------+-------+-...-+--------------+

  If the data to be transferred does not fit in one I2C Chunk, i.e. is Bigger than (0xE0 - 0x20)=0xC0,
  than it is split into the following records (each record starts at address TMF8X2C_COM_RESULT_ADDRESS:

  +-----+-----+------+------+---------+---------+---------+-...-+-----------+

  | RID | TID | Total Size  | Payload | Data[0] | Data[1] | ... | Data[m-1] |

  |  b  |  n  | LSB  | MSB  |    m    |         |         |     |           |

  +-----+-----+------+------+---------+---------+---------+-...-+-----------+

  +-----+-----+------+------+---------+---------+-----------+-...-+-----------------+

  | RID | RID | Total Size  | Payload | Data[m] | Data[m+1] | ... | Data[m + (m+1)] |

  |  b  | n+1 | - Payload m |   m+1   |         |           |     |                 |

  +-----+-----+------+------+---------+---------+-----------+-...-+-----------------+

  ....

  Note that the TID changes with each new packet, the Size is reduced with each new packet,
  the payload may change with each packet (most likely it stays the same - and only changes
  with the last packet - if at all), and the RID always stays the same.

*/

/** TID (1Byte), RID (1Byte), Size (2Bytes) */
/** every response has this header (4 bytes) */
#define TMF8X2X_COM_HEADER_SIZE                  ((TMF8X2X_COM_SIZE_MSB) - (TMF8X2X_COM_CONFIG_RESULT) + 1)
#define TMF8X2X_COM_HEADER_PLUS_PAYLOAD          ((0xDF) - (TMF8X2X_COM_CONFIG_RESULT) + 1)
#define TMF8X2X_COM_MAX_PAYLOAD                  ((TMF8X2X_COM_HEADER_PLUS_PAYLOAD) - (TMF8X2X_COM_HEADER_SIZE))


/** If the RID has the TMF8X2X_COM_SUB_PACKET_HEADER_MASK bit set, than there is a subheader immediatly after the header.
 *
 *  The optional sub-packet header consists of 2 bytes:
 *   A running number identifying which chunk of the complete packet is transitted.
 *   A payload byte giving the amount of data that is following in this sub-packet.
 *
 *  Note: The total size written to TMF8X2X_COM_SIZE_LSB *excludes* the sub-packet headers!
 *  This makes it easier, otherwise  we would need to calculate into how many sub-packets we cut
 * the complete packet before starting to send.
 */
#define TMF8X2X_COM_OPTIONAL_SUBPACKET_HEADER_SIZE              3                               /** if there is a sub-packet, this is the size of the sub-packet header */
#define TMF8X2X_COM_OPTIONAL_SUBPACKET_HEADER_MASK              (0x80)                          /** this is the bit that has to be set to indicated in the RID that there is a sub-packet header */
#define TMF8X2X_COM_OPTIONAL_SUBPACKET_NUMBER_ADDRESS           ((TMF8X2X_COM_SIZE_LSB)+2)  /** the sub-packet header is right after the packet header */
#define TMF8X2X_COM_OPTIONAL_SUBPACKET_PAYLOAD_ADDRESS          ((TMF8X2X_COM_OPTIONAL_SUBPACKET_NUMBER_ADDRESS)+1)   /** the size of the payload in the sub-packet comes as 2nd byte of the sub-packet header */
#define TMF8X2X_COM_OPTIONAL_SUBPACKET_CONFIG_ID_ADDRESS          ((TMF8X2X_COM_OPTIONAL_SUBPACKET_NUMBER_ADDRESS)+2)   /** the CONFIG ID is the subcapture number of this subpacket message or breakpoint #*/

/** Histogram types */

/** build the RID for histograms from select bit + sub-packet header mask: histograms never fit in 1 I2C packet */
#define TMF8X2X_COM_RID_FOR_HISTOGRAM( histType )               ( (TMF8X2X_COM_OPTIONAL_SUBPACKET_HEADER_MASK) | (histType) )

/** The combination of the original RID + sub-packet indication */
#define TMF8X2X_COM_RID_RAW_HISTOGRAM_24_BITS          ( TMF8X2X_COM_RID_FOR_HISTOGRAM( TMF8X2X_COM_HIST_DUMP__histogram__raw_24_bit_histogram ) )
#define TMF8X2X_COM_RID_ELECTRICAL_CALIBRATION_24_BITS ( TMF8X2X_COM_RID_FOR_HISTOGRAM( TMF8X2X_COM_HIST_DUMP__histogram__electrical_calibration_24_bit_histogram ) )

#define TMF8X2X_COM_RID_BREAKPOINT_HIT                 ( TMF8X2X_COM_RID_FOR_HISTOGRAM( TMF8X2X_COM_HIST_DUMP__histogram__breakpoint_hit ) )    /** Breakpoint information - generic format */

#define TMF8X2X_COM_MAX_MEASUREMENT_RESULTS  (36)
#define TMF8X2X_COM_MAX_SPAD_XSIZE           (18)
#define TMF8X2X_COM_MAX_SPAD_YSIZE           (10)
#define TMF8X2X_COM_MAX_SPAD_SIZE            (TMF8X2X_COM_MAX_SPAD_XSIZE * \
                                              TMF8X2X_COM_MAX_SPAD_YSIZE)
#define TMF8X2X_MAX_CONFIGURATIONS            2
#define TMF8X2X_MAIN_SPAD_VERTICAL_LSB_SHIFT     ( 0 )
#define TMF8X2X_MAIN_SPAD_VERTICAL_MID_SHIFT     ( 10 )
#define TMF8X2X_MAIN_SPAD_VERTICAL_MSB_SHIFT     ( 20 )
/** each channel can be encoded in 3 bits */
#define TMF8X2X_MAIN_SPAD_BITS_PER_CHANNEL       ( 3 )

/** to encode a single channel, we need to set 3 bits at specific positions in the 32-bit word */
#define TMF8X2X_MAIN_SPAD_ENCODE_CHANNEL( channel, yPosition )                                                       \
        ( ( ( ( (channel) & 1 /*LSB*/ )      ) << ( (TMF8X2X_MAIN_SPAD_VERTICAL_LSB_SHIFT) + (yPosition) ) )         \
        | ( ( ( (channel) & 2 /*Mid*/ ) >> 1 ) << ( (TMF8X2X_MAIN_SPAD_VERTICAL_MID_SHIFT) + (yPosition) ) )         \
        | ( ( ( (channel) & 4 /*MSB*/ ) >> 2 ) << ( (TMF8X2X_MAIN_SPAD_VERTICAL_MSB_SHIFT) + (yPosition) ) )         \
        )

/** to decode a single channel, we need to get 3 bits at specific positions in the 32-bit word */
#define TMF8X2X_MAIN_SPAD_DECODE_CHANNEL( config, yPosition )                                                        \
        ( ( ( ( (config) >> ( (TMF8X2X_MAIN_SPAD_VERTICAL_LSB_SHIFT) + (yPosition) ) )      ) & 1 /*LSB*/ )          \
        | ( ( ( (config) >> ( (TMF8X2X_MAIN_SPAD_VERTICAL_MID_SHIFT) + (yPosition) ) ) << 1 ) & 2 /*Mid*/ )          \
        | ( ( ( (config) >> ( (TMF8X2X_MAIN_SPAD_VERTICAL_MSB_SHIFT) + (yPosition) ) ) << 2 ) & 4 /*MSB*/ )          \
        )

#endif
