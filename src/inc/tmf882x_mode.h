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

/** @file tmf882x_mode.h
 *
 *  TMF882X generic mode interface
 */


#ifndef __TMF882X_MODE_H
#define __TMF882X_MODE_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief max number of i2c registers */
#define MAX_REGS (256)
/** @brief macro to return pointer to parent struct from an embedded struct*/
#define member_of(ptr, type, member) \
    ((type *) ((uint8_t *)(ptr) - offsetof(type, member)))


#define _IOCTL_NRBITS     8
#define _IOCTL_MODEBITS   2
#define _IOCTL_ISIZEBITS  10
#define _IOCTL_OSIZEBITS  10
#define _IOCTL_DIRBITS    2

#define _IOCTL_NRMASK     ((1 << _IOCTL_NRBITS)-1)
#define _IOCTL_MODEMASK   ((1 << _IOCTL_MODEBITS)-1)
#define _IOCTL_ISIZEMASK  ((1 << _IOCTL_ISIZEBITS)-1)
#define _IOCTL_OSIZEMASK  ((1 << _IOCTL_OSIZEBITS)-1)
#define _IOCTL_DIRMASK    ((1 << _IOCTL_DIRBITS)-1)

#define _IOCTL_NRSHIFT    0
#define _IOCTL_MODESHIFT  (_IOCTL_NRSHIFT+_IOCTL_NRBITS)
#define _IOCTL_ISIZESHIFT (_IOCTL_MODESHIFT+_IOCTL_MODEBITS)
#define _IOCTL_OSIZESHIFT (_IOCTL_ISIZESHIFT+_IOCTL_ISIZEBITS)
#define _IOCTL_DIRSHIFT   (_IOCTL_OSIZESHIFT+_IOCTL_OSIZEBITS)

// IOCTL is a 'read' (driver returns data)
#define _IOCTL_READ    2U
// IOCTL is a 'write' (driver accepts data)
#define _IOCTL_WRITE   1U
// IOCTL takes no parameters except command number
#define _IOCTL_NONE    0U

#define _IOCTL(mode,nr,dir,isize,osize) \
    (((dir) << _IOCTL_DIRSHIFT) | \
      ((mode) << _IOCTL_MODESHIFT) | \
      ((nr)   << _IOCTL_NRSHIFT) | \
      ((isize) << _IOCTL_ISIZESHIFT) | \
      ((osize) << _IOCTL_OSIZESHIFT))

#define _IOCTL_N(mode,nr)              _IOCTL((mode),(nr),_IOCTL_NONE,0,0)
#define _IOCTL_R(mode,nr,otype)        _IOCTL((mode),(nr),_IOCTL_READ,0,sizeof(otype))
#define _IOCTL_W(mode,nr,itype)        _IOCTL((mode),(nr),_IOCTL_WRITE,sizeof(itype),0)
#define _IOCTL_RW(mode,nr,itype,otype) _IOCTL((mode),(nr),_IOCTL_READ|_IOCTL_WRITE,sizeof(itype),sizeof(otype))
/* used to decode ioctl fields */
#define _IOCTL_MODE(nr)   (((nr) >> _IOCTL_MODESHIFT) & _IOCTL_MODEMASK)
#define _IOCTL_DIR(nr)    (((nr) >> _IOCTL_DIRSHIFT) & _IOCTL_DIRMASK)
#define _IOCTL_NR(nr)     (((nr) >> _IOCTL_NRSHIFT) & _IOCTL_NRMASK)
#define _IOCTL_ISIZE(nr)  (((nr) >> _IOCTL_ISIZESHIFT) & _IOCTL_ISIZEMASK)
#define _IOCTL_OSIZE(nr)  (((nr) >> _IOCTL_OSIZESHIFT) & _IOCTL_OSIZEMASK)

/**
 * @enum tmf882x_pwr_mode_t
 * @brief
 *      Indicate which power mode to switch to
 */
typedef enum tmf882x_pwr_mode_t {
    TOF_STANDBY = 0x00,
    TOF_WAKEUP  = 0x01,
} tmf882x_pwr_mode_t;

struct tmf882x_mode;
/**
 * @struct mode_vtable
 * @brief
 *      This is the Base mode behavioral function pointer structure
 * @var mode_vtable::tag
 *      This member is the mode type identifier
 * @var mode_vtable::open
 *      This member is the open method call for the current mode
 * @var mode_vtable::fwdl
 *      This member is the fwdl method call for the current mode
 * @var mode_vtable::mode_switch
 *      This member is the mode_switch method call for the current mode
 * @var mode_vtable::start
 *      This member is the start method call for the current mode
 * @var mode_vtable::stop
 *      This member is the stop method call for the current mode
 * @var mode_vtable::process_irq
 *      This member is the process_irq method call for the current mode
 * @var mode_vtable::close
 *      This member is the close method call for the current mode
 * @warning
 *      These function pointers should never be called directly, always
 *      use the @ref tmf882x_tof interface functions.
 */
struct mode_vtable {

    // Object identifier for error checking
    uint32_t tag;

    int32_t (*open) (struct tmf882x_mode *self);

    int32_t (*fwdl) (struct tmf882x_mode *self, int32_t fwdl_type,
                 const uint8_t *buf, size_t len);

    int32_t (*mode_switch) (struct tmf882x_mode *self, uint32_t mode);

    int32_t (*start) (struct tmf882x_mode *self);

    int32_t (*stop) (struct tmf882x_mode *self);

    int32_t (*process_irq) (struct tmf882x_mode *self);

    int32_t (*ioctl) (struct tmf882x_mode *self, uint32_t cmd,
                      const void *input, void *output);

    void (*close) (struct tmf882x_mode *self);

};

