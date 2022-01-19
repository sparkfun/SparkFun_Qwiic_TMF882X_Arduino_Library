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
#ifndef INTEL_HEX_INTERPRETER_H
#define INTEL_HEX_INTERPRETER_H

/** @file */

#include "tmf882x_host_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

/* intel hex defines ---------------------------------------------------------- */

/* supported intel hex record types: */
#define INTEL_HEX_TYPE_DATA                     0
#define INTEL_HEX_TYPE_EOF                      1
#define INTEL_HEX_TYPE_EXT_LIN_ADDR             4
#define INTEL_HEX_TYPE_START_LIN_ADDR           5


/* return codes: negative numbers are errors */
#define INTEL_HEX_EOF                   1       /* end of file -> reset */
#define INTEL_HEX_CONTINUE              0       /* continue reading in */
#define INTEL_HEX_ERR_NOT_A_NUMBER      -1
#define INTEL_HEX_ERR_TOO_SHORT         -2
#define INTEL_HEX_ERR_CRC_ERR           -3
#define INTEL_HEX_ERR_UNKNOWN_TYPE      -4
#define INTEL_HEX_WRITE_FAILED          -5


/* get the ULBA from a 32-bit address */
#define INTEL_HEX_ULBA( adr )         ( (adr) & 0xFFFF0000UL )

/* an intel hex record always has at least:
    :llaaaattcc
   l= length, a= address, t= type, c= crc
   so we need at least 11 ascii uint8_tacters for 1 regular record */
#define INTEL_HEX_MIN_RECORD_SIZE   11
/* the lineLength is at the beginning not the length but the last written
   address. To make out of this the length we substract instead
   of the min_record_size the min_last_address. */
#define INTEL_HEX_MIN_LAST_ADDRESS  ((INTEL_HEX_MIN_RECORD_SIZE) - 1 )

/* an intel hex record has a length field in that only 256 = an 8-bit number
 * can be represented, so we can limit the data buffer to half the size,
 * as it converts from ascii to binary */
#define INTEL_HEX_MAX_RECORD_DATA_SIZE  (128)

/*
 *****************************************************************************
 * FUNCTIONS
 *****************************************************************************
 */

/**
 * @struct intelRecord
 * @brief
 *      This structure represents a single intel hex record
 * @var intelRecord::ulba
 *      This member contains the unsigned linear base address (upper 16 bits)
 *      of the record
 * @var intelRecord::address
 *      This member contains the lower 16 bit address of the record
 * @var intelRecord::length
 *      This member contains the length of the data for this record
 * @var intelRecord::data
 *      This member contains the data for this record
 */
typedef struct _intelHexRecord
{
    uint32_t ulba;
    uint32_t address;
    uint32_t length;
    uint8_t data[ INTEL_HEX_MAX_RECORD_DATA_SIZE ];
} intelRecord;

/**
 * @struct intel_hex_interpreter
 * @brief
 *      This is the Base mode behavioral function pointer structure
 * @var intel_hex_interpreter::hex_records
 *      This member points to the string buffer containing all of the
 *      new-line separated hex records
 * @var intel_hex_interpreter::hex_size
 *      This member contains the size of the string in
 *      @ref intel_hex_interpreter::hex_records
 * @var intel_hex_interpreter::count
 *      This member tracks the position of the interpreter in the parser
 * @var intel_hex_interpreter::last_address
 *      This member tracks the last address used to keep the binary
 *      blocks contiguous
 * @var intel_hex_interpreter::rec
 *      This member is used as a temporary buffer for parsing individual records
 */
struct intel_hex_interpreter {
    const uint8_t * hex_records;
    uint32_t hex_size;
    uint32_t count;
    uint32_t last_addr;
    bool eof_reached;
    intelRecord rec;
};

/**
 *  @brief
 *       Initialize an intel hex interpreter
 *  @param[in] hex pointer to intel hex interpreter context structure
 *  @param[in] hex_records pointer to string of hex to decode
 *  @param[in] size size of hex_records buffer
 */
void ihexi_init(struct intel_hex_interpreter *hex, const uint8_t * hex_records,
                uint32_t size);

/**
 * @brief
 *      Parses an Intel Hex Record file records at a time and outputs binary data
 *      blobs.
 * @param[in] hex pointer to string of Intel Hex Records
 * @param[out] buf buffer to place data blob
 * @param[in] length size of buf
 * @param[out] addr address of data blob in buf
 * @return
 *      number of Bytes copied to buf, negative if error occurs, 0 indicates EOF
 */
int32_t ihexi_get_next_bin(struct intel_hex_interpreter *hex, uint8_t * buf,
                       uint32_t length, uint32_t * addr);

/**
 *  @brief
 *       Return whether EOF record was reached by the parser
 *  @param[in] hex pointer to intel hex interpreter context structure
 *  @note This function should always be called by the client
 *          to verify a complete set of hex records have been parsed
 *  @return
 *      bool Hex record(s) EOF status
 */
bool ihexi_is_eof(struct intel_hex_interpreter *hex);

#ifdef __cplusplus
}
#endif
#endif /* INTEL_HEX_INTERPRETER_H */

