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

#include "inc/tmf882x_host_interface.h"
#include "inc/tmf882x_clock_correction.h"

#define CLK_CORR_WRAPAROUND      0x80000000L        // 32 bits
#define CLK_CORR_MINCOUNT        3

void tmf882x_clk_corr_init(struct tmf882x_clk_corr * cr, uint32_t ratio)
{
    if (!cr || (ratio == 0)) return;
    //
    // start a new recalculation cycle, beginning with the default ratio
    //
    cr->first_ref = 0;
    cr->last_ref  = 0;
    cr->first_src  = 0;
    cr->last_src   = 0;
    cr->ratio      = ratio;
    cr->iratioQ15  = (1 << 15) / ratio;
    cr->count      = 0;
}

void tmf882x_clk_corr_recalc(struct tmf882x_clk_corr * cr)
{
    if (!cr) return;
    //
    // start a new recalculation cycle, but don't lose the latest ratio
    //
    cr->count      = 0;
}

void tmf882x_clk_corr_addpair(struct tmf882x_clk_corr * cr, uint32_t ref, uint32_t src)
{
    uint32_t num = 0;
    uint32_t den = 0;
    uint32_t denQM15 = 0;
    if (!cr) return;
    //
    // add a pair of ref and src times
    //
    if (src == 0) {
        return;
    }

    if (ref <= cr->last_ref) {
        //this should not be possible since ref count reference is unix_epoch
        cr->count = 0;
    }

    if (src <= cr->last_src) {
        //wraparound case
        cr->count = 0;
    }

    if (cr->count == 0) {
        cr->first_ref = ref;
        cr->first_src = src;
    }

    cr->last_ref  =  ref;
    cr->last_src  =  src;
    cr->count    +=  1;

    if (cr->count >= CLK_CORR_MINCOUNT) {

        num = cr->last_ref - cr->first_ref;
        den = cr->last_src - cr->first_src;

        while ((num < CLK_CORR_WRAPAROUND) && (den < CLK_CORR_WRAPAROUND)) {
            num <<= 1;
            den <<= 1;
        };

        denQM15 = ((den + (1 << 14)) >> 15);   // round up, always positive

        if ((denQM15 == 0) || (num > den)) {
            cr->count = 0;
        } else {
            cr->iratioQ15 = (num + (denQM15 >> 1))/denQM15;
        }
    }
}

uint32_t tmf882x_clk_corr_map(struct tmf882x_clk_corr * cr, uint32_t old_val)
{
    if (!cr) return old_val;
    //
    // apply the mapping function to calculate a clock-corrected distance
    //
    return (old_val * cr->ratio * cr->iratioQ15 + (1 << 14)) >> 15;
}

