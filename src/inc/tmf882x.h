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

#ifndef __TMF882X_H
#define __TMF882X_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Number of histogram bins */
#define TMF882X_HIST_NUM_BINS    256
/** @brief Number of TDCs */
#define TMF882X_HIST_NUM_TDC     5
/** @brief Number of Bytes per histogram bin */
#define TMF882X_BYTES_PER_BIN    4
/** @brief Maximum number of measurement result targets that can be reported at once */
#define TMF882X_MAX_MEAS_RESULTS 36
/** @brief Number of channels per TDC */
#define TMF882X_NUM_CH_PER_TDC   2
/** @brief Total Number of channels */
#define TMF882X_NUM_CH           ((TMF882X_HIST_NUM_TDC)*(TMF882X_NUM_CH_PER_TDC))
/* Max message size is a set of histograms + header info */
#ifdef CONFIG_TMF882X_NO_HISTOGRAM_SUPPORT
#define TMF882X_MAX_MSG_SIZE     (1024)
#else
/** @brief Maximum message size */
#define TMF882X_MAX_MSG_SIZE     (64 + (TMF882X_HIST_NUM_TDC * \
                                        TMF882X_BYTES_PER_BIN * \
                                        TMF882X_HIST_NUM_BINS))
#endif

/**
 * @enum tmf882x_msg_id
 * @brief Output message identifier codes
 */
enum tmf882x_msg_id {
  ID_MEAS_RESULTS    = 0x01,
  ID_MEAS_STATS      = 0x02,
  ID_HISTOGRAM       = 0x03,
  ID_ERROR           = 0x0F,
};

/**
 * @enum tmf882x_histogram_type
 * @brief Histogram message type identifier codes
 */
enum tmf882x_histogram_type {
  HIST_TYPE_RAW              = 0,
  HIST_TYPE_ELEC_CAL         = 1,
  TMF882X_NUM_HIST_TYPES
};

/**
 * @enum tmf882x_msg_error_codes
 * @brief Error message type identifier codes
 */
enum tmf882x_msg_error_codes {
  ERR_COMM            = 0xFD,
  ERR_RETRIES         = 0xFE,
  ERR_BUF_OVERFLOW    = 0xFF,
};

/* Output buffer is (HEADER_SIZE)+(DATA_SET_SIZE) to account for framing
 * before sending up the data out of the driver
 *
 *   *** Driver Output message stream (to-userspace) ***
 *   // Message 1
 *   {  Frame ID,
 *      Frame Length
 *      {  Frame_Data 0,
 *         Frame_Data 1,
 *         Frame_Data 2,
 *         .
 *         .
 *         .
 *         Frame_Data (size - 1)
 *      }
 *   }
 *   // Message 2
 *   {  Frame ID,
 *      Frame Length
 *      { Frame_Data 0,
 *        .
 *        .
 *        .
 *
 *   User space applications should cast the buffer to the appropriate
 *   structure based off the 'msg_id' see the example below:
 *
 *       // buffer must be at least 'TMF882X_MAX_MSG_SIZE' large
 *
 *       uint8_t buf[TMF882X_MAX_MSG_SIZE];
 *       size_t off = 0;
 *       ssize_t ret = 0;
 *       while (1) {
 *           ret = read(fd, buf + off, (sizeof(buf) - off));
 *           off = 0;
 *           do {
 *               struct tmf882x_msg *msg = buf + off;
 *               if (msg->hdr.msg_len > (ret - off)) {
 *                   // partial message, shift down and read more
 *                   memmove(buf, buf+off, (ret - off));
 *                   off = ret - off;
 *                   break;
 *               }
 *               off += msg->hdr.msg_len;
 *               switch (msg->hdr.msg_id) {
 *                   case ID_ERROR:
 *                      struct tmf882x_msg_error *err = &msg->err_msg;
 *                      *** Handle msg 'err' ***
 *                      break;
 *                   case .
 *                        .
 *                        .
 *               }
 *           } while (off < ret)
 *       }
 */

/**
 * @struct tmf882x_msg_header
 * @brief
 *      TMF882X message header type
 * @var tmf882x_msg_header::msg_id
 *      This member holds the message identifier code
 * @var tmf882x_msg_header::msg_len
 *      This member holds the message length (including the header)
 */
struct tmf882x_msg_header {
    uint32_t msg_id;
    uint32_t msg_len;
};
#define TMF882X_MSG_HEADER_SIZE  sizeof(struct tmf882x_msg_header)

/**
 * @struct tmf882x_msg_error
 * @brief TMF882X error message type.
 *      This error message is returned by the core driver for error such as
 *      communication errors.
 * @var tmf882x_msg_error::hdr
 *      This is the error message header @ref struct tmf882x_msg_header
 * @var tmf882x_msg_error::err_code
 *      This is the error code identifier
 */
struct tmf882x_msg_error {
    struct tmf882x_msg_header hdr;
    uint32_t err_code;
};