/**
 * @struct tmf882x_info_record
 * @brief
 *      This is the Base mode information record data
 * @var tmf882x_info_record::record
 *      This member is the record struct of the info record
 * @var tmf882x_info_record::data
 *      This member is the data buffer of the info record
 * @var tmf882x_info_record::record::app_id
 *      This member is the application ID of the current mode
 * @var tmf882x_info_record::record::min_ver
 *      This member is the minor version of the current mode
 * @var tmf882x_info_record::record::build_ver
 *      This member is the build version of the current mode
 * @var tmf882x_info_record::record::patch_ver
 *      This member is the patch version of the current mode
 */
struct decl_record{
		uint8_t app_id;
		uint8_t min_ver;
		uint8_t build_ver;
		uint8_t patch_ver;
		uint8_t reserved_4;
		uint8_t reserved_5;
		uint8_t reserved_6;
		uint8_t reserved_7;
};


struct tmf882x_info_record {
	union {
		struct decl_record record;
		uint8_t data[sizeof(struct decl_record)];
	};
};

/**
 * @struct tmf882x_mode
 * @brief
 *      This is the Base mode context structure
 * @var tmf882x_mode::ops
 *      This member is the @ref mode_vtable of the current mode
 * @var tmf882x_mode::info_rec
 *      This member is the @ref tmf882x_info_record of the current mode
 * @var tmf882x_mode::debug
 *      This member is the debug flag of the current mode
 * @var tmf882x_mode::buf
 *      This member is the scratch communication buffer of the current mode
 * @var tmf882x_mode::priv
 *      This member is the user-private context of the current mode
 */
struct tmf882x_mode {

    // ToF object virtual methods
    struct mode_vtable const * ops;

    // application mode info record
    struct tmf882x_info_record info_rec;

    // debug flag
    int32_t debug;

    // user private context
    void *priv;

};

/**
 * @brief
 *      initialize a @ref tmf882x_mode context structure
 * @param[in] self
 *      pointer to @ref tmf882x_mode context
 * @param[in] ops
 *      pointer to @ref mode_vtable structure to set mode behavior
 * @param[in] priv
 *      User-private context to pass back through callback functions
 */
extern void tmf882x_mode_init(struct tmf882x_mode *self,
                              struct mode_vtable const * ops,
                              void * priv);

/**
 * @brief
 *      Return this mode's private context pointer
 * @param[in] self
 *      pointer to @ref tmf882x_mode context
 * @return pointer to user-private context
 */
extern void * tmf882x_mode_priv(struct tmf882x_mode *self);

/**
 * @brief
 *      Return this mode's information record mode ID
 * @param[in] self
 *      pointer to @ref tmf882x_mode context
 * @return mode ID value
 */
extern uint8_t tmf882x_mode(struct tmf882x_mode *self);

/**
 * @brief
 *      Return this mode's major version number
 * @param[in] self
 *      pointer to @ref tmf882x_mode context
 * @return major version number
 */
extern uint8_t tmf882x_mode_maj_ver(struct tmf882x_mode *self);

/**
 * @brief
 *      Configure a chip poweron/standby operation
 * @param[in] self
 *      pointer to @ref tmf882x_mode context
 * @param[in] mode
 *      takes a @ref tmf882x_pwr_mode_t to configure the power mode
 * @return 0 for success, otherwise failure
 */
extern int32_t tmf882x_mode_standby_operation(struct tmf882x_mode *self, tmf882x_pwr_mode_t mode);

/**
 * @brief
 *      Set the powerup boot matrix of the device
 * @param[in] self
 *      pointer to @ref tmf882x_mode context
 * @param[in] powerup_bitfield
 *      Bitfield to use for powerup matrix
 *      - 0x0: use default bootup
 *      - 0x1: Force boot monitor (bootloader)
 *      - 0x2: Force boot application currently in RAM
 * @return 0 for success, otherwise failure
 */
extern int32_t tmf882x_mode_set_powerup_bootmatrix(struct tmf882x_mode *self,
                                                   uint32_t powerup_bitfield);

/**
 * @brief
 *      Perform a cpu reset
 * @param[in] self
 *      pointer to @ref tmf882x_mode context
 * @param[in] powerup_bitfield
 *      Bitfield to use for powerup matrix
 *      - 0x0: use default bootup
 *      - 0x1: Force boot monitor (bootloader)
 *      - 0x2: Force boot application currently in RAM
 * @return 0 for success, otherwise failure
 */
extern int32_t tmf882x_mode_cpu_reset(struct tmf882x_mode *self,
                                      uint32_t powerup_bitfield);

/**
 * @brief
 *      Set debug logging for this mode
 * @param[in] self
 *      pointer to @ref tmf882x_mode context
 * @param[in] flag
 *      non zero flag value enables debug logging, 0 disables debug logging
 */
extern void tmf882x_mode_set_debug(struct tmf882x_mode *self, int32_t flag);

/**
 * @brief
 *      Fill buffer with mode version string
 * @param[in] self
 *      pointer to @ref tmf882x_mode context
 * @param[in] ver
 *      Buffer to be filled with version string
 * @param[in] len
 *      length of buffer
 * @return
 *      number of characters copied to buffer
 */
extern int32_t tmf882x_mode_version(struct tmf882x_mode *self, char *ver, size_t len);

/**
 * @brief
 *      Log the i2c register map
 * @param[in] self
 *      pointer to @ref tmf882x_mode context
 */
extern void tmf882x_dump_i2c_regs(struct tmf882x_mode * self);

/**
 * @brief
 *      Log the data buffer
 * @param[in] self
 *      pointer to @ref tmf882x_mode context
 * @param[in] buf
 *      pointer to data buffer to log
 * @param[in] len
 *      length of data buffer
 */
extern void tmf882x_dump_data(struct tmf882x_mode *self, const uint8_t *buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif
