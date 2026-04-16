/*****************************************************************************
 * File:    dma_channel_config.c
 * Target:  Infineon AURIX TC3xx
 * Purpose: Configure DMA Channel 0 for an 8-bit memory-to-memory transfer
 *          Source:      0x70000000
 *          Destination: 0xB0000000
 *          Data width:  8-bit (1 byte per DMA move)
 *          Mode:        Single Mode, software-triggered
 *
 * Assumptions:
 *   - iLLD (Infineon Low-Level Drivers) for TC3xx are included in the project.
 *   - DMA is already enabled after reset (DMA_CLC.DISS = 0 by default).
 *   - TREL = 1 (one transfer = one byte). Increase DMA_TRANSFER_COUNT for
 *     multi-byte transfers; each count moves 1 byte (8-bit data width).
 *   - Source/destination addresses must be byte-aligned for 8-bit transfers
 *     (Ref: User Manual Section 18.3.4.4.4 "Address Alignment", p.18-23).
 *
 * Register Reference (AURIX TC3xx User Manual, Chapter 18 - DMA, v2.0.0):
 *   DMA Base address:  0xF0010000  (Table 578, p.18-57)
 *
 *   Per-channel DMARAM registers (channel c, stride = 0x20):
 *     ADICRc   : 0xF0010000 + 0x2004 + c*0x20  (p.18-71..73)
 *     SADRc    : 0xF0010000 + 0x2008 + c*0x20  (p.18-70)
 *     DADRc    : 0xF0010000 + 0x200C + c*0x20  (p.18-70)
 *     CHCFGRc  : 0xF0010000 + 0x2014 + c*0x20  (p.18-75..76)
 *     SHADRc   : 0xF0010000 + 0x2018 + c*0x20  (p.18-77)
 *     CHCSRc   : 0xF0010000 + 0x201C + c*0x20  (p.18-77)
 *     TSRc     : MODULE_DMA.TSR[c]              (p.18-68)
 *
 *   Clock Control:
 *     CLC      : 0xF0010000 + 0x0000            (p.18-60, Section 18.4.2)
 *****************************************************************************/

#include "IfxDma_reg.h"       /* TC3xx DMA SFR definitions                  */
#include "IfxDma_bf.h"        /* TC3xx DMA bit-field definitions             */
#include <stdint.h>

/*---------------------------------------------------------------------------
 * Configuration constants
 *---------------------------------------------------------------------------*/
#define DMA_CHANNEL          (0U)            /* Use DMA channel 0            */
#define DMA_SOURCE_ADDR      (0x70000000UL)  /* Source start address         */
#define DMA_DEST_ADDR        (0xB0000000UL)  /* Destination start address    */
#define DMA_TRANSFER_COUNT   (1U)            /* Number of 8-bit transfers    */

/*---------------------------------------------------------------------------
 * Step 1 — Enable DMA clock (CLC register)
 *
 * Register: DMA_CLC (offset 0x0000, p.18-60, Section 18.4.2)
 *   DISR [bit 0] = 0B → DMA enable requested
 *   DISS [bit 1] = 0B → DMA is enabled (read-only status, set by hardware)
 *   EDIS [bit 3] = 1B → Sleep mode disabled (DMA shall not enter sleep)
 *
 * Note: After application reset, DMA is already enabled (DISS=0).
 *       This step ensures it is explicitly enabled and sleep is disabled.
 *---------------------------------------------------------------------------*/
static void DMA_EnableClock(void)
{
    /* Clear DISR (bit 0) to request DMA enable.                            */
    /* Set EDIS (bit 3) = 1 to prevent DMA entering sleep mode.            */
    /* DISR=0: DMA enable requested  (p.18-60)                             */
    /* EDIS=1: Sleep control disabled (p.18-60)                            */
    MODULE_DMA.CLC.U = (0U << IFX_DMA_CLC_DISR_OFF)   /* DISR = 0        */
                     | (1U << IFX_DMA_CLC_EDIS_OFF);   /* EDIS = 1        */

    /* Wait until DMA is enabled: DISS (bit 1) must read as 0.             */
    while (MODULE_DMA.CLC.B.DISS != 0U)
    {
        /* Spin until module is active.                                     */
    }
}

/*---------------------------------------------------------------------------
 * Step 2 — Configure DMA Channel 0 Transaction Control Set (TCS)
 *
 * The TCS is stored in DMARAM and defines every aspect of the DMA move:
 * source, destination, data width, transfer count, address stepping.
 * (Ref: Section 18.3.4 "DMA Random Access Memory", p.18-15)
 *---------------------------------------------------------------------------*/
