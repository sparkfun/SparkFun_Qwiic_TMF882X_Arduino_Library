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

/***** tmf882x_app.c *****/
#include "inc/tmf882x.h"
#include "inc/tmf882x_host_interface.h"
#include "inc/tmf882x_clock_correction.h"
#include "inc/tmf882x_mode_app_protocol.h"
#include "inc/tmf882x_mode_app_ioctl.h"
#include "inc/tmf882x_mode_app.h"
#include "tmf882x_interface.h"

#define TMF882X_APP_MODE_TAG          0x03U

#define NUM_8x8_CFG (4)
#define DEBUG_DUMP_HIST                 0
#define DEBUG_DUMP_CONFIG               1
#define DEBUG_DUMP_SPAD_CONFIG          1
#define CLK_CORR_ENABLE                 1
// ratio of host clk (1 MHz) to tof clk (5 MHz) is 5
#define TMF882X_SYSTICK_RATIO           5
#define TID_CHANGE_RETRIES              3
#define CMD_DEF_TIMEOUT_MS              30
#define CMD_FAC_CALIB_TIMEOUT_MS        4000
#define CMD_TIMEOUT_RETRIES             3
#define CMD_USLEEP_INCR                 10000
#define MS_TIME_TO_RETRIES(ms)          ((ms)*1000/(CMD_USLEEP_INCR))
#define BITS_IN_BYTE                    8
#define TMF882X_INT_MASK                0x7
#define RESULT_IDX_TO_CHANNEL(idx)     (((idx)%((TMF882X_HIST_NUM_TDC*2)-1)) + 1)
#define RESULT_IDX_TO_SUB_CAPTURE(idx) (((idx)/((TMF882X_HIST_NUM_TDC*2)-1)) % \
                                        TMF8X2X_MAX_CONFIGURATIONS)
#define APP_IS_CMD_BUSY(x)             ((x) >= TMF8X2X_COM_CMD_STAT__cmd_stat__CMD_MEASURE)
#define reg_to_idx(reg)                ((reg) - TMF8X2X_COM_CONFIG_RESULT - \
                                        TMF8X2X_COM_HEADER_SIZE)

// MSB of RID register is used to indicate a multi-packet response
#define APP_RESP_IS_MULTI_PACKET(RID) (RID & TMF8X2X_COM_OPTIONAL_SUBPACKET_HEADER_MASK)

#define ARR_SIZE(arr)  (sizeof(arr)/sizeof(arr[0]))
#define tof_app_dbg(app, fmt, ...) \
({ \
    struct tmf882x_mode_app *__app = (app); \
    if(__app->mode.debug) \
        tof_info(priv(app), fmt, ##__VA_ARGS__); \
})
#define verify_mode(mode) \
({ \
    struct tmf882x_mode *__mode = mode; \
    (TMF882X_APP_MODE_TAG == __mode->ops->tag); \
})

enum tmf882x_irq_flags {
    F_RESULT_IRQ       = 0x2,
    F_ALT_RESULT_IRQ   = 0x4,
    F_RAW_HIST_IRQ     = 0x8,
    F_BRKPT_IRQ        = 0x10,
    F_CMD_DONE_IRQ     = 0x20,
    F_ERROR_IRQ        = 0x40,
    F_IRQ_ALL          = F_RESULT_IRQ   | F_ALT_RESULT_IRQ |
                         F_RAW_HIST_IRQ | F_BRKPT_IRQ      |
                         F_CMD_DONE_IRQ | F_ERROR_IRQ ,
};

static int32_t tmf882x_mode_app_open(struct tmf882x_mode *self);

static void *app_memmove(void *dest, const void *source, size_t cnt)
{
    uint8_t *dst = (uint8_t *)dest;
    uint8_t *src = (uint8_t *)source;
    if (dst && src && cnt && (dst != src)) {
        if ((src > dst + cnt) || (dst > src + cnt)) {
            memcpy(dst, src, cnt);
        } else if (dst < src) {
            do { *dst++ = *src++; } while (--cnt);
        } else {
            dst += cnt;
            src += cnt;
            do {*--dst = *--src;} while (--cnt);
        }
    }
    return dest;
}

static uint8_t * encode_8b(uint8_t *buf, uint8_t val)
{
    if (!buf) return buf;
    *buf++ = val;
    return buf;
}

static const uint8_t * decode_8b(const uint8_t *buf, uint8_t *val)
{
    uint8_t t;
    if (!buf) return buf;
    t = *buf++;
    if (val) *val = t;
    return buf;
}

static uint8_t * encode_16b(uint8_t *buf, uint16_t val)
{
    if (!buf) return buf;
    *buf++ = val & 0xFF;
    *buf++ = (val >> 8) & 0xFF;
    return buf;
}

static const uint8_t * decode_16b(const uint8_t *buf, uint16_t *val)
{
    uint16_t t;
    if (!buf) return buf;
    t = *buf++;
    t += (*buf++ << 8);
    if (val) *val = t;
    return buf;
}

static uint8_t * encode_24b(uint8_t *buf, uint32_t val)
{
    if (!buf) return buf;
    *buf++ = val & 0xFF;
    *buf++ = (val >> 8) & 0xFF;
    *buf++ = (val >> 16) & 0xFF;
    return buf;
}

static const uint8_t * decode_24b(const uint8_t *buf, uint32_t *val)
{
    uint32_t t;
    if (!buf) return buf;
    t = *buf++;
    t += (*buf++ << 8);
    t += (*buf++ << 16);
    if (val) *val = t;
    return buf;
}

static uint8_t * encode_32b(uint8_t *buf, uint32_t val)
{
    if (!buf) return buf;
    *buf++ = val & 0xFF;
    *buf++ = (val >> 8) & 0xFF;
    *buf++ = (val >> 16) & 0xFF;
    *buf++ = (val >> 24) & 0xFF;
    return buf;
}

static const uint8_t * decode_32b(const uint8_t *buf, uint32_t *val)
{
    uint32_t t;
    if (!buf) return buf;
    t = *buf++;
    t += (*buf << 8);
    buf++;
    t += (*buf << 16);
    buf++;
    t += (*buf << 24);
    buf++;
    if (val) *val = t;
    return buf;
}

static inline struct tmf882x_mode *to_parent(struct tmf882x_mode_app *app)
{
    return &app->mode;
}

static inline struct tmf882x_msg * to_msg(struct tmf882x_mode_app *app)
{
    return &app->volat_data.msg;
}

static inline struct tmf882x_mode_app_i2c_msg * to_i2cmsg(struct tmf882x_mode_app *app)
{
    return &app->volat_data.i2c_msg;
}

static inline void * priv(struct tmf882x_mode_app *app)
{
    return tmf882x_mode_priv(to_parent(app));
}

static inline bool driver_compatible_with_app(struct tmf882x_mode *self)
{
    return ((TMF882X_MAJ_MODULE_VER) == (tmf882x_mode_maj_ver(self)));
}

static inline bool is_measuring(struct tmf882x_mode_app *app)
{
    return app->volat_data.is_measuring;
}

static uint8_t * encode_i2c_msg_header(const struct tmf882x_mode_app_i2c_msg *msg,
                                       uint8_t *buf, size_t len)
{
    if (msg && buf && (len >= TMF8X2X_COM_HEADER_SIZE)) {
        /*
         * Do not encode RID for sending messages, assume the proper page
         *  is already loaded in the device i2c register map
         *  */
        buf = encode_8b(buf, msg->tid);
        buf = encode_16b(buf, msg->size);
    }
    return buf;
}

static const uint8_t * decode_i2c_msg_header(struct tmf882x_mode_app_i2c_msg *msg,
                                             const uint8_t *buf, size_t len)
{
    uint16_t msg_size = 0;
    if (msg && buf && (len >= TMF8X2X_COM_HEADER_SIZE)) {

        // assume first packet in message
        msg->pckt_num = 0;
        msg->cfg_id = 0;

        /*** decode base header ***/
        buf = decode_8b(buf, &msg->rid);
        buf = decode_8b(buf, &msg->tid);
        buf = decode_16b(buf, &msg_size);

        if(APP_RESP_IS_MULTI_PACKET(msg->rid) &&
           (len >= TMF8X2X_COM_HEADER_SIZE +
                   TMF8X2X_COM_OPTIONAL_SUBPACKET_HEADER_SIZE)) {

            /*** decode subpacket header ***/
            buf = decode_8b(buf, &msg->pckt_num);
            buf = decode_8b(buf, &msg->pckt_size);
            buf = decode_8b(buf, &msg->cfg_id);
        }

        if (msg->pckt_num == 0) {
            // only take total message size if this is the first packet
            msg->size = msg_size;
        }
    }
    return buf;
}

static void dump_config(struct tmf882x_mode_app *app,
                        const struct tmf882x_mode_app_config *cfg)
{
    if (!app || !cfg) return;
    tof_info(priv(app), "Application Common Config --");
    tof_info(priv(app), "  report_period_ms: %u", cfg->report_period_ms);
    tof_info(priv(app), "  iterations: %u", (cfg->kilo_iterations) << 10);
    tof_info(priv(app), "  low_threshold: %u", cfg->low_threshold);
    tof_info(priv(app), "  high_threshold: %u", cfg->high_threshold);
    tof_info(priv(app), "  zone_mask: %#x", cfg->zone_mask);
    tof_info(priv(app), "  persistence: %u", cfg->persistence);
    tof_info(priv(app), "  confidence_threshold: %u", cfg->confidence_threshold);
    tof_info(priv(app), "  gpio_0: %#x", cfg->gpio_0);
    tof_info(priv(app), "  gpio_1: %#x", cfg->gpio_1);
    tof_info(priv(app), "  power_cfg: %#x", cfg->power_cfg);
    tof_info(priv(app), "  spad_map_id: %u", cfg->spad_map_id);
    tof_info(priv(app), "  alg_setting: %#x", cfg->alg_setting);
    tof_info(priv(app), "  histogram_dump: %#x", cfg->histogram_dump);
    tof_info(priv(app), "  spread_spectrum: %#x", cfg->spread_spectrum);
    tof_info(priv(app), "  i2c_slave_addr: %#x", cfg->i2c_slave_addr);
    tof_info(priv(app), "  oscillator_trim: %#x", cfg->oscillator_trim);
}

static void dump_spad_config(struct tmf882x_mode_app *app,
                             const struct tmf882x_mode_app_single_spad_config *spad_cfg)
{
    uint32_t y, x;
    uint8_t spad_map_str[64];
    uint32_t t;
    uint32_t ysize, xsize;
    if (!app || !spad_cfg) return;
    tof_info(priv(app), "Application Spad Config --");
    tof_info(priv(app), "  xoff_q1: %d", spad_cfg->xoff_q1);
    tof_info(priv(app), "  yoff_q1: %d", spad_cfg->yoff_q1);
    tof_info(priv(app), "  xsize  : %u", spad_cfg->xsize);
    tof_info(priv(app), "  ysize  : %u", spad_cfg->ysize);
    ysize =
        spad_cfg->ysize > TMF8X2X_COM_MAX_SPAD_YSIZE ?
        TMF8X2X_COM_MAX_SPAD_YSIZE : spad_cfg->ysize;
    xsize =
        spad_cfg->xsize > TMF8X2X_COM_MAX_SPAD_XSIZE ?
        TMF8X2X_COM_MAX_SPAD_XSIZE : spad_cfg->xsize;
    tof_info(priv(app), "  spad_mask:");
    for (y = 0; y < ysize; ++y) {
        for (x = 0, t = 0; x < xsize; ++x)
            t += snprintf((char *)spad_map_str + t, sizeof(spad_map_str) - t,
                          "%u ", !!(spad_cfg->spad_mask[y*xsize + x]));
        tof_info(priv(app), "    [%u]: %s", ysize - y - 1, spad_map_str);
    }
    tof_info(priv(app), "  spad_map:");
    for (y = 0; y < ysize; ++y) {
        for (x = 0, t = 0; x < xsize; ++x)
            t += snprintf((char *)spad_map_str + t, sizeof(spad_map_str) - t,
                          "%u ", spad_cfg->spad_map[y*xsize + x]);
        tof_info(priv(app), "    [%u]: %s", ysize - y -1, spad_map_str);
    }
}

static bool tmf882x_mode_app_is_8x8_mode(struct tmf882x_mode_app *app)
{
    int32_t error = 0;
    uint8_t reg = 0;

    if (!app) return false;

    error = tof_get_register(priv(app), TMF8X2X_COM_MODE, &reg);
    if (error) {
        tof_err(priv(app), "Error reading 8x8 mode status register (%d)", error);
        return false;
    }

    return (reg & TMF8X2X_COM_MODE__mode__MASK);
}

static int32_t tof_clear_irq(struct tmf882x_mode_app *app)
{
    int32_t rc = 0;
    uint8_t int_stat;

    rc = tof_get_register(priv(app),
                          TMF882X_INT_STAT,
                          &int_stat);
    if (rc) {
        tof_err(priv(app), "Error reading INTSTAT");
        return -1;
    }

    if (!int_stat) return 0;

    //Clear interrupt
    rc = tof_set_register(priv(app),
                          TMF882X_INT_STAT,
                          int_stat);
    if (rc) {
        tof_err(priv(app), "Error writing INTSTAT");
        return -1;
    }

    return int_stat;
}

static int32_t wait_for_tid_change(struct tmf882x_mode_app *app)
{
    struct tmf882x_mode_app_i2c_msg *i2c_msg;
    uint32_t num_retries;
    uint8_t tid = 0;

    i2c_msg = to_i2cmsg(app);

    num_retries = TID_CHANGE_RETRIES;

    do {
        num_retries -= 1;
        if (tof_get_register(priv(app), TMF8X2X_COM_TID, &tid)) {
            return -1;
        }
        if (i2c_msg->tid == tid) {
            /* TID has not changed*/
            if (num_retries == 0) {
                tof_app_dbg(app, "app prev TID: %#x curr TID: %#x",
                            i2c_msg->tid, tid);
                return -1;
            }
            tof_usleep(priv(app), 1000);
            continue;
        }
        /* if we have reached here, the command has completed*/
        break;
    } while (num_retries > 0);

    // cache the newest tid
    i2c_msg->tid = tid;
    return 0;
}

static int32_t check_app_cmd_status(struct tmf882x_mode_app *app, uint8_t status)
{
    if( (status != TMF8X2X_COM_CMD_STAT__cmd_stat__STAT_OK) &&
        (status != TMF8X2X_COM_CMD_STAT__cmd_stat__STAT_ACCEPTED) ) {
        tof_err(priv(app), "Error app cmd status: \'%#x\'", status);
        return -1;
    }
    return 0;
}

static uint8_t get_app_cmd_stat(struct tmf882x_mode_app *app)
{
    uint8_t status = 0;
    if (tof_get_register(priv(app), TMF8X2X_COM_CMD_STAT, &status)) {
        tof_err(priv(app), "Error reading app cmd status");
        return -1;
    }
    return status;
}

static int32_t check_cmd_status(struct tmf882x_mode_app *app,
                            int32_t retries)
{
    int32_t num_retries;
    uint8_t status;

    if (retries < 0)
        retries = 5;

    num_retries = retries;

    // initial wait for any command
    tof_usleep(priv(app), 1000);

    do {
        num_retries -= 1;
        status = get_app_cmd_stat(app);
        if (APP_IS_CMD_BUSY(status)) {
            /* CMD is still executing, wait and retry */
            tof_usleep(priv(app), CMD_USLEEP_INCR);
            continue;
        }
        /* if we have reached here, the command has completed*/
        break;
    } while (num_retries > 0);
    return check_app_cmd_status(app, status);
}

static int32_t check_cmd_status_timeout(struct tmf882x_mode_app *app,
                                    uint32_t timeout_ms)
{
    uint32_t wait_ms = 0;

    // Wait part of the timeout before checking cmd status
    if (timeout_ms) {
        wait_ms = timeout_ms >> 3; // initial wait fraction 1/8
        tof_usleep(priv(app), wait_ms * 1000);
    }
    return check_cmd_status(app, MS_TIME_TO_RETRIES(timeout_ms - wait_ms));
}

static inline uint32_t timespec_to_usec(struct timespec ts)
{
    return ((ts.tv_sec * 1000000) + ((ts.tv_nsec + 500)/ 1000));
}

static int32_t clock_skew_correction(struct tmf882x_mode_app *app,
                                 struct tmf882x_msg_meas_results *results)
{
    // Assume timespec is defined by platform include files
    struct timespec current_ts = {0};
    uint32_t usec_epoch = 0;
    uint32_t cr_dist = 0;
    uint32_t i = 0;

    tof_get_timespec(&current_ts);
    if ( (current_ts.tv_sec - app->volat_data.timestamp.tv_sec) >= 60 ) {
        // Reset our clock correction averaging every minute so we can still
        //  be responsive to clock drift in the device
        app->volat_data.timestamp = current_ts;
        tmf882x_clk_corr_recalc(&app->volat_data.clk_cr);
    }

    usec_epoch = timespec_to_usec(current_ts);
    // sys tick timestamp must have LSB set to be valid
    if (results->sys_ticks & 0x1) {
        tmf882x_clk_corr_addpair(&app->volat_data.clk_cr, usec_epoch, results->sys_ticks);
    }

    tof_app_dbg(app, "clock skew host_ts: %u (usec) dev_ts: %u (sys_ticks) "
                "iratioQ15: %u", usec_epoch, results->sys_ticks,
                app->volat_data.clk_cr.iratioQ15);

    if (CLK_CORR_ENABLE && app->volat_data.clk_corr_enabled) {
        for (i = 0; i < results->num_results; ++i) {
            cr_dist = tmf882x_clk_corr_map(&app->volat_data.clk_cr,
                                           results->results[i].distance_mm);
            tof_app_dbg(app, "clock compensation ch: %u subcapture: %u "
                        "old_dist: %u new_dist: %u",
                        results->results[i].channel,
                        results->results[i].sub_capture,
                        results->results[i].distance_mm, cr_dist);
            results->results[i].distance_mm = cr_dist;
        }
    }

    return 0;
}

static int32_t publish_measure_results(struct tmf882x_mode_app *app,
                                   struct tmf882x_msg_meas_results *results)
{
    // perform clock correction on results before publishing
    (void) clock_skew_correction(app, results);

    // fire away
    return tof_queue_msg(priv(app), to_msg(app));
}

static int32_t decode_result_msg(struct tmf882x_mode_app *app,
                                 const struct tmf882x_mode_app_i2c_msg *i2c_msg)
{
    uint32_t i = 0, j = 0;
    struct tmf882x_msg_meas_results *result_msg = &(to_msg(app)->meas_result_msg);
    const uint8_t *head = i2c_msg->buf;
    const uint8_t *tail = NULL;
    uint8_t confidence = 0;
    uint16_t distance_mm = 0;
    uint32_t obj_cnt = 0;
    uint32_t ch_target_idx = 0;
    int32_t extra_data = 0;

    //initialize output msg
    TOF_ZERO_MSG(result_msg);
    TOF_SET_MSG_HDR(result_msg, ID_MEAS_RESULTS, struct tmf882x_msg_meas_results);

    // Decode result Header
    decode_8b(&head[reg_to_idx(TMF8X2X_COM_RESULT_NUMBER)],
              (uint8_t *)&result_msg->result_num);
    decode_8b(&head[reg_to_idx(TMF8X2X_COM_TEMPERATURE)],
              (uint8_t *)&result_msg->temperature);
    decode_8b(&head[reg_to_idx(TMF8X2X_COM_NUMBER_VALID_RESULTS)],
              (uint8_t *)&result_msg->valid_results);
    decode_32b(&head[reg_to_idx(TMF8X2X_COM_AMBIENT_LIGHT_0)],
               &result_msg->ambient_light);
    decode_32b(&head[reg_to_idx(TMF8X2X_COM_PHOTON_COUNT_0)],
               &result_msg->photon_count);
    decode_32b(&head[reg_to_idx(TMF8X2X_COM_REFERENCE_COUNT_0)],
               &result_msg->ref_photon_count);
    decode_32b(&head[reg_to_idx(TMF8X2X_COM_SYS_TICK_0)], &result_msg->sys_ticks);

    // start of object result list
    for (i = 0, tail = &head[reg_to_idx(TMF8X2X_COM_RES_CONFIDENCE_0)], obj_cnt = 0;
         i < TMF8X2X_COM_MAX_MEASUREMENT_RESULTS; ++i) {

        tail = decode_8b(tail, &confidence);
        tail = decode_16b(tail, &distance_mm);

        if (confidence != 0 || distance_mm != 0) {
            // object detected, add it to the result message

            // Find how many targets are in this channel already
            for (j = 0, ch_target_idx = 0; j < obj_cnt; ++j)
                if (result_msg->results[j].channel == RESULT_IDX_TO_CHANNEL(i) &&
                    result_msg->results[j].sub_capture == RESULT_IDX_TO_SUB_CAPTURE(i))
                    ch_target_idx++;

            result_msg->results[obj_cnt].confidence = confidence;
            result_msg->results[obj_cnt].distance_mm = distance_mm;
            result_msg->results[obj_cnt].channel = RESULT_IDX_TO_CHANNEL(i);
            result_msg->results[obj_cnt].sub_capture = RESULT_IDX_TO_SUB_CAPTURE(i);
            result_msg->results[obj_cnt].ch_target_idx = ch_target_idx;
            obj_cnt++;
        }
    }

    result_msg->num_results = obj_cnt;
    if (obj_cnt != result_msg->valid_results) {
        tof_info(priv(app), "Warning num objects (%u) != valid results (%u)",
                 obj_cnt, result_msg->valid_results);
    }

    // extra data in result message? log it
    if(app->mode.debug && (tail - head) < i2c_msg->size) {
        extra_data = ((int32_t) i2c_msg->size) - (tail - head);
        tof_app_dbg(app, "Extra result data size: %u", extra_data);
        tmf882x_dump_data(to_parent(app), tail, extra_data);
    }

    // Try to keep up with which measurement iteration we are on, 256 rollover
    app->volat_data.capture_num = result_msg->result_num + 1;
    if (app->volat_data.capture_num == 256)
        app->volat_data.capture_num = 0;

    return publish_measure_results(app, result_msg);
}

static int32_t decode_meas_stats_msg(struct tmf882x_mode_app *app,
                                 const struct tmf882x_mode_app_i2c_msg *i2c_msg)
{
    uint32_t i;
    struct tmf882x_msg_meas_stats *stat_msg = &(to_msg(app)->meas_stat_msg);
    const uint8_t *head = i2c_msg->buf;
    const uint8_t *tail;

    //initialize output msg
    TOF_ZERO_MSG(stat_msg);
    TOF_SET_MSG_HDR(stat_msg, ID_MEAS_STATS, struct tmf882x_msg_meas_stats);

    // This tag *should* match the 'result_num' from the next measurement
    // result data
    stat_msg->capture_num = app->volat_data.capture_num;
    //fill out sub-capture index field
    decode_8b(&head[reg_to_idx(TMF8X2X_COM_STATISTICS_CFG_IDX)],
              (uint8_t *)&stat_msg->sub_capture);
    decode_32b(&head[reg_to_idx(TMF8X2X_COM_TDCIF_STATUS)],
               &stat_msg->tdcif_status);
    decode_32b(&head[reg_to_idx(TMF8X2X_COM_ITERATIONS_CONFIGURED)],
               &stat_msg->iterations_configured);
    decode_32b(&head[reg_to_idx(TMF8X2X_COM_REMAINING_ITERATIONS)],
               &stat_msg->remaining_iterations);
    decode_32b(&head[reg_to_idx(TMF8X2X_COM_ACCUMULATED_HITS)],
               &stat_msg->accumulated_hits);

    // Iterate over per-TDC stats
    for (i = 0, tail = &head[reg_to_idx(TMF8X2X_COM_RAW_HITS_TDC0)];
         i < TMF882X_HIST_NUM_TDC; ++i) {
        tail = decode_32b(tail, &stat_msg->raw_hits[i]);
    }

    for (i = 0, tail = &head[reg_to_idx(TMF8X2X_COM_SATURATION_CNT_TDC0)];
         i < TMF882X_HIST_NUM_TDC; ++i) {
        tail = decode_32b(tail, &stat_msg->saturation_cnt[i]);
    }

    // publish statistic data
    return tof_queue_msg(priv(app), to_msg(app));
}

static int32_t encode_config_msg(struct tmf882x_mode_app *app,
                             struct tmf882x_mode_app_i2c_msg *i2c_msg,
                             const struct tmf882x_mode_app_config *config)
{
    uint8_t *head = i2c_msg->buf;

    if (!app || !i2c_msg || !config) return -1;

    encode_16b(&head[reg_to_idx(TMF8X2X_COM_PERIOD_MS_LSB)], config->report_period_ms);
    encode_16b(&head[reg_to_idx(TMF8X2X_COM_KILO_ITERATIONS_LSB)], config->kilo_iterations);
    encode_16b(&head[reg_to_idx(TMF8X2X_COM_INT_THRESHOLD_LOW_LSB)], config->low_threshold);
    encode_16b(&head[reg_to_idx(TMF8X2X_COM_INT_THRESHOLD_HIGH_LSB)], config->high_threshold);
    encode_24b(&head[reg_to_idx(TMF8X2X_COM_INT_ZONE_MASK_0)], config->zone_mask);
    encode_8b(&head[reg_to_idx(TMF8X2X_COM_INT_PERSISTENCE)], config->persistence);
    encode_8b(&head[reg_to_idx(TMF8X2X_COM_CONFIDENCE_THRESHOLD)], config->confidence_threshold);
    encode_8b(&head[reg_to_idx(TMF8X2X_COM_GPIO_0)], config->gpio_0);
    encode_8b(&head[reg_to_idx(TMF8X2X_COM_GPIO_1)], config->gpio_1);
    encode_8b(&head[reg_to_idx(TMF8X2X_COM_POWER_CFG)], config->power_cfg);
    encode_8b(&head[reg_to_idx(TMF8X2X_COM_SPAD_MAP_ID)], config->spad_map_id);
    encode_32b(&head[reg_to_idx(TMF8X2X_COM_ALG_SETTING_0)], config->alg_setting);
    encode_8b(&head[reg_to_idx(TMF8X2X_COM_HIST_DUMP)], config->histogram_dump);
    encode_8b(&head[reg_to_idx(TMF8X2X_COM_SPREAD_SPECTRUM)], config->spread_spectrum);
    encode_8b(&head[reg_to_idx(TMF8X2X_COM_I2C_SLAVE_ADDRESS)],
              (config->i2c_slave_addr << TMF8X2X_COM_I2C_SLAVE_ADDRESS__7bit_slave_address__SHIFT));
    encode_16b(&head[reg_to_idx(TMF8X2X_COM_OSC_TRIM_VALUE_LSB)], config->oscillator_trim);
    return 0;
}

static int32_t decode_config_msg(struct tmf882x_mode_app *app,
                             const struct tmf882x_mode_app_i2c_msg *i2c_msg,
                             struct tmf882x_mode_app_config *config)
{
    const uint8_t *head = i2c_msg->buf;

    if (!app || !i2c_msg || !config) return -1;

    decode_16b(&head[reg_to_idx(TMF8X2X_COM_PERIOD_MS_LSB)], &config->report_period_ms);
    decode_16b(&head[reg_to_idx(TMF8X2X_COM_KILO_ITERATIONS_LSB)], &config->kilo_iterations);
    decode_16b(&head[reg_to_idx(TMF8X2X_COM_INT_THRESHOLD_LOW_LSB)], &config->low_threshold);
    decode_16b(&head[reg_to_idx(TMF8X2X_COM_INT_THRESHOLD_HIGH_LSB)], &config->high_threshold);
    decode_24b(&head[reg_to_idx(TMF8X2X_COM_INT_ZONE_MASK_0)], &config->zone_mask);
    decode_8b(&head[reg_to_idx(TMF8X2X_COM_INT_PERSISTENCE)], &config->persistence);
    decode_8b(&head[reg_to_idx(TMF8X2X_COM_CONFIDENCE_THRESHOLD)], &config->confidence_threshold);
    decode_8b(&head[reg_to_idx(TMF8X2X_COM_GPIO_0)], &config->gpio_0);
    decode_8b(&head[reg_to_idx(TMF8X2X_COM_GPIO_1)], &config->gpio_1);
    decode_8b(&head[reg_to_idx(TMF8X2X_COM_POWER_CFG)], &config->power_cfg);
    decode_8b(&head[reg_to_idx(TMF8X2X_COM_SPAD_MAP_ID)], &config->spad_map_id);
    decode_32b(&head[reg_to_idx(TMF8X2X_COM_ALG_SETTING_0)], &config->alg_setting);
    decode_8b(&head[reg_to_idx(TMF8X2X_COM_HIST_DUMP)], &config->histogram_dump);
    decode_8b(&head[reg_to_idx(TMF8X2X_COM_SPREAD_SPECTRUM)], &config->spread_spectrum);
    decode_8b(&head[reg_to_idx(TMF8X2X_COM_I2C_SLAVE_ADDRESS)], &config->i2c_slave_addr);
    config->i2c_slave_addr >>= TMF8X2X_COM_I2C_SLAVE_ADDRESS__7bit_slave_address__SHIFT;
    decode_16b(&head[reg_to_idx(TMF8X2X_COM_OSC_TRIM_VALUE_LSB)], &config->oscillator_trim);
    return 0;
}

static int32_t encode_spad_config_msg(struct tmf882x_mode_app *app,
                                  struct tmf882x_mode_app_i2c_msg *i2c_msg,
                                  const struct tmf882x_mode_app_single_spad_config *spad_cfg)
{
    int32_t x, y, yIdx;
    uint32_t ch_select = 0;
    uint32_t ch_map = 0;
    uint32_t ch;
    uint32_t mask;
    uint8_t *head = i2c_msg->buf;
    uint8_t *tail;

    if (!app || !i2c_msg || !spad_cfg) return -1;

    // encode spad enable mask
    for (y = spad_cfg->ysize - 1, yIdx = 0,
         tail = &head[reg_to_idx(TMF8X2X_COM_SPAD_ENABLE_SPAD0_0)];
         y >= 0; --y, ++yIdx) {
        mask = 0;
        for (x = 0; x < spad_cfg->xsize; ++x) {
            mask |= (!!spad_cfg->spad_mask[y*spad_cfg->xsize + x] << x);
        }
        tail = encode_24b(tail, mask);
    }

    // encode spad map
    for (x = 0, tail = &head[reg_to_idx(TMF8X2X_COM_SPAD_TDC_CHANNEL0_0)];
         x < spad_cfg->xsize; ++x)
    {
        ch_map = 0;
        for (y = spad_cfg->ysize - 1, yIdx = 0; y >= 0; --y, ++yIdx) {
            ch = spad_cfg->spad_map[y*spad_cfg->xsize + x];
            if (ch == 8) {
                ch = 0;
                ch_select |= (1 << (spad_cfg->ysize - 1 - y));
            }
            else if (ch == 9) {
                ch = 1;
                ch_select |= (1 << (spad_cfg->ysize - 1 - y));
            }
            ch = TMF8X2X_MAIN_SPAD_ENCODE_CHANNEL( ch, yIdx );
            ch_map |= ch;
        }
        tail = encode_32b(tail, ch_map);
    }

    // encode channel select mask
    encode_24b(&head[reg_to_idx(TMF8X2X_COM_SPAD_TDC_CHANNEL_SELECT_0)], ch_select);
    // encode offsets
    encode_8b(&head[reg_to_idx(TMF8X2X_COM_SPAD_X_OFFSET_2)], spad_cfg->xoff_q1);
    encode_8b(&head[reg_to_idx(TMF8X2X_COM_SPAD_Y_OFFSET_2)], spad_cfg->yoff_q1);
    // encode size
    encode_8b(&head[reg_to_idx(TMF8X2X_COM_SPAD_X_SIZE)], spad_cfg->xsize);
    encode_8b(&head[reg_to_idx(TMF8X2X_COM_SPAD_Y_SIZE)], spad_cfg->ysize);
    return 0;
}

static int32_t decode_spad_config_msg(struct tmf882x_mode_app *app,
                                  const struct tmf882x_mode_app_i2c_msg *i2c_msg,
                                  struct tmf882x_mode_app_single_spad_config *spad_cfg)
{
    int32_t x, y, yIdx;
    uint32_t ch_select;
    uint32_t ch_map;
    uint32_t ch;
    uint32_t mask;
    const uint8_t *head = i2c_msg->buf;
    const uint8_t *tail;

    if (!app || !i2c_msg || !spad_cfg) return -1;

    // decode offsets
    decode_8b(&head[reg_to_idx(TMF8X2X_COM_SPAD_X_OFFSET_2)],
              (uint8_t *)&spad_cfg->xoff_q1);
    decode_8b(&head[reg_to_idx(TMF8X2X_COM_SPAD_Y_OFFSET_2)],
              (uint8_t *)&spad_cfg->yoff_q1);
    // decode size
    decode_8b(&head[reg_to_idx(TMF8X2X_COM_SPAD_X_SIZE)], &spad_cfg->xsize);
    decode_8b(&head[reg_to_idx(TMF8X2X_COM_SPAD_Y_SIZE)], &spad_cfg->ysize);

    // decode spad enable mask
    for (y = spad_cfg->ysize - 1,
         tail = &head[reg_to_idx(TMF8X2X_COM_SPAD_ENABLE_SPAD0_0)]; y >= 0; --y)
    {
        tail = decode_24b(tail, &mask);
        for (x = 0; x < spad_cfg->xsize; ++x)
        {
            spad_cfg->spad_mask[y*spad_cfg->xsize + x] =
                (mask >> x) & 0x01;
        }
    }

    // decode spad map
    decode_24b(&head[reg_to_idx(TMF8X2X_COM_SPAD_TDC_CHANNEL_SELECT_0)], &ch_select);
    for (x = 0, tail = &head[reg_to_idx(TMF8X2X_COM_SPAD_TDC_CHANNEL0_0)];
         x < spad_cfg->xsize; ++x)
    {
        tail = decode_32b(tail, &ch_map);
        for (y = spad_cfg->ysize - 1, yIdx = 0; y >= 0; --y, ++yIdx) {
            ch = TMF8X2X_MAIN_SPAD_DECODE_CHANNEL( ch_map, yIdx );
            if (ch_select & (1 << (spad_cfg->ysize - 1 - y))) {
                if (ch == 0) ch = 8;
                else if (ch == 1) ch = 9;
            }
            spad_cfg->spad_map[y*spad_cfg->xsize + x] = ch;
        }
    }
    return 0;
}

#ifndef CONFIG_TMF882X_NO_HISTOGRAM_SUPPORT
static int32_t rid_to_histogram_type(struct tmf882x_mode_app *app,
                                 uint32_t rid)
{
    // Translate RID to output histogram type
    switch(rid) {
        case TMF8X2X_COM_RID_RAW_HISTOGRAM_24_BITS:
            return HIST_TYPE_RAW;
        case TMF8X2X_COM_RID_ELECTRICAL_CALIBRATION_24_BITS:
            return HIST_TYPE_ELEC_CAL;
        default:
            tof_info(priv(app), "Error '%s' unknown RID -> Histogram mapping",
                     __func__);
            return -1;
    };
}

static int32_t decode_histogram_msg(struct tmf882x_mode_app *app,
                                    const struct tmf882x_mode_app_i2c_msg *i2c_msg)
{
    struct tmf882x_msg *msg = to_msg(app);
    uint32_t tdc_idx = 0;
    uint32_t bin_idx = 0;
    uint32_t byte_idx = 0;
    uint32_t offset = 0;
    uint32_t num_tdc = 0;
    uint32_t bytes_per_bin = 0;
    uint32_t num_bins = 0;
    uint32_t hist_type = rid_to_histogram_type(app, i2c_msg->rid);
    const uint8_t *data = i2c_msg->buf;

    tof_app_dbg(app, "Histogram Info - HIST_TYPE: %u", hist_type);

    // All histograms have same number of bins/channels by default
    num_bins = TMF882X_HIST_NUM_BINS;
    num_tdc = TMF882X_HIST_NUM_TDC;

    switch (hist_type) {
        case HIST_TYPE_RAW:
        case HIST_TYPE_ELEC_CAL:
            /* raw / elec cal histograms are 24 bit */
            bytes_per_bin = 3;
            break;
        default:
            return -1;
    };

    if (num_tdc * bytes_per_bin * num_bins > i2c_msg->size) {
        tof_err(priv(app), "Error decoding histogram i2c message, "
                "len too short: %u", i2c_msg->size);
        return -1;
    }

    /* Histogram output format from device is shifted out LSB-first
     * of each bin, for each TDC. See below:
     *
     *     tdc0 - bin0 - byte0
     *     tdc0 - bin1 - byte0
     *      .
     *     tdc1 - bin0 - byte0
     *     tdc1 - bin1 - byte0
     *      .
     *      .
     *      .
     *     tdc0 - bin0 - byte1
     *     tdc0 - bin1 - byte1
     *      .
     *     tdc1 - bin0 - byte1
     *     tdc1 - bin1 - byte1
     */

    //initialize output msg
    TOF_ZERO_MSG(msg);

    // Init histogram msg header
    TOF_SET_HISTOGRAM_MSG(msg, hist_type);
    msg->hist_msg.num_bins = num_bins;
    msg->hist_msg.num_tdc = num_tdc;
    // This tag *should* match the 'result_num' from the next measurement
    // result data
    msg->hist_msg.capture_num = app->volat_data.capture_num;

    for (tdc_idx = 0; tdc_idx < num_tdc; ++tdc_idx) {

        for (byte_idx = 0; byte_idx < bytes_per_bin; ++byte_idx) {

            offset = (byte_idx * num_bins * num_tdc) + (num_bins * tdc_idx);

            for (bin_idx = 0; bin_idx < num_bins; ++bin_idx) {
                msg->hist_msg.bins[tdc_idx][bin_idx] |=
                    (data[offset + bin_idx] << (BITS_IN_BYTE*byte_idx));
            }
        }
    }

    // Update time-multiplexed index (sub capture)
    msg->hist_msg.sub_capture = i2c_msg->cfg_id;

    // debug histogram data
    if (DEBUG_DUMP_HIST) {
        tmf882x_dump_data(to_parent(app), (uint8_t *)msg->hist_msg.bins[0], 256);
    }

    // publish histogram data
    return tof_queue_msg(priv(app), msg);
}
#endif

static int32_t tmf882x_mode_app_i2c_msg_push(struct tmf882x_mode_app *app,
                                             const struct tmf882x_mode_app_i2c_msg *i2c_msg)
{
    int32_t rc;
    uint8_t hdr_buf[TMF8X2X_COM_HEADER_SIZE] = {0};

    if (i2c_msg->size > 0) {

        (void) encode_i2c_msg_header(i2c_msg, hdr_buf, sizeof(hdr_buf));

        /* write header info */
        rc = tof_i2c_write(priv(app), TMF8X2X_COM_TID,
                           hdr_buf, TMF8X2X_COM_HEADER_SIZE);
        if (rc) {
            TOF_SET_ERR_MSG(to_msg(app), ERR_COMM);
            tof_queue_msg(priv(app), to_msg(app));
            tof_err(priv(app), "Error: %d writing App i2c_msg header", rc);
            return -1;
        }

        /* write payload data */
        rc = tof_i2c_write(priv(app),
                           TMF8X2X_COM_CONFIG_RESULT + TMF8X2X_COM_HEADER_SIZE,
                           i2c_msg->buf, i2c_msg->size);
        if (rc) {
            tof_err(priv(app), "Error: %d writing App i2c_msg payload", rc);
            TOF_SET_ERR_MSG(to_msg(app), ERR_COMM);
            tof_queue_msg(priv(app), to_msg(app));
            return -1;
        }
    }

    tof_app_dbg(app, "app: i2c_msg_send - CMD: %#x SIZE: %u B",
                i2c_msg->cmd, i2c_msg->size);

    /* commit command */
    rc = tof_set_register(priv(app), TMF8X2X_COM_CMD_STAT,
                          i2c_msg->cmd);
    if (rc) {
        tof_err(priv(app), "Error: %d writing App i2c_msg command", rc);
        TOF_SET_ERR_MSG(to_msg(app), ERR_COMM);
        tof_queue_msg(priv(app), to_msg(app));
        return -1;
    }

    return rc;
}

static void tmf882x_force_stop(struct tmf882x_mode_app *app)
{
    struct tmf882x_mode_app_i2c_msg *i2c_msg;

    i2c_msg = to_i2cmsg(app);
    tmf882x_dump_i2c_regs(to_parent(app));

    i2c_msg->cmd = TMF8X2X_COM_CMD_STAT__cmd_stat__CMD_STOP;
    i2c_msg->size = 0;

    tof_info(priv(app), "App mode cmd %s", __func__);
    (void) tmf882x_mode_standby_operation(to_parent(app), TOF_WAKEUP);
    (void) tmf882x_mode_app_i2c_msg_push(app, i2c_msg);
    // try to wait for STOP command completed
    (void) check_cmd_status(app, CMD_TIMEOUT_RETRIES);
    app->volat_data.is_measuring = false;
}

static int32_t tmf882x_mode_app_i2c_msg_send_timeout(struct tmf882x_mode_app *app,
                                                     const struct tmf882x_mode_app_i2c_msg *i2c_msg,
                                                     uint32_t timeout_ms)
{
    int32_t rc;

    if (!i2c_msg) return -1;

    // Check that a command can be sent
    rc = check_cmd_status(app, CMD_TIMEOUT_RETRIES);
    if (rc) {
        tof_err(priv(app), "Error (%d), timeout waiting to send message",
                rc);
        tmf882x_force_stop(app); // force stop to get to stable state
        return rc;
    }

    // send command
    rc = tmf882x_mode_app_i2c_msg_push(app, i2c_msg);
    if (rc) {
        tof_err(priv(app), "Error (%d) sending message", rc);
        tmf882x_force_stop(app); // force stop to get to stable state
        return rc;
    }

    // check that command status is successful
    rc = check_cmd_status_timeout(app, timeout_ms);
    if (rc) {
        tof_err(priv(app), "Error (%d) timeout waiting for msg send complete",
                rc);
        tmf882x_force_stop(app); // force stop to get to stable state
        return rc;
    }

    return rc;
}

static int32_t tmf882x_mode_app_i2c_msg_send(struct tmf882x_mode_app *app,
                                             const struct tmf882x_mode_app_i2c_msg *i2c_msg)
{
    int32_t rc;

    if (!i2c_msg) return -1;

    // Check that a command can be sent
    rc = check_cmd_status(app, CMD_TIMEOUT_RETRIES);
    if (rc) {
        tof_err(priv(app), "Error (%d), timeout waiting to send message",
                rc);
        tmf882x_force_stop(app); // force stop to get to stable state
        return rc;
    }

    // send command
    rc = tmf882x_mode_app_i2c_msg_push(app, i2c_msg);
    if (rc) {
        tof_err(priv(app), "Error (%d) sending message", rc);
        tmf882x_force_stop(app); // force stop to get to stable state
        return rc;
    }

    // check that command status is successful
    rc = check_cmd_status(app, CMD_TIMEOUT_RETRIES);
    if (rc) {
        tof_err(priv(app), "Error (%d) timeout waiting for msg send complete",
                rc);
        tmf882x_force_stop(app); // force stop to get to stable state
        return rc;
    }

    return rc;
}

static int32_t tmf882x_mode_app_i2c_msg_recv(struct tmf882x_mode_app *app,
                                             struct tmf882x_mode_app_i2c_msg *i2c_msg)
{
    const uint8_t *head = NULL;
    uint8_t last_tid;
    size_t data_read = 0;
    size_t payload_sz = TMF8X2X_COM_HEADER_PLUS_PAYLOAD;
    uint32_t num_retries;
    int32_t rc = 0;

    if (!i2c_msg) return -1;

    rc = check_cmd_status(app, CMD_TIMEOUT_RETRIES);
    if (rc) {
        tof_err(priv(app), "Error, app CMD_STAT timeout");
        tmf882x_force_stop(app); // force stop to get to stable state
        return rc;
    }

    // make sure a new message has been published
    rc = wait_for_tid_change(app);
    if (rc) {
        tof_err(priv(app), "Error: %d IRQ TID never changed", rc);
        TOF_SET_ERR_MSG(to_msg(app), ERR_COMM);
        tof_queue_msg(priv(app), to_msg(app));
        return -1;
    }

    // Read i2c message
    rc = tof_i2c_read(priv(app), TMF8X2X_COM_CONFIG_RESULT,
                      i2c_msg->buf, payload_sz);
    if (rc) {
        tof_err(priv(app), "Error: %d reading App i2c_msg header", rc);
        TOF_SET_ERR_MSG(to_msg(app), ERR_COMM);
        tof_queue_msg(priv(app), to_msg(app));
        return -1;
    }

    head = decode_i2c_msg_header(i2c_msg, i2c_msg->buf, payload_sz);
    tof_app_dbg(app, "app: i2c_msg_recv - RID: %#x TID: %#x SIZE: %u B",
                i2c_msg->rid, i2c_msg->tid, i2c_msg->size);

    if (APP_RESP_IS_MULTI_PACKET(i2c_msg->rid)) {

        data_read = i2c_msg->pckt_size;
        // First packet is already read, throw out header by shifting data down
        app_memmove(i2c_msg->buf, head, i2c_msg->pckt_size);

        while (data_read < i2c_msg->size) {

            // clear subpacket irq
            (void) tof_clear_irq(app);

            num_retries = TID_CHANGE_RETRIES;
            last_tid = i2c_msg->tid;

            do {
                // Read i2c data packet
                rc = tof_i2c_read(priv(app),
                                  TMF8X2X_COM_CONFIG_RESULT,
                                  &i2c_msg->buf[data_read],
                                  payload_sz);
                if (rc) {
                    tof_err(priv(app), "Error: %d reading App i2c_msg packet",
                            rc);
                    TOF_SET_ERR_MSG(to_msg(app), ERR_COMM);
                    tof_queue_msg(priv(app), to_msg(app));
                    return -1;
                }

                head = decode_i2c_msg_header(i2c_msg,
                                             &i2c_msg->buf[data_read],
                                             payload_sz);

                // check TID
                if (i2c_msg->tid == last_tid) {
                    /* TID has not changed*/
                    if (num_retries == 0) {
                        tof_app_dbg(app, "app prev TID: %#x curr TID: %#x",
                                    last_tid, i2c_msg->tid);
                        return -1;
                    }
                    tof_info(priv(app), "app tid did not change, retrying");
                    continue;
                } else {
                    break;
                }
            } while (--num_retries);

            // throw away subpacket header by shifting down data
            app_memmove(&i2c_msg->buf[data_read], head, i2c_msg->pckt_size);
            data_read += i2c_msg->pckt_size;
        }

        tof_app_dbg(app, "app: multi-packet read complete - data_read: %zu B",
                    data_read);

    } else {
        // Single payload message has already been read,
        // so shift i2c payload down since the header has been parsed
        // memmove is required here instead of memcpy because the mem overlaps
        app_memmove(i2c_msg->buf, head, i2c_msg->size);
    }
    return 0;
}

static int32_t send_breakpoint_continue_cmd(struct tmf882x_mode_app *app)
{
    struct tmf882x_mode_app_i2c_msg *i2c_msg;

    i2c_msg = to_i2cmsg(app);

    i2c_msg->cmd = TMF8X2X_COM_CMD_STAT__cmd_stat__CMD_BREAKPOINT_GO;
    i2c_msg->size = 0;

    return tmf882x_mode_app_i2c_msg_send(app, i2c_msg);
}

static int32_t decode_irq_msg(struct tmf882x_mode_app *app,
                          const struct tmf882x_mode_app_i2c_msg *i2c_msg)
{
    int32_t rc = 0;

    if (!i2c_msg) return -1;

    // IRQ messages are usually usually published as driver data
    // messages, or simply logged. Synchronous response messages
    // should not go through this function.

    switch(i2c_msg->rid) {
        case TMF8X2X_COM_CONFIG_RESULT__cid_rid__MEASUREMENT_RESULT:
            rc = decode_result_msg(app, i2c_msg);
            break;
#ifndef CONFIG_TMF882X_NO_HISTOGRAM_SUPPORT
        case TMF8X2X_COM_RID_RAW_HISTOGRAM_24_BITS:
        case TMF8X2X_COM_RID_ELECTRICAL_CALIBRATION_24_BITS:
            rc = decode_histogram_msg(app, i2c_msg);
            break;
#endif
        case TMF8X2X_COM_RID_BREAKPOINT_HIT:
            if (i2c_msg->size) {
                tof_info(priv(app), "app RID: %#x BREAKPOINT data size: %u B",
                         i2c_msg->rid, i2c_msg->size);
                tmf882x_dump_data(to_parent(app), i2c_msg->buf, i2c_msg->size);
            }
            rc = send_breakpoint_continue_cmd(app);
            break;
        case TMF8X2X_COM_CONFIG_RESULT__cid_rid__ELECTRICAL_CALIBRATION_RESULT:
            if (i2c_msg->size) {
                tof_info(priv(app), "app RID: %#x ALGORITHM BREAKPOINT data "
                         "size: %u B", i2c_msg->rid, i2c_msg->size);
                tmf882x_dump_data(to_parent(app), i2c_msg->buf, i2c_msg->size);
            }
            break;
        case TMF8X2X_COM_CONFIG_RESULT__cid_rid__ACCUMULATED_HITS_RESULT:
            rc = decode_meas_stats_msg(app, i2c_msg);
            break;
        case 0:
            // Message RID '0' is not a valid message, and this IRQ does not
            // need any handling
            rc = 0;
            break;
        default:
            tof_err(priv(app), "Error, unhandled IRQ message RID: (0x%02x)",
                    i2c_msg->rid);
            break;
    }

    if (rc) {
        // publish error message
        TOF_SET_ERR_MSG(to_msg(app), ERR_COMM);
        tof_queue_msg(priv(app), to_msg(app));
    }

    return rc;
}

static int32_t validate_spad_config(struct tmf882x_mode_app *app,
                                    const struct tmf882x_mode_app_spad_config *spad_cfg)
{
    int32_t i, x, y;
    bool muxed_lch = false;
    bool muxed_hch = false;
    uint32_t ch;
    uint32_t spad_idx = 0;
    for (i = 0; i < spad_cfg->num_spad_configs; ++i) {
        if (spad_cfg->spad_configs[i].xsize > TMF8X2X_COM_MAX_SPAD_XSIZE) {
            tof_err(priv(app), "Error spad_cfg xsize too large");
            return -1;
        }
        if (spad_cfg->spad_configs[i].ysize > TMF8X2X_COM_MAX_SPAD_YSIZE) {
            tof_err(priv(app), "Error spad_cfg ysize too large");
            return -1;
        }
        for (y = spad_cfg->spad_configs[i].ysize - 1; y <= 0; --y) {
            muxed_lch = false;
            muxed_hch = false;
            for (x = 0; x < spad_cfg->spad_configs[i].xsize; ++x) {
                spad_idx = y * spad_cfg->spad_configs[i].xsize + x;
                ch =
                    spad_cfg->spad_configs[i].spad_map[spad_idx];
                if (ch == 8 || ch == 9) muxed_hch = true;
                if (ch == 0 || ch == 1) muxed_lch = true;
                if (muxed_lch && muxed_hch) {
                    tof_err(priv(app), "Error spad_cfg invalid "
                            "channel mapping (0/1 and 8/9) collision");
                    return -1;
                }
            }
        }
    }
    return 0;
}

static int32_t i2c_msg_send_and_receive(struct tmf882x_mode_app *app,
                                    struct tmf882x_mode_app_i2c_msg *msg)
{
    int32_t rc = 0;
    rc = tmf882x_mode_app_i2c_msg_send(app, msg);
    if (rc) {
        return rc;
    }

    rc = tmf882x_mode_app_i2c_msg_recv(app, msg);
    if (rc) {
        return rc;
    }

    return rc;
}

static int32_t read_uid(struct tmf882x_mode_app *app)
{
    return tof_i2c_read(priv(app), TMF8X2X_COM_SERIAL_NUMBER_0,
                        app->volat_data.uid,
                        sizeof(app->volat_data.uid));
}

static int32_t tmf882x_enable_interrupts(struct tmf882x_mode_app *app, uint32_t flags)
{
    uint8_t reg;
    uint8_t int_en_flags = flags;
    int32_t error;

    error = tof_get_register(priv(app), TMF882X_INT_EN, &reg);
    reg &= TMF882X_INT_MASK;
    reg |= int_en_flags;
    if (error) {
        return error;
    }
    return tof_set_register(priv(app), TMF882X_INT_EN, reg);
}

static int32_t tmf882x_disable_interrupts(struct tmf882x_mode_app *app, uint32_t flags)
{
    uint8_t reg;
    uint8_t int_en_flags = flags;
    int32_t error;

    error = tof_get_register(priv(app), TMF882X_INT_EN, &reg);
    reg &= TMF882X_INT_MASK;
    reg &= ~int_en_flags;
    if (error) {
        return error;
    }
    return tof_set_register(priv(app), TMF882X_INT_EN, reg);
}

static void tmf882x_mode_app_close(struct tmf882x_mode *self)
{
    struct tmf882x_mode_app *app;
    if (!verify_mode(self)) return;
    app = member_of(self, struct tmf882x_mode_app, mode);
    tof_info(priv(app), "%s", __func__);
    tmf882x_force_stop(app);
    (void) tmf882x_disable_interrupts(app, F_IRQ_ALL);
    app->volat_data.is_open = false;
}

static int32_t tmf882x_mode_app_stop_measurements(struct tmf882x_mode *self)
{
    struct tmf882x_mode_app_i2c_msg *i2c_msg;
    struct tmf882x_mode_app *app;
    int32_t rc;

    if (!verify_mode(self)) return -1;
    app = member_of(self, struct tmf882x_mode_app, mode);
    i2c_msg = to_i2cmsg(app);

    i2c_msg->cmd = TMF8X2X_COM_CMD_STAT__cmd_stat__CMD_STOP;
    i2c_msg->size = 0;

    // make sure device is not in standby-timed before issuing STOP command
    (void) tmf882x_mode_standby_operation(to_parent(app), TOF_WAKEUP);
    tof_app_dbg(app, "Stopping app measurements");
    rc = tmf882x_mode_app_i2c_msg_send(app, i2c_msg);
    if (rc) {
        tof_err(priv(app), "Error (%d) stopping measurements", rc);
    }

    app->volat_data.is_measuring = false;
    return rc;
}

static int32_t tmf882x_mode_app_switch_to_bl(struct tmf882x_mode *self,
                                             uint32_t req_mode)
{
    struct tmf882x_mode_app *app;

    if (!verify_mode(self)) return -1;
    app = member_of(self, struct tmf882x_mode_app, mode);
    if (req_mode == TMF882X_MODE_APP) return 0;
    if (req_mode != TMF882X_MODE_BOOTLOADER) return -1;
    // close app mode
    tmf882x_mode_app_close(self);
    tof_info(priv(app), "%s", __func__);
    /***************************************************************************
    * Try to use the safe approach --
    *  PON standby with bootmatrix, if any errors use unfriendly CPU reset
    ***************************************************************************/
    if (tmf882x_mode_standby_operation(self, TOF_STANDBY)) {
        return tmf882x_mode_cpu_reset(to_parent(app), 0x1);
    }
    if (tmf882x_mode_set_powerup_bootmatrix(self, 0x1)) {
        return tmf882x_mode_cpu_reset(to_parent(app), 0x1);
    }
    // If we got here, the device is in standby with the bootmatrix configured
    // for the bootloader mode when PON=1
    return 0;
}

static int32_t tmf882x_mode_app_start_measurements(struct tmf882x_mode *self)
{
    struct tmf882x_mode_app_i2c_msg *i2c_msg;
    struct tmf882x_mode_app *app;
    int32_t rc;

    if (!verify_mode(self)) return -1;
    app = member_of(self, struct tmf882x_mode_app, mode);
    if (is_measuring(app)) return 0;
    i2c_msg = to_i2cmsg(app);

    tof_app_dbg(app, "Starting app measurements");
    i2c_msg->cmd = TMF8X2X_COM_CMD_STAT__cmd_stat__CMD_MEASURE;
    i2c_msg->size = 0;

    rc = tmf882x_mode_app_i2c_msg_send(app, i2c_msg);
    if (rc) {
        tof_err(priv(app), "Error:%d starting measurements", rc);
    }

    //restart our capture iteration counter
    app->volat_data.capture_num = 1;
    tmf882x_clk_corr_recalc(&app->volat_data.clk_cr);
    app->volat_data.is_measuring = true;
    return rc;
}

static int32_t commit_config_msg(struct tmf882x_mode_app *app,
                             struct tmf882x_mode_app_i2c_msg *i2c_msg)
{
    if (!app || !i2c_msg) return -1;
    i2c_msg->cmd = TMF8X2X_COM_CMD_STAT__cmd_stat__CMD_WRITE_CONFIG_PAGE;

    // commit new config page data
    return tmf882x_mode_app_i2c_msg_send(app, i2c_msg);
}

static int32_t tmf882x_mode_app_handle_irq(struct tmf882x_mode *self)
{
    int32_t rc = 0;
    int32_t int_stat;
    struct tmf882x_mode_app_i2c_msg *i2c_msg;
    struct tmf882x_mode_app *app;

    if (!verify_mode(self)) return -1;
    app = member_of(self, struct tmf882x_mode_app, mode);

    i2c_msg = to_i2cmsg(app);

    int_stat = tof_clear_irq(app);
    if (int_stat < 0) {
        TOF_SET_ERR_MSG(to_msg(app), ERR_COMM);
        tof_queue_msg(priv(app), to_msg(app));
        return int_stat;
    }

    tof_app_dbg(app, "IRQ stat: %#x", int_stat);

    // cache the IRQ type while processing
    app->volat_data.irq = int_stat;

    int_stat &= ~0x01;  // clear common IRQ flag

    // handle error IRQ
    if (int_stat & F_ERROR_IRQ) {
        tof_err(priv(app), "Received ERROR_IRQ");
        tmf882x_dump_i2c_regs(to_parent(app));
        int_stat &= ~F_ERROR_IRQ;  // clear error IRQ flag
    }

    // if we have stopped measuring since reaching the IRQ handler,
    // just return
    if (!is_measuring(app))
        return 0;

    // handle CMD_DONE IRQ
    if (int_stat & F_CMD_DONE_IRQ) {
        int_stat &= ~F_CMD_DONE_IRQ;  // clear cmd_done IRQ flag
    }

    if (int_stat) {
        // All other IRQs are handled here
        rc = tmf882x_mode_app_i2c_msg_recv(app, i2c_msg);
        if (rc) {
            tof_err(priv(app), "Error (%d) receiving i2c message", rc);
            return rc;
        }
        rc = decode_irq_msg(app, i2c_msg);
        if (rc) {
            tof_err(priv(app), "Error (%d) decoding i2c message", rc);
            return rc;
        }
    }

    app->volat_data.irq = 0;
    return rc;
}

static int32_t tmf882x_mode_app_get_config(struct tmf882x_mode_app *app,
                                           struct tmf882x_mode_app_config *cfg)
{
    int32_t rc = 0;
    bool capture_state = false;
    struct tmf882x_mode_app_i2c_msg *i2c_msg;

    if (!verify_mode(&app->mode)) return -1;
    if (!cfg) return -1;

    if ((capture_state = is_measuring(app))) {
        rc = tmf882x_mode_app_stop_measurements(&app->mode);
        if (rc) {
            tof_err(priv(app), "Error (%d) stopping measurements "
                    "for reading config", rc);
            return -1;
        }
    }

    i2c_msg = to_i2cmsg(app);
    i2c_msg->cmd = TMF8X2X_COM_CMD_STAT__cmd_stat__CMD_LOAD_CONFIG_PAGE_COMMON;
    i2c_msg->size = 0;

    rc = i2c_msg_send_and_receive(app, i2c_msg);
    if (rc) {
        tof_err(priv(app), "Error (%d) reading common config", rc);
        return -1;
    }

    rc = decode_config_msg(app, i2c_msg, cfg);
    if (rc) {
        tof_err(priv(app), "Error (%d) decoding common config", rc);
        return -1;
    }

    if (DEBUG_DUMP_CONFIG) {
        tof_info(priv(app), "READ Config");
        dump_config(app, cfg);
    }

    // Cache latest common config to local context
    app_memmove(&app->volat_data.cfg, cfg, sizeof(app->volat_data.cfg));

    if (capture_state) {
        rc = tmf882x_mode_app_start_measurements(&app->mode);
        if (rc) {
            tof_err(priv(app), "Error (%d) re-starting measurements", rc);
            return -1;
        }
    }

    return rc;
}

static int32_t tmf882x_mode_app_set_config(struct tmf882x_mode_app *app,
                                           const struct tmf882x_mode_app_config *cfg)
{
    int32_t rc = 0;
    bool capture_state = false;
    struct tmf882x_mode_app_i2c_msg *i2c_msg;

    if (!verify_mode(&app->mode)) return -1;
    if (!cfg) return -1;

    if ((capture_state = is_measuring(app))) {
        rc = tmf882x_mode_app_stop_measurements(&app->mode);
        if (rc) {
            tof_err(priv(app), "Error (%d) stopping measurements "
                    "for setting new config", rc);
            return -1;
        }
    }

    // set message command type
    i2c_msg = to_i2cmsg(app);
    i2c_msg->cmd = TMF8X2X_COM_CMD_STAT__cmd_stat__CMD_LOAD_CONFIG_PAGE_COMMON;
    i2c_msg->size = 0;

    rc = i2c_msg_send_and_receive(app, i2c_msg);
    if (rc) {
        tof_err(priv(app), "Error (%d) reading common config for modification",
                rc);
        return -1;
    }

    rc = encode_config_msg(app, i2c_msg, cfg);
    if (rc) {
        tof_err(priv(app), "Error (%d) encoding common config", rc);
        return -1;
    }

    rc = commit_config_msg(app, i2c_msg);
    if (rc) {
        tof_err(priv(app), "Error (%d) commiting common config", rc);
        return -1;
    }

    // Cache latest common config to local context
    app_memmove(&app->volat_data.cfg, cfg, sizeof(app->volat_data.cfg));

    if (DEBUG_DUMP_CONFIG) {
        tof_info(priv(app), "WRITE Config");
        dump_config(app, cfg);
    }

    if (capture_state) {
        rc = tmf882x_mode_app_start_measurements(&app->mode);
        if (rc) {
            tof_err(priv(app), "Error (%d) re-starting measurements", rc);
            return -1;
        }
    }

    return rc;
}

static int32_t tmf882x_mode_app_get_spad_config(struct tmf882x_mode_app *app,
                                                struct tmf882x_mode_app_spad_config *spad_cfg)
{
    int32_t rc = 0;
    bool capture_state = false;
    struct tmf882x_mode_app_i2c_msg *i2c_msg;
    int32_t i;
    uint32_t num_cfg;

    if (!verify_mode(&app->mode)) return -1;
    if (!spad_cfg) return -1;

    if ((capture_state = is_measuring(app))) {
        rc = tmf882x_mode_app_stop_measurements(&app->mode);
        if (rc) {
            tof_err(priv(app), "Error (%d) stopping measurements "
                    "for reading spad config", rc);
            return -1;
        }
    }

    // limit to max # of supported spad configs
    num_cfg = TMF8X2X_MAX_CONFIGURATIONS;
    i2c_msg = to_i2cmsg(app);

    for (i = 0, spad_cfg->num_spad_configs = 0; i < num_cfg; ++i) {

        // Read the nth time-multiplexed spad config assume spad read configs are all
        // mapped together incrementally
        i2c_msg->cmd = TMF8X2X_COM_CMD_STAT__cmd_stat__CMD_LOAD_CONFIG_PAGE_SPAD_1 + i;
        i2c_msg->size = 0;

        rc = i2c_msg_send_and_receive(app, i2c_msg);
        if (rc) {
            tof_err(priv(app), "Error (%d) reading spad_%u config", rc, i);
            return -1;
        }

        rc = decode_spad_config_msg(app, i2c_msg, &spad_cfg->spad_configs[i]);
        if (rc) {
            tof_err(priv(app), "Error (%d) decoding spad_%u config", rc, i);
            return -1;
        }

        spad_cfg->num_spad_configs++;

        if (DEBUG_DUMP_SPAD_CONFIG) {
            tof_info(priv(app), "READ Spad Config[%u]", i);
            dump_spad_config(app, &spad_cfg->spad_configs[i]);
        }

    }

    if (capture_state) {
        rc = tmf882x_mode_app_start_measurements(&app->mode);
        if (rc) {
            tof_err(priv(app), "Error (%d) re-starting measurements", rc);
            return -1;
        }
    }

    return rc;
}

static int32_t tmf882x_mode_app_set_spad_config(struct tmf882x_mode_app *app,
                                                const struct tmf882x_mode_app_spad_config *spad_cfg)
{
    int32_t rc = 0;
    int32_t i;
    bool capture_state = false;
    struct tmf882x_mode_app_i2c_msg *i2c_msg;
    uint32_t num_cfg;

    if (!verify_mode(&app->mode)) return -1;
    if (!spad_cfg) return -1;
    if (validate_spad_config(app, spad_cfg) != 0) return -1;

    num_cfg = spad_cfg->num_spad_configs;

    if ((capture_state = is_measuring(app))) {
        rc = tmf882x_mode_app_stop_measurements(&app->mode);
        if (rc) {
            tof_err(priv(app), "Error (%d) stopping measurements "
                    "for setting new spad config", rc);
            return -1;
        }
    }

    // limit to max # of supported spad configs
    switch (app->volat_data.cfg.spad_map_id) {
        case TMF8X2X_COM_SPAD_MAP_ID__spad_map_id__user_defined_1:
            // This spad_map_id only supports one spad config
            if (num_cfg == 0) {
                tof_err(priv(app), "Error, num spad configs needed is '1',"
                        " %u provided", num_cfg);
                return -1;
            }
            num_cfg = 1;
            break;
        case TMF8X2X_COM_SPAD_MAP_ID__spad_map_id__user_defined_2:
            // This spad_map_id only supports two spad configs
            if (num_cfg < 2) {
                tof_err(priv(app), "Error, num spad configs needed is '2',"
                        " %u provided", num_cfg);
                return -1;
            }
            num_cfg = 2;
            break;
        default:
            tof_err(priv(app), "Error spad_map_id config setting %u "
                    "invalid for setting custom spad configurations",
                    app->volat_data.cfg.spad_map_id);
            return -1;
    }

    i2c_msg = to_i2cmsg(app);
    for (i = 0; i < num_cfg; ++i) {

        // Write the nth time-multiplexed spad config assume spad read configs are all
        // mapped together incrementally
        i2c_msg->cmd = TMF8X2X_COM_CMD_STAT__cmd_stat__CMD_LOAD_CONFIG_PAGE_SPAD_1 + i;
        i2c_msg->size = 0;
        memset(i2c_msg->buf, 0, sizeof(i2c_msg->buf));

        rc = i2c_msg_send_and_receive(app, i2c_msg);
        if (rc) {
            tof_err(priv(app), "Error (%d) reading spad_%u config for modification",
                    rc, i);
            return -1;
        }

        rc = encode_spad_config_msg(app, i2c_msg, &spad_cfg->spad_configs[i]);
        if (rc) {
            tof_err(priv(app), "Error (%d) encoding spad_%u config", rc, i);
            return -1;
        }

        rc = commit_config_msg(app, i2c_msg);
        if (rc) {
            tof_err(priv(app), "Error (%d) commiting spad_%u config RID: %#x",
                    rc, i, i2c_msg->rid);
            return -1;
        }

        if (DEBUG_DUMP_SPAD_CONFIG) {
            tof_info(priv(app), "Write Spad Config[%u]", i);
            dump_spad_config(app, &spad_cfg->spad_configs[i]);
        }
    }

    if (capture_state) {
        rc = tmf882x_mode_app_start_measurements(&app->mode);
        if (rc) {
            tof_err(priv(app), "Error (%d) re-starting measurements", rc);
            return -1;
        }
    }

    return rc;
}

static int32_t tmf882x_mode_app_get_calib_data(struct tmf882x_mode_app *app,
                                               struct tmf882x_mode_app_calib *calib)
{
    int32_t rc = 0;
    bool capture_state = false;
    bool is_8x8 = false;
    struct tmf882x_mode_app_i2c_msg *i2c_msg;
    uint32_t len = 0;
    uint32_t i=0;
    uint32_t num_calib=0;

    if (!verify_mode(&app->mode)) return -1;
    if (!calib) return -1;

    if ((capture_state = is_measuring(app))) {
        rc = tmf882x_mode_app_stop_measurements(&app->mode);
        if (rc) {
            tof_err(priv(app), "Error (%d) stopping measurements "
                    "for setting new spad config", rc);
            return -1;
        }
    }

    is_8x8 = tmf882x_mode_app_is_8x8_mode(app);

    i2c_msg = to_i2cmsg(app);
    if (is_8x8) {
        // 8x8 application factory cal reset prep
        i2c_msg->cmd = TMF8X2X_COM_CMD_STAT__cmd_stat__CMD_RESET_FACTORY_CALIBRATION;
        i2c_msg->size = 0;
        rc = tmf882x_mode_app_i2c_msg_send(app, i2c_msg);
        if (rc) {
            tof_err(priv(app), "Error (%d) sending factory cal reset cmd", rc);
        }
    }

    calib->calib_len = 0;
    num_calib = is_8x8 ? NUM_8x8_CFG : 1;
    for (i = 0; i < num_calib; i++) {
        i2c_msg->cmd = TMF8X2X_COM_CMD_STAT__cmd_stat__CMD_LOAD_CONFIG_PAGE_FACTORY_CALIB;
        i2c_msg->size = 0;
        rc = i2c_msg_send_and_receive(app, i2c_msg);
        if (rc) {
            tof_err(priv(app), "Error (%d) reading factory calibration page", rc);
            return -1;
        }

        len = (i2c_msg->size + calib->calib_len) > sizeof(calib->data) ?
                (sizeof(calib->data) - calib->calib_len) : i2c_msg->size;
        // copy calibration data
        memcpy(calib->data + calib->calib_len, i2c_msg->buf, len);
        calib->calib_len += len;

        tof_info(priv(app), "Read calibration data: %u B", calib->calib_len);
    }

    if (capture_state) {
        rc = tmf882x_mode_app_start_measurements(&app->mode);
        if (rc) {
            tof_err(priv(app), "Error (%d) re-starting measurements", rc);
            return -1;
        }
    }

    return 0;
}

static int32_t tmf882x_mode_app_set_calib_data(struct tmf882x_mode_app *app,
                                               const struct tmf882x_mode_app_calib *calib)
{
    int32_t rc = 0;
    bool capture_state = false;
    bool is_8x8 = false;
    struct tmf882x_mode_app_i2c_msg *i2c_msg;
    uint32_t len = 0;
    uint32_t i = 0;
    uint32_t num_calib = 0;
    uint32_t off = 0;

    if (!verify_mode(&app->mode)) return -1;
    if (!calib) return -1;

    if ((capture_state = is_measuring(app))) {
        rc = tmf882x_mode_app_stop_measurements(&app->mode);
        if (rc) {
            tof_err(priv(app), "Error (%d) stopping measurements "
                    "for setting new spad config", rc);
            return -1;
        }
    }

    is_8x8 = tmf882x_mode_app_is_8x8_mode(app);

    i2c_msg = to_i2cmsg(app);
    if (is_8x8) {
        i2c_msg->cmd = TMF8X2X_COM_CMD_STAT__cmd_stat__CMD_RESET_FACTORY_CALIBRATION;
        i2c_msg->size = 0;
        rc = tmf882x_mode_app_i2c_msg_send(app, i2c_msg);
        if (rc) {
            tof_err(priv(app), "Error (%d) sending factory cal reset cmd", rc);
        }
    }

    num_calib = is_8x8 ? NUM_8x8_CFG : 1;
    for (i = 0, off = 0; i < num_calib; i++, off += len) {
        i2c_msg->cmd = TMF8X2X_COM_CMD_STAT__cmd_stat__CMD_LOAD_CONFIG_PAGE_FACTORY_CALIB;
        i2c_msg->size = 0;
        rc = i2c_msg_send_and_receive(app, i2c_msg);
        if (rc) {
            tof_err(priv(app), "Error (%d) loading factory calibration page", rc);
            return -1;
        }

        len = (calib->calib_len - off) > 188 ? 188 : calib->calib_len - off;
        // copy calibration data
        memcpy(i2c_msg->buf, calib->data + off, len);
        i2c_msg->size = len;

        rc = commit_config_msg(app, i2c_msg);
        if (rc) {
            tof_err(priv(app), "Error (%d) commiting calibration data config",
                    rc);
            return -1;
        }

        tof_info(priv(app), "Write calibration data: %zu B", len + off);
    }

    if (capture_state) {
        rc = tmf882x_mode_app_start_measurements(&app->mode);
        if (rc) {
            tof_err(priv(app), "Error (%d) re-starting measurements", rc);
            return -1;
        }
    }

    return rc;
}

static int32_t tmf882x_mode_app_do_factory_calib(struct tmf882x_mode_app *app,
                                                 struct tmf882x_mode_app_calib *calib)
{
    int32_t rc = 0;
    bool capture_state = false;
    bool is_8x8 = false;
    struct tmf882x_mode_app_i2c_msg *i2c_msg;
    uint32_t hist_dump_save = 0;
    uint32_t i = 0;
    uint32_t num_calib = 0;

    if (!verify_mode(&app->mode)) return -1;
    if (!calib) return -1;

    if ((capture_state = is_measuring(app))) {
        rc = tmf882x_mode_app_stop_measurements(&app->mode);
        if (rc) {
            tof_err(priv(app), "Error (%d) stopping measurements "
                    "for performing factory calibration", rc);
            return -1;
        }
    }

    // disable histogram readout for factory calibration
    hist_dump_save = app->volat_data.cfg.histogram_dump;
    app->volat_data.cfg.histogram_dump = 0;
    rc = tmf882x_mode_app_set_config(app, &app->volat_data.cfg);
    if (rc) {
        tof_err(priv(app), "Error (%d) disabling histogram dump "
                "for performing factory calibration", rc);
        return -1;
    }
    app->volat_data.cfg.histogram_dump = hist_dump_save;

    is_8x8 = tmf882x_mode_app_is_8x8_mode(app);

    i2c_msg = to_i2cmsg(app);
    if (is_8x8) {
        i2c_msg->cmd = 31; // 8x8 application factory cal reset prep
        i2c_msg->size = 0;
        rc = tmf882x_mode_app_i2c_msg_send(app, i2c_msg);
        if (rc) {
            tof_err(priv(app), "Error (%d) sending factory cal reset cmd", rc);
        }
    }

    num_calib = is_8x8 ? NUM_8x8_CFG : 1;
    tof_info(priv(app), "Running factory calibration -");
    for (i = 0; i < num_calib; i++) {
        i2c_msg = to_i2cmsg(app);
        i2c_msg->cmd = TMF8X2X_COM_CMD_STAT__cmd_stat__CMD_FACTORY_CALIBRATION;
        i2c_msg->size = 0;
        rc = tmf882x_mode_app_i2c_msg_send_timeout(app, i2c_msg,
                CMD_FAC_CALIB_TIMEOUT_MS);
        if (rc) {
            tof_err(priv(app), "Error (%d) performing factory calibration", rc);
            return -1;
        }
    }

    rc = tmf882x_mode_app_get_calib_data(app, calib);
    if (rc) {
        tof_err(priv(app), "Error reading factory calibration");
        return -1;
    }

    app->volat_data.cfg.histogram_dump = hist_dump_save;
    rc = tmf882x_mode_app_set_config(app, &app->volat_data.cfg);
    if (rc) {
        tof_err(priv(app), "Error (%d) re-loading histogram dump "
                "after performing factory calibration", rc);
    }

    if (capture_state) {
        rc = tmf882x_mode_app_start_measurements(&app->mode);
        if (rc) {
            tof_err(priv(app), "Error (%d) re-starting measurements", rc);
            return -1;
        }
    }

    return 0;
}

static int32_t tmf882x_mode_app_get_device_uid(struct tmf882x_mode_app *app,
                                               struct tmf882x_mode_app_dev_UID *uid)
{
    uint8_t *b_uid = app->volat_data.uid;
    if (!verify_mode(&app->mode)) return -1;
    if (!uid) return -1;

    return snprintf(uid->uid, sizeof(uid->uid), "%u.%u.%u.%u",
                    b_uid[0], b_uid[1], b_uid[2], b_uid[3]);
}

static int32_t tmf882x_mode_app_get_clock_compensation(struct tmf882x_mode_app *app,
                                                       bool *clk_skew_compensation)
{
    if (!verify_mode(&app->mode) || !clk_skew_compensation) return -1;
    *clk_skew_compensation = !!app->volat_data.clk_corr_enabled;
    return 0;
}

static int32_t tmf882x_mode_app_set_clock_compensation(struct tmf882x_mode_app *app,
                                                       bool clk_skew_compensation)
{
    if (!verify_mode(&app->mode)) return -1;
    app->volat_data.clk_corr_enabled = clk_skew_compensation;
    return 0;
}

static inline bool tmf882x_mode_app_is_measuring(struct tmf882x_mode_app *app)
{
    if (!app) return false;
    else return app->volat_data.is_measuring;
}

static int32_t tmf882x_mode_app_set_8x8_mode(struct tmf882x_mode_app *app, bool is_8x8)
{
    int32_t rc = 0;
    struct tmf882x_mode_app_i2c_msg *i2c_msg;
    bool capture_state = false;

    if (is_8x8 == tmf882x_mode_app_is_8x8_mode(app))
        return 0; // nothing to do, already in correct mode

    if ((capture_state = is_measuring(app))) {
        rc = tmf882x_mode_app_stop_measurements(&app->mode);
        if (rc) {
            tof_err(priv(app), "Error (%d) stopping measurements "
                    "switching 8x8 mode", rc);
            return -1;
        }
    }

    i2c_msg = to_i2cmsg(app);
    i2c_msg->cmd = is_8x8 ?
                    TMF8X2X_COM_CMD_STAT__cmd_stat__CMD_SWITCH_TMF8828_MODE :
                    TMF8X2X_COM_CMD_STAT__cmd_stat__CMD_SWITCH_TMF8821_MODE;
    i2c_msg->size = 0;
    rc = tmf882x_mode_app_i2c_msg_send_timeout(app, i2c_msg, CMD_DEF_TIMEOUT_MS);
    if (rc) {
        tof_err(priv(app), "Error (%d) setting 8x8 mode to %u", rc, is_8x8);
        return -1;
    }

    // Switching this mode is a device reset so re-open the application mode
    app->volat_data.is_open = false;
    tof_info(priv(app), "Re-opening device mode-");
    rc = tmf882x_mode_app_open(to_parent(app));
    if (rc) {
        tof_err(priv(app), "Error (%d) re-opening application mode", rc);
        return -1;
    }

    // check if the switch was successful
    if (is_8x8 != tmf882x_mode_app_is_8x8_mode(app)) {
        tof_err(priv(app), "Error (%d) setting 8x8 mode to '%u'", rc, is_8x8);
        return -1;
    }

    if (capture_state) {
        rc = tmf882x_mode_app_start_measurements(&app->mode);
        if (rc) {
            tof_err(priv(app), "Error (%d) re-starting measurements", rc);
            return -1;
        }
    }

    return 0;
}

static int32_t tmf882x_mode_app_ioctl(struct tmf882x_mode *self, uint32_t cmd,
                                      const void *input, void *output)
{
    int32_t rc = -1;
    struct tmf882x_mode_app *app;

    if (!verify_mode(self)) return -1;
    app = member_of(self, struct tmf882x_mode_app, mode);

    if (!VERIFY_IOCAPP(cmd)) {
        tof_err(priv(app), "Error IOCTL cmd [%x] does not match "
                "APP mode type.", cmd);
        return -1;
    }

    if (_IOCTL_NR(cmd) >= NUM_APP_IOCTL) {
        tof_err(priv(app), "Error IOCTL cmd [%x] does not exist", cmd);
        return -1;
    }

    switch(_IOCTL_NR(cmd)) {
        case APP_SET_CFG:
            rc = tmf882x_mode_app_set_config(app, input);
            break;
        case APP_GET_CFG:
            rc = tmf882x_mode_app_get_config(app, output);
            break;
        case APP_SET_SPADCFG:
            rc = tmf882x_mode_app_set_spad_config(app, input);
            break;
        case APP_GET_SPADCFG:
            rc = tmf882x_mode_app_get_spad_config(app, output);
            break;
        case APP_GET_CALIB:
            rc = tmf882x_mode_app_get_calib_data(app, output);
            break;
        case APP_SET_CALIB:
            rc = tmf882x_mode_app_set_calib_data(app, input);
            break;
        case APP_DO_FACCAL:
            rc = tmf882x_mode_app_do_factory_calib(app, output);
            break;
        case APP_IS_MEAS:
            (*(bool *)output) = tmf882x_mode_app_is_measuring(app);
            rc = 0;
            break;
        case APP_DEV_UID:
            (void) tmf882x_mode_app_get_device_uid(app, output);
            rc = 0;
            break;
        case APP_IS_CLKADJ:
            rc = tmf882x_mode_app_get_clock_compensation(app, output);
            break;
        case APP_SET_CLKADJ:
            rc = tmf882x_mode_app_set_clock_compensation(app, (*(bool *)input));
            break;
        case APP_SET_8X8MODE:
            rc = tmf882x_mode_app_set_8x8_mode(app, (*(bool *)input));
            break;
        case APP_IS_8X8MODE:
            (*(bool *)output) = tmf882x_mode_app_is_8x8_mode(app);
            rc = 0;
            break;
        default:
            tof_err(priv(app), "Error unhandled IOCTL cmd [%x]", cmd);
    }

    return rc;
}

static int32_t tmf882x_mode_app_open(struct tmf882x_mode *self)
{
    int32_t rc;
    struct tmf882x_mode_app_i2c_msg *i2c_msg;
    struct tmf882x_mode_app *app;
    if (!verify_mode(self)) return -1;
    app = member_of(self, struct tmf882x_mode_app, mode);

    if (app->volat_data.is_open)
        return 0;

    if ( !driver_compatible_with_app(self) ) {
        tof_err(priv(app), "Error, mode is not compatible with driver "
                "module version: %s", TMF882X_MODULE_VER);
        return -1;
    }

    tof_info(priv(app), "%s", __func__);
    memset(&app->volat_data, 0, sizeof(struct volat_data));

    tmf882x_enable_interrupts(app, F_IRQ_ALL & ~F_CMD_DONE_IRQ);
    (void) tmf882x_mode_app_stop_measurements(self);

    // init clock correction context
    tmf882x_clk_corr_init(&app->volat_data.clk_cr, TMF882X_SYSTICK_RATIO);

    // Read Device UID
    rc = read_uid(app);
    if (rc)
        tof_err(priv(app), "Error reading out UID: %d", rc);

    // Read initial application config
    rc = tmf882x_mode_app_get_config(app, &app->volat_data.cfg);
    if (rc) {
        tof_err(priv(app), "Error reading common cfg during open(): %d", rc);
        return -1;
    }

    i2c_msg = to_i2cmsg(app);
    i2c_msg->tid = 0xFF; // set TID to non-zero
    app->volat_data.clk_corr_enabled = CLK_CORR_ENABLE; // set clock correction
    app->volat_data.is_open = true;
    return rc;
}

static const struct mode_vtable ops = {
    .tag = TMF882X_APP_MODE_TAG,
    .open = tmf882x_mode_app_open,
    .mode_switch = tmf882x_mode_app_switch_to_bl,
    .start = tmf882x_mode_app_start_measurements,
    .stop = tmf882x_mode_app_stop_measurements,
    .process_irq = tmf882x_mode_app_handle_irq,
    .ioctl = tmf882x_mode_app_ioctl,
    .close = tmf882x_mode_app_close,
};

void tmf882x_mode_app_init(struct tmf882x_mode_app *app, void *priv)
{
    if (!app) return;
    tmf882x_mode_init(to_parent(app), &ops, priv);
}

