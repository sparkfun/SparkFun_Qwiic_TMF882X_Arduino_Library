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

#ifndef __TMF882X_CLOCK_CORRECTION_H
#define __TMF882X_CLOCK_CORRECTION_H

#include "inc/tmf882x_host_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct tmf882x_clk_corr
 * @brief
 *      This is the Context structure for the clock correction machine
 * @var tmf882x_clk_corr::first_ref
 *      This member contains the first reference clock value
 * @var tmf882x_clk_corr::last_ref
 *      This member contains the last reference clock value
 * @var tmf882x_clk_corr::first_src
 *      This member contains the first source clock value
 * @var tmf882x_clk_corr::last_src
 *      This member contains the last source clock value
 * @var tmf882x_clk_corr::ratio
 *      This member contains the expected ratio of reference and source
 * @var tmf882x_clk_corr::iratioQ15
 *      This member contains the current inverted ratio of source and reference
 *      in Q15 fixed point format
 * @var tmf882x_clk_corr::count
 *      This member contains the current number of pairs added to the mapping
 */
struct tmf882x_clk_corr {
    uint32_t first_ref;
    uint32_t last_ref;
    uint32_t first_src;
    uint32_t last_src;
    uint32_t ratio;
    uint32_t iratioQ15;
    uint32_t count;
};

/**
 *  @brief
 *       Initialize a clock correction context
 *  @param[in] cr pointer to clock correction context structure
 *  @param[in] ratio expected integer ratio of the two clocks
 */
extern void tmf882x_clk_corr_init(struct tmf882x_clk_corr * cr, uint32_t ratio);

/**
 *  @brief
 *       Reset running clock correction state
 *  @param[in] cr pointer to clock correction context structure
 */
extern void tmf882x_clk_corr_recalc(struct tmf882x_clk_corr * cr);

/**
 *  @brief
 *       Add a pair of clock counts to the clock correction state
 *  @param[in] cr pointer to clock correction context structure
 *  @param[in] ref monotonic clock count for reference clock
 *  @param[in] src monotonic clock count for source clock
 *  @note Which clock is used for reference and source does not matter as long
 *        as their use is consistent and is aligned with the expected ratio in
 *        @ref tmf882x_clk_corr_init
 */
extern void tmf882x_clk_corr_addpair(struct tmf882x_clk_corr * cr, uint32_t ref, uint32_t src);

/**
 *  @brief
 *       Apply a clock correction mapping
 *  @param[in] cr pointer to clock correction context structure
 *  @param[in] old_val Value to be corrected by the clock correction mapping
 *  @return Corrected value using the clock correction state. Default with no
 *          pairs added returns old_val.
 */
extern uint32_t tmf882x_clk_corr_map(struct tmf882x_clk_corr * cr, uint32_t old_val);

#ifdef __cplusplus
}
#endif
#endif