static void DMA_ConfigureChannel(void)
{
    Ifx_DMA_CH *ch = &MODULE_DMA.CHANNEL[DMA_CHANNEL];  /* Channel pointer */

    /*-----------------------------------------------------------------------
     * 2a. Source Address Register — SADRc
     *
     * Register: SADRc at offset 2008H + c*20H  (p.18-70)
     * Field: SADR[31:0] = 32-bit source start address
     *
     * Alignment rule (p.18-23, Section 18.3.4.4.4):
     *   For 8-bit (CHDW=000B), source must be byte-aligned → any address OK.
     *-----------------------------------------------------------------------*/
    ch->SADR.U = DMA_SOURCE_ADDR;   /* SADR[31:0] = 0x70000000            */

    /*-----------------------------------------------------------------------
     * 2b. Destination Address Register — DADRc
     *
     * Register: DADRc at offset 200CH + c*20H  (p.18-70)
     * Field: DADR[31:0] = 32-bit destination start address
     *
     * Alignment rule: byte-aligned for 8-bit width → any address OK.
     *-----------------------------------------------------------------------*/
    ch->DADR.U = DMA_DEST_ADDR;     /* DADR[31:0] = 0xB0000000            */

    /*-----------------------------------------------------------------------
     * 2c. Address and Interrupt Control Register — ADICRc
     *
     * Register: ADICRc at offset 2004H + c*20H  (p.18-71..73)
     *
     * Source address control:
     *   SMF  [2:0] = 000B → address offset = 1 × CHDW = 1 byte  (p.18-71)
     *   INCS [3]   = 1B   → add offset after each move (increment) (p.18-71)
     *
     * Destination address control:
     *   DMF  [6:4] = 000B → address offset = 1 × CHDW = 1 byte  (p.18-72)
     *   INCD [7]   = 1B   → add offset after each move (increment) (p.18-72)
     *
     * Circular buffer (disabled):
     *   CBLS [11:8]  = 0H → no circular source buffer   (p.18-72)
     *   CBLD [15:12] = 0H → no circular dest buffer     (p.18-73)
     *   SCBE [20]    = 0B → source circular buffer off
     *   DCBE [21]    = 0B → dest   circular buffer off
     *
     * Shadow / interrupt:
     *   SHCT [19:16] = 0H → Move Operation (no shadow)  (p.18-73)
     *   INTCT[27:26] = 00B → no interrupt on completion
     *   IRDV [31:28] = 0H → interrupt at end of transaction (TCOUNT=0)
     *
     * Resulting raw value: 0x00000088
     *   bit3 (INCS)=1, bit7 (INCD)=1, all other fields = 0
     *-----------------------------------------------------------------------*/
    ch->ADICR.U =
        (0U << IFX_DMA_CH_ADICR_SMF_OFF)    /* SMF  = 000B: offset=1×CHDW  */
      | (1U << IFX_DMA_CH_ADICR_INCS_OFF)   /* INCS = 1:    src increment   */
      | (0U << IFX_DMA_CH_ADICR_DMF_OFF)    /* DMF  = 000B: offset=1×CHDW  */
      | (1U << IFX_DMA_CH_ADICR_INCD_OFF)   /* INCD = 1:    dst increment   */
      | (0U << IFX_DMA_CH_ADICR_CBLS_OFF)   /* CBLS = 0:    no circ src buf */
      | (0U << IFX_DMA_CH_ADICR_CBLD_OFF)   /* CBLD = 0:    no circ dst buf */
      | (0U << IFX_DMA_CH_ADICR_SHCT_OFF)   /* SHCT = 0:    move operation  */
      | (0U << IFX_DMA_CH_ADICR_SCBE_OFF)   /* SCBE = 0:    circ src off    */
      | (0U << IFX_DMA_CH_ADICR_DCBE_OFF);  /* DCBE = 0:    circ dst off    */

    /*-----------------------------------------------------------------------
     * 2d. Channel Configuration Register — CHCFGRc
     *
     * Register: CHCFGRc at offset 2014H + c*20H  (p.18-75..76)
     *
     *   TREL   [13:0]  = DMA_TRANSFER_COUNT
     *                    Transfer Reload Value: number of DMA transfers per
     *                    transaction. TCOUNT is loaded with TREL at start.
     *                    If TREL=0 or 1, ME loads TCOUNT=1 (p.18-75).
     *
     *   BLKM   [18:16] = 000B → 1 DMA move per transfer (single move mode)
     *                    (p.18-75): 000B = 1 move/transfer
     *
     *   RROAT  [19]    = 0B   → Reset Request after each Transfer (default)
     *                    TSR.CH cleared at start of each DMA transfer (p.18-75)
     *
     *   CHMODE [20]    = 0B   → Single Mode: TSR.HTRE reset after transaction
     *                    (p.18-76): SW must re-enable HW request for next run
     *
     *   CHDW   [23:21] = 000B → 8-bit data width per move  (p.18-76)
     *                    Note: Single Data Transfer Byte (SDTB)
     *
     *   PATSEL [26:24] = 000B → No pattern detection        (p.18-76)
     *   PRSEL  [28]    = 0B   → Hardware request source selected
     *                    (irrelevant here as we use SW request)
     *
     * Resulting raw value:
     *   TREL=1 → bits[13:0]=0x0001, CHDW=000B → bits[23:21]=0, rest=0
     *   Raw = 0x00000001
     *-----------------------------------------------------------------------*/
    ch->CHCFGR.U =
        ((DMA_TRANSFER_COUNT) << IFX_DMA_CH_CHCFGR_TREL_OFF)  /* TREL     */
      | (0U << IFX_DMA_CH_CHCFGR_BLKM_OFF)   /* BLKM  = 000B: 1 move/xfer */
      | (0U << IFX_DMA_CH_CHCFGR_RROAT_OFF)  /* RROAT = 0:    per-transfer */
      | (0U << IFX_DMA_CH_CHCFGR_CHMODE_OFF) /* CHMODE= 0:    single mode  */
      | (0U << IFX_DMA_CH_CHCFGR_CHDW_OFF)   /* CHDW  = 000B: 8-bit width  */
      | (0U << IFX_DMA_CH_CHCFGR_PATSEL_OFF) /* PATSEL= 000B: no pattern   */
      | (0U << IFX_DMA_CH_CHCFGR_PRSEL_OFF); /* PRSEL = 0:    HW request   */
}