/**
 * @struct tmf882x_msg_histogram
 * @brief TMF882X histogram message type.
 *      This message is returned by the core driver for each set of historams
 *      that are received by the core driver from the device.
 * @var tmf882x_msg_histogram::hdr
 *      This is the message header @ref struct tmf882x_msg_header
 * @var tmf882x_msg_histogram::capture_num
 *      This is the capture number that this set of histograms is associated
 *      with. The capture_num will match the
 *      @ref struct tmf882x_msg_meas_results::result_num
 * @var tmf882x_msg_histogram::sub_capture
 *      This is the time-multiplexed sub-capture index of this set of
 *      histograms. For non-time-multiplexed measurements this value is always
 *      zero.
 * @var tmf882x_msg_histogram::histogram_type
 *      This is the histogram type identifier code
 *      @ref enum tmf882x_histogram_type
 * @var tmf882x_msg_histogram::num_tdc
 *      This is the number of TDC histograms being published
 * @var tmf882x_msg_histogram::num_bins
 *      This is the number of bins in the histograms being published
 * @var tmf882x_msg_histogram::bins
 *      These are the histogram bin values for each TDC. There are
 *      two channels per TDC, the first channel histogram occupies bins
 *      [0 : @ref TMF882X_HIST_NUM_BINS/2 - 1], the 2nd channel occupies bins
 *      [@ref TMF882X_HIST_NUM_BINS/2 : @ref TMF882X_HIST_NUM_BINS - 1]
 */
struct tmf882x_msg_histogram {
    struct tmf882x_msg_header hdr;
    uint32_t capture_num;       /* matches the value of 'result_num' from measure result messages*/
    uint32_t sub_capture;       /* sub-capture measurement nubmer for time multiplexed measurements*/
    uint32_t histogram_type;    /* RAW, ELEC_CAL, etc */
    uint32_t num_tdc;           /* Number of histogram channels in this message */
    uint32_t num_bins;          /* length of histogram(s) for each channel */
#ifdef CONFIG_TMF882X_NO_HISTOGRAM_SUPPORT
    uint32_t bins[1][1];
#else
    uint32_t bins[TMF882X_HIST_NUM_TDC][TMF882X_HIST_NUM_BINS];
#endif
};

/**
 * @struct tmf882x_meas_result
 * @brief TMF882X measure result
 *      This represents an individual target measurement result
 * @var tmf882x_meas_result::confidence
 *      This is the confidence level of the result reported
 * @var tmf882x_meas_result::distance_mm
 *      This is the distance reported in millimeters
 * @var tmf882x_meas_result::channel
 *      This is the channel that reported the target
 * @var tmf882x_meas_result::ch_target_idx
 *      This is the index of the target detected if the channel detected more
 *      than one target.
 * @var tmf882x_meas_result::sub_capture
 *      This is the time-multiplexed sub_capture index of the channel that
 *      detected the target. For non-time-multiplexed measurements this value
 *      is zero.
 */
struct tmf882x_meas_result {
    uint32_t confidence;    /*!< confidence level 0 .. no confidence, 0xFFFF .. highest confidence */
    uint32_t distance_mm;   /*!< distance in mm */
    uint32_t channel;       /*!< channel of result */
    uint32_t ch_target_idx; /*!< indicates target index in a given channel*/
    uint32_t sub_capture;   /*!< indicates which sub-capture of time-multiplexed measurement*/
};

/**
 * @struct tmf882x_msg_meas_results
 * @brief TMF882X measure results message type.
 *      This message is returned by the core driver for each set of measurement
 *      results that are received by the core driver from the device.
 * @var tmf882x_msg_meas_results::hdr
 *      This is the message header @ref struct tmf882x_msg_header
 * @var tmf882x_msg_meas_results::result_num
 *      This is the result number reported by the device
 * @var tmf882x_msg_meas_results::temperature
 *      This is the temperature reported by the device (in Celsius)
 * @var tmf882x_msg_meas_results::ambient_light
 *      This is the ambient light level reported by the device
 * @var tmf882x_msg_meas_results::photon_count
 *      This is the photon count reported by the device
 * @var tmf882x_msg_meas_results::ref_photon_count
 *      This is the reference channel photon count reported by the device
 * @var tmf882x_msg_meas_results::sys_ticks
 *      This is the system tick counter (5MHz counter) reported by the device.
 *      This is used by the core driver to perform clock compensation
 *      correction on the measurement results.
 * @var tmf882x_msg_meas_results::valid_results
 *      This is the number of targets reported by the device
 * @var tmf882x_msg_meas_results::num_results
 *      This is the number of non-zero targets counted by the core driver
 * @var tmf882x_msg_meas_results::results
 *      This is the list of measurement targets @ref struct tmf882x_meas_result
 */
struct tmf882x_msg_meas_results {
    struct tmf882x_msg_header hdr;
    uint32_t result_num;         /* increments with every new bunch of results */
    uint32_t temperature;        /* temperature */
    uint32_t ambient_light;      /* ambient light level */
    uint32_t photon_count;       /* photon count */
    uint32_t ref_photon_count;   /* reference photon count */
    uint32_t sys_ticks;          /* system ticks */
    uint32_t valid_results;      /* number of valid results */
    uint32_t num_results;        /* number of results */
    struct tmf882x_meas_result results[TMF882X_MAX_MEAS_RESULTS];
};

