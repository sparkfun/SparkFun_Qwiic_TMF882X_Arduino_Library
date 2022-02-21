/*
 *****************************************************************************
 * Copyright by ams AG                                                       *
 * All rights are reserved.                                                  *
 *                                                                           *
 * IMPORTANT - PLEASE READ CAREFULLY BEFORE COPYING, INSTALLING OR USING     *
 * THE SOFTWARE.                                                             *
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
/*
 *****************************************************************************
 * INCLUDES
 *****************************************************************************
 */
#include "inc/tmf882x_host_interface.h"
#include "inc/intel_hex_interpreter.h"

/*
 *****************************************************************************
 * TYPES
 *****************************************************************************
 */

/*
 *****************************************************************************
 * VARIABLES
 *****************************************************************************
 */

/*
 *****************************************************************************
 * FUNCTIONS
 *****************************************************************************
 */

/* convert an ascii nibble to its binary value exit loader in case
   that the ascii does not represent a number */
static uint8_t asciiToBinaryNibble ( uint8_t a, int32_t * error )
{
    uint8_t b = a - '0';
    if ( b > 9 )
    {
        b = a - 'A';
        if ( b > 5 )  /* 'A'..'F': note that we still have to add 10 */
        {
            if ( error )
            {
                *error = INTEL_HEX_ERR_NOT_A_NUMBER;
            }
            b = 0;
        }
        b += 10; /* 'A'..'F' means 10, 11, 12, 13,.. not 1, 2, ... 6 */
    }
    return b;
}

/* convert 2 ascii uint8_tacters into a single byte - if possible.
   flag an error in variable intelHexError in case
   that the ascii uint8_tacter does not represent a number */
static uint8_t asciiToBinaryByte ( const uint8_t * * linePtr, int32_t * error )
{
    const uint8_t * line = *linePtr;
    (*linePtr) += 2;
    return ( asciiToBinaryNibble( *line, error ) << 4 ) | asciiToBinaryNibble( *(line+1), error );
}

static int32_t parse_record (intelRecord *rec, const uint8_t * record, uint32_t len)
{
    int32_t result = INTEL_HEX_ERR_TOO_SHORT;
    int32_t lineLength = 0;
    uint32_t end_idx = 0;

    if (!rec || !record || len <= 1) return result;

    lineLength = len;
    // assume record string contains many records
    for (end_idx = 1; end_idx < len; end_idx++) {
        if (record[end_idx] == ':') {
            lineLength = end_idx;
            break;
        }
    }

    if ( lineLength && record[ 0 ] == ':' ) /* intel hex records must start with a colon */
    {
        /* looks promising -> we found the starting uint8_tacter of an intel hex record. */
        uint8_t crc;
        uint8_t type;
        uint8_t data;
        int32_t i;

        /* intel hex records we interpret as follow:
           :llaaaattdddd...dddcc
           l=length a=address t=type d=data c=checksum
           So an intel hex record has as 11 uint8_tacters that
           are not data.
         */

        if ( lineLength < INTEL_HEX_MIN_LAST_ADDRESS )
        {
            return INTEL_HEX_ERR_TOO_SHORT;
        }
        else
        {
            result = lineLength;
            lineLength -= INTEL_HEX_MIN_LAST_ADDRESS; /* substract the minimum address from
            the last written address -> out comes the lineLength (of the real data) */

            /* 1. the first uint8_tacter (':') has to be eaten */
            record++;

            /* 2. read length - 2 ascii = 8 bit value */
            rec->length = asciiToBinaryByte( &record, &result );
            crc = rec->length; /* start calculating crc */

            /* 3. read address - 4 ascii = 16 bit value */
            rec->address = asciiToBinaryByte( &record, &result );
            crc += rec->address;
            rec->address <<= 8; /* move up by 1 byte */
            data = asciiToBinaryByte( &record, &result );
            crc += data;
            rec->address += data;

            /* 4. read type - 2 ascii = 8 bit value */
            type = asciiToBinaryByte( &record, &result );
            crc += type;

            if ( ( rec->length * 2 ) > lineLength )
            {
                return INTEL_HEX_ERR_TOO_SHORT;
            }
            else /* record still valid */
            {
                for ( i = 0; i < rec->length; ++i )
                { /* fill data into record */
                    data = asciiToBinaryByte( &record, &result );
                    crc += data;
                    rec->data[ i ] = data;
                }

                /* read crc and compare */
                data = asciiToBinaryByte( &record, &result );
                crc = ( -crc ) & 0xff;
                if ( crc != data )
                {
                    return INTEL_HEX_ERR_CRC_ERR;
                }
                else /* record still valid */
                {
                    /* depending on the type we interpret the data differently and must adjust the length */
                    if ( type == INTEL_HEX_TYPE_DATA )
                    {
                        /* not an EOF record - conversion errors directly lead to an abort */
                    }
                    else if ( type == INTEL_HEX_TYPE_EOF )
                    {
                        rec->length = 0;
                        /* impossible ULBA (only upper 16-bits are allowed to be non-zero) */
                        rec->ulba = 0xFFFFFFFFUL;
                        result = 0;
                    }
                    else if ( type == INTEL_HEX_TYPE_EXT_LIN_ADDR )
                    {
                        rec->ulba = rec->data[ 0 ];
                        rec->ulba <<= 8;
                        rec->ulba |= rec->data[ 1 ];
                        rec->ulba <<= 16;
                        rec->length = 0; /* no other valid data */
                        /* not an EOF, - conversion errors directly lead to an abort */;
                    }
                    else if ( type == INTEL_HEX_TYPE_START_LIN_ADDR )
                    {
                        rec->length = 0;
                    }
                    else
                    {
                        return INTEL_HEX_ERR_UNKNOWN_TYPE;
                    }
                }
            }
        }
    }

    return result; /* valid data received within time */
}