/*---------------------------------------------------------------------------
 * Step 3 — Disable DMA hardware request and trigger software transfer
 *
 * Register: TSRc — Transaction State Register  (p.18-68)
 *   DCH [17] = 1B (write-only) → Disable DMA Channel Hardware Transaction
 *              Request. Ensures no spurious HW trigger starts the transfer.
 *              (Ref: Use Case code example, p.18-98, Section 18.6.1)
 *
 * Register: CHCSRc — Channel Control and Status Register  (p.18-77)
 *   SCH [31] = 1B (write-only) → Initiate DMA Software Request.
 *              Sets TSR.CH which triggers the DMA Move Engine.
 *              (Ref: Section 18.3.3.1, p.18-8; Use Case p.18-98)
 *---------------------------------------------------------------------------*/
static void DMA_StartTransfer(void)
{
    Ifx_DMA_CH *ch = &MODULE_DMA.CHANNEL[DMA_CHANNEL];

    /* Disable hardware requests on this channel before SW trigger.         */
    /* TSR.DCH (bit 17) = 1B — write-only, disables HW transaction request */
    /* (p.18-68, Section 18.4)                                             */
    MODULE_DMA.TSR[DMA_CHANNEL].U = (1U << IFX_DMA_TSR_DCH_OFF);

    /* Initiate a DMA Software Request to start the transfer.               */
    /* CHCSR.SCH (bit 31) = 1B — write-only, sets TSR.CH to start move.    */
    /* (p.18-77, CHCSR register; Use Case p.18-98)                         */
    ch->CHCSR.U = (1U << IFX_DMA_CH_CHCSR_SCH_OFF);
}

/*---------------------------------------------------------------------------
 * Public entry point — call this from your application initialisation.
 *
 * Initialization order (AURIX TC3xx DMA, Section 18.3.4, p.18-15):
 *   1. Enable DMA clock (CLC.DISR = 0)
 *   2. Configure channel TCS: SADR, DADR, ADICR, CHCFGR
 *   3. Disable HW request (TSR.DCH), then trigger SW request (CHCSR.SCH)
 *---------------------------------------------------------------------------*/
void DMA_Init_Channel0_8bit(void)
{
    DMA_EnableClock();      /* Step 1: Ensure DMA module is active          */
    DMA_ConfigureChannel(); /* Step 2: Set up TCS (addresses, width, count) */
    DMA_StartTransfer();    /* Step 3: Disable HW req, fire SW request      */
}