/**
 * @struct tmf882x_msg_meas_stats
 * @brief TMF882X measure statistics message type.
 *      This message is returned by the core driver for each set of measurement
 *      statistics that are received by the core driver from the device.
 * @var tmf882x_msg_meas_stats::hdr
 *      This is the message header @ref struct tmf882x_msg_header
 * @var tmf882x_msg_meas_stats::capture_num
 *      This is the capture number that this set of statistics is associated
 *      with. The capture_num will match the
 *      @ref struct tmf882x_msg_meas_results::result_num
 * @var tmf882x_msg_meas_stats::sub_capture
 *      This is the time-multiplexed sub_capture index of the channel that
 *      detected the target. For non-time-multiplexed measurements this value
 *      is zero.
 * @var tmf882x_msg_meas_stats::tdcif_status
 *      This is the tdcif status reported by the device
 * @var tmf882x_msg_meas_stats::iterations_configured
 *      This is the iterations configured reported by the device
 * @var tmf882x_msg_meas_stats::remaining_iterations
 *      This is the remaining iterations reported by the device
 * @var tmf882x_msg_meas_stats::accumulated_hits
 *      This is the accumulated hits reported by the device
 * @var tmf882x_msg_meas_stats::raw_hits
 *      This is the raw hits reported by the device for each TDC
 * @var tmf882x_msg_meas_stats::saturation_cnt
 *      This is the saturation count reported by the device for each TDC
 */
struct tmf882x_msg_meas_stats {
    struct tmf882x_msg_header hdr;
    uint32_t capture_num;       /* matches the value of 'result_num' from measure result messages*/
    uint32_t sub_capture;       /* sub-capture measurement nubmer for time multiplexed measurements*/
    uint32_t tdcif_status;
    uint32_t iterations_configured;
    uint32_t remaining_iterations;
    uint32_t accumulated_hits;
    uint32_t raw_hits[TMF882X_HIST_NUM_TDC];
    uint32_t saturation_cnt[TMF882X_HIST_NUM_TDC];
};

/**
 * @struct tmf882x_msg
 * @brief TMF882X message type.
 *      This is the global generic message type returned by the core driver
 * @var tmf882x_msg::hdr
 *      This is the message header @ref struct tmf882x_msg_header
 * @var tmf882x_msg::err_msg
 *      This is the error message @ref struct tmf882x_msg_error
 * @var tmf882x_msg::hist_msg
 *      This is the histogram message @ref struct tmf882x_msg_histogram
 * @var tmf882x_msg::meas_result_msg
 *      This is the results message @ref struct tmf882x_msg_meas_results
 * @var tmf882x_msg::meas_stat_msg
 *      This is the statistics message @ref struct tmf882x_msg_meas_stats
 * @var tmf882x_msg::msg_buf
 *      This is the low level buffer used to hold the message
 */
struct tmf882x_msg {
    union {
        struct tmf882x_msg_header       hdr;
        struct tmf882x_msg_error        err_msg;
        struct tmf882x_msg_histogram    hist_msg;
        struct tmf882x_msg_meas_results meas_result_msg;
        struct tmf882x_msg_meas_stats   meas_stat_msg;
        uint8_t msg_buf[TMF882X_MAX_MSG_SIZE];
    };
};

/* - convenience macros -
 * msg is always pointer to struct tmf882x_msg
 */
#define TOF_ZERO_MSG(msg) \
({ \
    struct tmf882x_msg *__m = (struct tmf882x_msg *)(msg); \
    memset(__m, 0, sizeof(struct tmf882x_msg)); \
})

#define TOF_INIT_MSG(msg) \
({ \
    struct tmf882x_msg *__m = (struct tmf882x_msg *)(msg); \
    memset(&__m->hdr, 0, TMF882X_MSG_HEADER_SIZE); \
})

#define TOF_SET_MSG_HDR(msg, id, type) \
({ \
    struct tmf882x_msg *__m = (struct tmf882x_msg *)(msg); \
    TOF_INIT_MSG(msg); \
    __m->hdr.msg_id = id; \
    __m->hdr.msg_len = sizeof(type); \
})

#define TOF_SET_ERR_MSG(msg, errid) \
({ \
    struct tmf882x_msg *__m = (struct tmf882x_msg *)(msg); \
    TOF_SET_MSG_HDR(msg, ID_ERROR, struct tmf882x_msg_error); \
    __m->err_msg.err_code = errid; \
 })

#define TOF_SET_HISTOGRAM_MSG(msg, hist_type) \
({ \
    struct tmf882x_msg *__m = (struct tmf882x_msg *)(msg); \
    TOF_SET_MSG_HDR(msg, ID_HISTOGRAM, struct tmf882x_msg_histogram); \
    __m->hist_msg.histogram_type = hist_type; \
 })

#ifdef __cplusplus
}
#endif
#endif /* __TMF882X_H */