void ihexi_init(struct intel_hex_interpreter *hex, const uint8_t * hex_records,
                uint32_t size)
{
    if (!hex || !hex_records) return;
    memset(hex, 0, sizeof(struct intel_hex_interpreter));
    hex->hex_records = hex_records;
    hex->hex_size = size;
}

int32_t ihexi_get_next_bin(struct intel_hex_interpreter *hex, uint8_t * buf,
                       uint32_t length, uint32_t * addr)
{
    uint32_t s = 0;
    uint32_t a = 0;
    uint32_t head_addr = 0;
    int32_t rc = -1;

    if ( !hex  || !buf || !addr || !(hex->hex_records))
        return -1;

    a = hex->last_addr;
    head_addr = hex->last_addr;

    while ( (rc = parse_record(&hex->rec, &hex->hex_records[hex->count],
                               hex->hex_size - hex->count)) ) {

        // If we have run out of records to process, break and return what we
        //   have accumulated so far
        if (rc == INTEL_HEX_ERR_TOO_SHORT)
            break;

        // Record parsing error
        if (rc < 0)
            return rc;

        // 1. check for address records
        if (INTEL_HEX_ULBA(a) != hex->rec.ulba) {
            if (s != 0) {
                // we are in the middle of a data block, so stop reading
                break;
            } else {
                // set new starting address
                a = hex->rec.ulba;
                head_addr = hex->rec.ulba;
            }
        }

        // 2. check that data block will fit in buffer
        if (s + hex->rec.length > length)
            break;

        // 3. check that data block is continuous with current address
        if ((head_addr & 0xFFFF) != hex->rec.address && s != 0)
            break;

        memcpy(&buf[s], hex->rec.data, hex->rec.length);
        if (s == 0)
            a = hex->rec.ulba + hex->rec.address;
        s += hex->rec.length;
        head_addr = a + s;
        hex->count += rc;
    }

    if (0 == rc)
        hex->eof_reached = true;

    hex->last_addr = head_addr;
    *addr = a;

    // return number of bytes copied to output buffer
    return s;
}

bool ihexi_is_eof(struct intel_hex_interpreter *hex)
{
    if (!hex) return false;
    return hex->eof_reached;
}

