/***
**************************************************************************************
 * Interface to CDR Registers
 **************************************************************************************
 */

#include <string.h>
#include "types.h"
#include "mdio.h"
#include "cdr.h"
#include "interpreter.h"
#include "hex.h"
#include "console.h"
#include "timer.h"
#include "tpos.h"
#include "pvar.h"
#include "iodefine.h"
#include "interrupt_handlers.h"

#define CDR_IPL		(1)
/* Pin definitions */
#define CDR_RESET_DIR PORTE.PDR.BIT.B3
#define CDR_RESET_DR  PORTE.PODR.BIT.B3

/* Register definitions for INPHY-100 CDR */
#define CDR_VS_DEVICE_CONTROL                   0
#define CDR_VS_DEVICE_IDENTIFIER2               2
#define CDR_VS_DEVICE_IDENTIFIER3               3
#define CDR_PMA_PMD_DEVICES_IN_PACKAGE5         5
#define CDR_PMA_PMD_DEVICES_IN_PACKAGE6         6
#define CDR_VS_STATUS                           8
#define CDR_VS_PACKAGE_ID14                     0xe     // read as 0
#define CDR_VS_PACKAGE_ID15                     0xf
#define CDR_LANE_PATTERN_CONTROL0               16      
#define CDR_LANE_PATTERN_CONTROL1               17
#define CDR_LANE_PATTERN_CONTROL2               18
#define CDR_LANE_PATTERN_CONTROL3               19
#define CDR_CUSTOM_TEST_PATTERN30               30
#define CDR_CUSTOM_TEST_PATTERN31               31
#define CDR_CUSTOM_TEST_PATTERN32               32
#define CDR_LOL_STATUS                          33
#define CDR_FIFO_STATUS                         37
#define CDR_TXPLL_CONFIG1                       39      
#define CDR_TXPLL_CONFIG2                       40
#define CDR_RXPLL_CONFIG1                       41
#define CDR_RXPLL_CONFIG2                       42
#define CDR_MANUAL_RESET_CONTROL                44
#define CDR_LANE0_ERROR_COUNTER                 48
#define CDR_LANE1_ERROR_COUNTER                 49
#define CDR_LANE2_ERROR_COUNTER                 50
#define CDR_LANE3_ERROR_COUNTER                 51
#define CDR_LANE0_SAMPLE0_READ                  64
#define CDR_LANE0_SAMPLE1_READ                  65      
#define CDR_LANE0_SAMPLE2_READ                  66
#define CDR_LANE1_SAMPLE0_READ                  0x44    
#define CDR_LANE1_SAMPLE1_READ                  0x45
#define CDR_LANE1_SAMPLE2_READ                  0x46
#define CDR_LANE2_SAMPLE0_READ                  0x48
#define CDR_LANE2_SAMPLE1_READ                  0x49
#define CDR_LANE2_SAMPLE2_READ                  0x4a
#define CDR_LANE3_SAMPLE0_READ                  0x4c
#define CDR_LANE3_SAMPLE1_READ                  0x4d
#define CDR_LANE3_SAMPLE2_READ                  0x4e
/* 0x100-0x17f, 0x200- 0x27f , 0x300 - 0x37f , 0x400 - 0x47f Transmitter register spaces */
#define CDR_TX_MAIN_CONTROL(lane)               (0x100 + (0x100 * (lane)))
#define CDR_TX_EQ_CONTROL(lane)                 (0x101 + (0x100 * (lane)))
#define CDR_TX_SWING_CONTROL(lane)              (0x102 + (0x100 * (lane)))
/* Receive lane register spaces */
#define CDR_RX_MAIN_CONTROL(lane)               (384 + (0x100 * (lane)))
#define CDR_RX_1ST_ORDER_CONTROL(lane)          (385 + (0x100 * (lane)))
#define CDR_RX_2ND_ORDER_CONTROL(lane)          (386 + (0x100 * (lane)))
#define CDR_RX_EQ_CONTROL(lane)                 (387 + (0x100 * (lane)))
#define CDR_RX_EYESCAN_CONTROL1(lane)           (391 + (0x100 * (lane)))
#define CDR_RX_EYESCAN_CONTROL2(lane)           (392 + (0x100 * (lane)))
#define CDR_RX_EYESCAN_RESULT(lane)             (393 + (0x100 * (lane)))
#define CDR_RX_EQ_CONTROL2(lane)                (420 + (0x100 * (lane)))
#define CDR_RX_EQ_OBSERVE1(lane)                (421 + (0x100 * (lane)))
#define CDR_RX_EQ_SETTING(lane)                 (422 + (0x100 * (lane)))
#define CDR_RX_2ND_ORDER_CDR_OBSERVE(lane)      (423 + (0x100 * (lane)))

/* The INPHY CDR's are of devive type 0x30 which means "Vendor specific" */
#define DEVTYPE		(30)
typedef struct CDR {
	uint64_t berCntr[4];
	uint8_t phyAddr;
} CDR;

static CDR gCDR[2];

typedef struct CdrRegister {
	const char *name;
	uint16_t regNo;
	uint8_t lastBit;
	uint8_t firstBit;
} CdrRegister;

/**
 ******************************************************************
 * This is the list of the exported CDR registers with name
 * and address.  
 ******************************************************************
 */
static const CdrRegister gCdrRegister[] = {
	/* Register 16-19 */
	{
	 	.name = "l0.prbs_lock",
	 	.regNo = 16,
	 	.lastBit = 15,
	 	.firstBit = 15,
	},
	{
	 	.name = "l0.prbs_autovr",
	 	.regNo = 16,
	 	.lastBit = 14,
	 	.firstBit = 14,
	},
	{
	 	.name = "l0.Loopback_en",
	 	.regNo = 16,
	 	.lastBit = 13,
	 	.firstBit = 13,
	},
	{
	 	.name = "l0.pat_ver_en",
	 	.regNo = 16,
	 	.lastBit = 12,
	 	.firstBit = 12,
	},
	{
	 	.name = "l0.prbs_ver_inv",
	 	.regNo = 16,
	 	.lastBit = 11,
	 	.firstBit = 11,
	},
	{
	 	.name = "l0.pat_ver_sel",
	 	.regNo = 16,
	 	.lastBit = 10,
	 	.firstBit = 8,
	},
	{
	 	.name = "l0.odb_en",
	 	.regNo = 16,
	 	.lastBit = 7,
	 	.firstBit = 7,
	},
	{
	 	.name = "l0.tx_disable",
	 	.regNo = 16,
	 	.lastBit = 6,
	 	.firstBit = 6,
	},
	{
	 	.name = "l0.error_insert",
	 	.regNo = 16,
	 	.lastBit = 5,
	 	.firstBit = 5,
	},
	{
	 	.name = "l0.pat_gen_en",
	 	.regNo = 16,
	 	.lastBit = 4,
	 	.firstBit = 4,
	},
	{
	 	.name = "l0.prbs_gen_inv",
	 	.regNo = 16,
	 	.lastBit = 3,
	 	.firstBit = 3,
	},
	{
	 	.name = "l0.pat_gen_sel",
	 	.regNo = 16,
	 	.lastBit = 2,
	 	.firstBit = 0,
	},
	/* Lane 1 */
	{
	 	.name = "l1.prbs_lock",
	 	.regNo = 17,
	 	.lastBit = 15,
	 	.firstBit = 15,
	},
	{
	 	.name = "l1.prbs_autovr",
	 	.regNo = 17,
	 	.lastBit = 14,
	 	.firstBit = 14,
	},
	{
	 	.name = "l1.Loopback_en",
	 	.regNo = 17,
	 	.lastBit = 13,
	 	.firstBit = 13,
	},
	{
	 	.name = "l1.pat_ver_en",
	 	.regNo = 17,
	 	.lastBit = 12,
	 	.firstBit = 12,
	},
	{
	 	.name = "l1.prbs_ver_inv",
	 	.regNo = 17,
	 	.lastBit = 11,
	 	.firstBit = 11,
	},
	{
	 	.name = "l1.pat_ver_sel",
	 	.regNo = 17,
	 	.lastBit = 10,
	 	.firstBit = 8,
	},
	{
	 	.name = "l1.odb_en",
	 	.regNo = 17,
	 	.lastBit = 7,
	 	.firstBit = 7,
	},
	{
	 	.name = "l1.tx_disable",
	 	.regNo = 17,
	 	.lastBit = 6,
	 	.firstBit = 6,
	},
	{
	 	.name = "l1.error_insert",
	 	.regNo = 17,
	 	.lastBit = 5,
	 	.firstBit = 5,
	},
	{
	 	.name = "l1.pat_gen_en",
	 	.regNo = 17,
	 	.lastBit = 4,
	 	.firstBit = 4,
	},
	{
	 	.name = "l1.prbs_gen_inv",
	 	.regNo = 17,
	 	.lastBit = 3,
	 	.firstBit = 3,
	},
	{
	 	.name = "l1.pat_gen_sel",
	 	.regNo = 17,
	 	.lastBit = 2,
	 	.firstBit = 0,
	},
	/* Lane 2 */
	{
	 	.name = "l2.prbs_lock",
	 	.regNo = 18,
	 	.lastBit = 15,
	 	.firstBit = 15,
	},
	{
	 	.name = "l2.prbs_autovr",
	 	.regNo = 18,
	 	.lastBit = 14,
	 	.firstBit = 14,
	},
	{
	 	.name = "l2.Loopback_en",
	 	.regNo = 18,
	 	.lastBit = 13,
	 	.firstBit = 13,
	},
	{
	 	.name = "l2.pat_ver_en",
	 	.regNo = 18,
	 	.lastBit = 12,
	 	.firstBit = 12,
	},
	{
	 	.name = "l2.prbs_ver_inv",
	 	.regNo = 18,
	 	.lastBit = 11,
	 	.firstBit = 11,
	},
	{
	 	.name = "l2.pat_ver_sel",
	 	.regNo = 18,
	 	.lastBit = 10,
	 	.firstBit = 8,
	},
	{
	 	.name = "l2.odb_en",
	 	.regNo = 18,
	 	.lastBit = 7,
	 	.firstBit = 7,
	},
	{
	 	.name = "l2.tx_disable",
	 	.regNo = 18,
	 	.lastBit = 6,
	 	.firstBit = 6,
	},
	{
	 	.name = "l2.error_insert",
	 	.regNo = 18,
	 	.lastBit = 5,
	 	.firstBit = 5,
	},
	{
	 	.name = "l2.pat_gen_en",
	 	.regNo = 18,
	 	.lastBit = 4,
	 	.firstBit = 4,
	},
	{
	 	.name = "l2.prbs_gen_inv",
	 	.regNo = 18,
	 	.lastBit = 3,
	 	.firstBit = 3,
	},
	{
	 	.name = "l2.pat_gen_sel",
	 	.regNo = 18,
	 	.lastBit = 2,
	 	.firstBit = 0,
	},
	/* Lane 3 */
	{
	 	.name = "l3.prbs_lock",
	 	.regNo = 19,
	 	.lastBit = 15,
	 	.firstBit = 15,
	},
	{
	 	.name = "l3.prbs_autovr",
	 	.regNo = 19,
	 	.lastBit = 14,
	 	.firstBit = 14,
	},
	{
	 	.name = "l3.Loopback_en",
	 	.regNo = 19,
	 	.lastBit = 13,
	 	.firstBit = 13,
	},
	{
	 	.name = "l3.pat_ver_en",
	 	.regNo = 19,
	 	.lastBit = 12,
	 	.firstBit = 12,
	},
	{
	 	.name = "l3.prbs_ver_inv",
	 	.regNo = 19,
	 	.lastBit = 11,
	 	.firstBit = 11,
	},
	{
	 	.name = "l3.pat_ver_sel",
	 	.regNo = 19,
	 	.lastBit = 10,
	 	.firstBit = 8,
	},
	{
	 	.name = "l3.odb_en",
	 	.regNo = 19,
	 	.lastBit = 7,
	 	.firstBit = 7,
	},
	{
	 	.name = "l3.tx_disable",
	 	.regNo = 19,
	 	.lastBit = 6,
	 	.firstBit = 6,
	},
	{
	 	.name = "l3.error_insert",
	 	.regNo = 19,
	 	.lastBit = 5,
	 	.firstBit = 5,
	},
	{
	 	.name = "l3.pat_gen_en",
	 	.regNo = 19,
	 	.lastBit = 4,
	 	.firstBit = 4,
	},
	{
	 	.name = "l3.prbs_gen_inv",
	 	.regNo = 19,
	 	.lastBit = 3,
	 	.firstBit = 3,
	},
	{
	 	.name = "l3.pat_gen_sel",
	 	.regNo = 19,
	 	.lastBit = 2,
	 	.firstBit = 0,
	},
	{
		.name = "custom_tst_patL",
		.regNo = 30,
		.lastBit = 15,
		.firstBit = 0,
	},
	{
		.name = "custom_tst_patM",
		.regNo = 31,
		.lastBit = 15,
		.firstBit = 0,
	},
	{
		.name = "custom_tst_patH",
		.regNo = 32,
		.lastBit = 7,
		.firstBit = 0,
	},
	{
		.name = "l0.no_prbs_lck",
		.regNo = 33,
		.firstBit = 0,
		.lastBit = 0,
	},
	{
		.name = "l1.no_prbs_lck",
		.regNo = 33,
		.firstBit = 1,
		.lastBit = 1,
	},
	{
		.name = "l2.no_prbs_lck",
		.regNo = 33,
		.firstBit = 2,
		.lastBit = 2,
	},
	{
		.name = "l3.no_prbs_lck",
		.regNo = 33,
		.firstBit = 3,
		.lastBit = 3,
	},
	{
		.name = "l0.no_prot_lck",
		.regNo = 33,
		.firstBit = 4,
		.lastBit = 4,
	},
	{
		.name = "l1.no_prot_lck",
		.regNo = 33,
		.firstBit = 5,
		.lastBit = 5,
	},
	{
		.name = "l2.no_prot_lck",
		.regNo = 33,
		.firstBit = 6,
		.lastBit = 6,
	},
	{
		.name = "l3.no_prot_lck",
		.regNo = 33,
		.firstBit = 6,
		.lastBit = 6,
	},
	{
		.name = "l0.lol_stat",
		.regNo = 33,
		.firstBit = 8,
		.lastBit = 8,
	},
	{
		.name = "l1.lol_stat",
		.regNo = 33,
		.firstBit = 9,
		.lastBit = 9,
	},
	{
		.name = "l2.lol_stat",
		.regNo = 33,
		.firstBit = 10,
		.lastBit = 10,
	},
	{
		.name = "l3.lol_stat",
		.regNo = 33,
		.firstBit = 11,
		.lastBit = 11,
	},
	{
		.name = "l0.latched_lol",
		.regNo = 33,
		.firstBit = 12,
		.lastBit = 12,
	},
	{
		.name = "l1.latched_lol",
		.regNo = 33,
		.firstBit = 13,
		.lastBit = 13,
	},
	{
		.name = "l2.latched_lol",
		.regNo = 33,
		.firstBit = 14,
		.lastBit = 14,
	},
	{
		.name = "l3.latched_lol",
		.regNo = 33,
		.firstBit = 15,
		.lastBit = 15,
	},
	/* FIFO underrun register */
	{
		.name = "l0.fifo_urun",
		.regNo = 37,
		.firstBit = 0,
		.lastBit = 0,
	},
	{
		.name = "l1.fifo_urun",
		.regNo = 37,
		.firstBit = 1,
		.lastBit = 1,
	},
	{
		.name = "l2.fifo_urun",
		.regNo = 37,
		.firstBit = 2,
		.lastBit = 2,
	},
	{
		.name = "l3.fifo_urun",
		.regNo = 37,
		.firstBit = 3,
		.lastBit = 3,
	},
	{
		.name = "l0.fifo_orun",
		.regNo = 37,
		.firstBit = 4,
		.lastBit = 4,
	},
	{
		.name = "l1.fifo_orun",
		.regNo = 37,
		.firstBit = 5,
		.lastBit = 5,
	},
	{
		.name = "l2.fifo_orun",
		.regNo = 37,
		.firstBit = 6,
		.lastBit = 6,
	},
	{
		.name = "l3.fifo_orun",
		.regNo = 37,
		.firstBit = 7,
		.lastBit = 7,
	},
	{
		.name = "l0.FIFO_error",
		.regNo = 37,
		.firstBit = 8,
		.lastBit = 8,
	},
	{
		.name = "l1.FIFO_error",
		.regNo = 37,
		.firstBit = 9,
		.lastBit = 9,
	},
	{
		.name = "l2.FIFO_error",
		.regNo = 37,
		.firstBit = 10,
		.lastBit = 10,
	},
	{
		.name = "l3.FIFO_error",
		.regNo = 37,
		.firstBit = 11,
		.lastBit = 11,
	},
	{
		.name = "reset_on_lol",
		.regNo = 37,
		.firstBit = 13,
		.lastBit = 13,
	},
	{
		.name = "reset_on_err",
		.regNo = 37,
		.firstBit = 14,
		.lastBit = 14,
	},
	{
		.name = "fifo_reset",
		.regNo = 37,
		.firstBit = 15,
		.lastBit = 15,
	},
	{
		.name = "txpll_cal_bus_force_fine",
		.regNo = 39,
		.firstBit = 0,
		.lastBit = 2,
	},
	{
		.name = "txpll_cal_bus_force_coarse",
		.regNo = 39,
		.firstBit = 3,
		.lastBit = 7,
	},
	{
		.name = "txpll_force_cal_bus",
		.regNo = 39,
		.firstBit = 8,
		.lastBit = 8,
	},
	{
		.name = "txpll_charge_pump_curr",
		.regNo = 39,
		.firstBit = 9,
		.lastBit = 11, 
	},
	{
		.name = "txpll_cal_step",
		.regNo = 39,
		.firstBit = 13,
		.lastBit = 14, 
	},
	{
		.name = "txpll_recalib",
		.regNo = 39,
		.firstBit = 15,
		.lastBit = 15, 
	},
	{
		.name = "txpll_cal_read_fine",
		.regNo = 40,
		.firstBit = 0,
		.lastBit = 2,
	},
	{
		.name = "txpll_cal_read_coarse",
		.regNo = 40,
		.firstBit = 3,
		.lastBit = 7,
	},
	{
		.name = "txpll_lock",
		.regNo = 40,
		.firstBit = 8,
		.lastBit = 8,
	},
	{
		.name = "txpll_lco_ampl",
		.regNo = 40,
		.firstBit = 11, 
		.lastBit = 13,
	},
	{
		.name = "txpll_vctlref",
		.regNo = 40,
		.firstBit = 14, 
		.lastBit = 15,
	},
	{
		.name = "rxpll_cal_bus_force_fine",
		.regNo = 41,
		.firstBit = 0,
		.lastBit = 2,
	},
	{
		.name = "rxpll_cal_bus_force_coarse",
		.regNo = 41,
		.firstBit = 3,
		.lastBit = 7,
	},
	{
		.name = "rxpll_force_cal_bus",
		.regNo = 41,
		.firstBit = 8,
		.lastBit = 8,
	},
	{
		.name = "rxpll_charge_pump_curr",
		.regNo = 41,
		.firstBit = 9,
		.lastBit = 11, 
	},
	{
		.name = "rxpll_cal_step",
		.regNo = 41,
		.firstBit = 13,
		.lastBit = 14, 
	},
	{
		.name = "rxpll_recalib",
		.regNo = 41,
		.firstBit = 15,
		.lastBit = 15, 
	},
	{
		.name = "rxpll_cal_read_fine",
		.regNo = 42,
		.firstBit = 0,
		.lastBit = 2,
	},
	{
		.name = "rxpll_cal_read_coarse",
		.regNo = 42,
		.firstBit = 3,
		.lastBit = 7,
	},
	{
		.name = "rxpll_lock",
		.regNo = 42,
		.firstBit = 8,
		.lastBit = 8,
	},
	{
		.name = "rxpll_lco_ampl",
		.regNo = 42,
		.firstBit = 11, 
		.lastBit = 13,
	},
	{
		.name = "rxpll_vctlref",
		.regNo = 42,
		.firstBit = 14, 
		.lastBit = 15,
	},
//CDR_MANUAL_RESET_CONTROL                44
	{
		.name = "l0.err_cntr",
		.regNo = 48,
		.firstBit = 0,
		.lastBit = 15,
	},
	{
		.name = "l1.err_cntr",
		.regNo = 49,
		.firstBit = 0,
		.lastBit = 15,
	},
	{
		.name = "l2.err_cntr",
		.regNo = 50,
		.firstBit = 0,
		.lastBit = 15,
	},
	{
		.name = "l3.err_cntr",
		.regNo = 51,
		.firstBit = 0,
		.lastBit = 15,
	}
};

#if 0
#define CDR_LANE0_SAMPLE0_READ                  64
#define CDR_LANE0_SAMPLE1_READ                  65      
#define CDR_LANE0_SAMPLE2_READ                  66
#define CDR_LANE1_SAMPLE0_READ                  0x44    
#define CDR_LANE1_SAMPLE1_READ                  0x45
#define CDR_LANE1_SAMPLE2_READ                  0x46
#define CDR_LANE2_SAMPLE0_READ                  0x48
#define CDR_LANE2_SAMPLE1_READ                  0x49
#define CDR_LANE2_SAMPLE2_READ                  0x4a
#define CDR_LANE3_SAMPLE0_READ                  0x4c
#define CDR_LANE3_SAMPLE1_READ                  0x4d
#define CDR_LANE3_SAMPLE2_READ                  0x4e
#endif

/**
 ****************************************************************************+
 * Lane specific RX/TX registers with 256 byte spacing
 ****************************************************************************+
 */
static const CdrRegister gCdrLaneRegister[] = 
{
	{
		.name = "Swap_TXP_N",
		.regNo = 256,
		.lastBit = 0,
		.firstBit = 0,
	},
	{
		.name = "LoopbackOE",
		.regNo = 256,
		.lastBit = 1,
		.firstBit = 1,
	},
	{
		.name = "txa_eqpst",
		.regNo = 257,
		.lastBit = 10,
		.firstBit = 8,
	},
	{
		.name = "txa_eqpre",
		.regNo = 257,
		.lastBit = 1,
		.firstBit = 0,
	},
	{
		.name = "txa_swing",
		.regNo = 258,
		.firstBit = 0,
		.lastBit = 2,
	},
	{
		.name = "eq_state",
		.regNo = 421,
		.firstBit = 0,
		.lastBit = 3,
	},
	{
		.name = "cdr2_trip",
		.regNo = 423,
		.firstBit = 0,
		.lastBit = 13,
	},
	{
		.name = "pi_pos1_pisel",
		.regNo = 424,
		.firstBit = 12,
		.lastBit = 13,
	},
	{
		.name = "pi_pos1_piquadr",
		.regNo = 424,
		.firstBit = 8,
		.lastBit = 9,
	},
	{
		.name = "pi_pos1_picode",
		.regNo = 424,
		.firstBit = 0,
		.lastBit = 7,
	},
	{
		.name = "sec_order_state",
		.regNo = 426,
		.firstBit = 0,
		.lastBit = 4,
	},
};

/**
 ************************************************************************
 * \fn void Cdr_WriteReg(uint8_t phyAddr,uint16_t regAddr,uint16_t value)
 * Write to the CDR by setting up the Address with the MDIO Address cmd.
 * and then writing a 16bit word.
 ************************************************************************
 */
static void
Cdr_WriteReg(uint8_t phyAddr, uint16_t regAddr, uint16_t value)
{
	Flags_t flags;
        SAVE_FLAGS_SET_IPL(flags, CDR_IPL);
	MDIO_Address(phyAddr, DEVTYPE, regAddr);
	MDIO_Write(phyAddr, DEVTYPE, value);
	RESTORE_FLAGS(flags);
}

/**
 ************************************************************************
 * \fn void Cdr_ReadReg(uint8_t phyAddr,uint16_t regAddr,uint16_t value)
 * Write to the CDR by setting up the Address with the MDIO Address cmd.
 * and then writing a 16bit word.
 ************************************************************************
 */
static uint16_t
Cdr_ReadReg(uint8_t phyAddr, uint16_t regAddr)
{
	Flags_t flags;
	uint16_t regVal;
        SAVE_FLAGS_SET_IPL(flags, CDR_IPL);
	MDIO_Address(phyAddr, DEVTYPE, regAddr);
	regVal = MDIO_Read(phyAddr, DEVTYPE);
	RESTORE_FLAGS(flags);
	return regVal;
}

/*
 * Same but from interrupt handler
 */
INLINE uint16_t
Cdr_ReadIrq(uint8_t phyAddr, uint16_t regAddr)
{
	MDIO_Address(phyAddr, DEVTYPE, regAddr);
	return MDIO_Read(phyAddr, DEVTYPE);
}


/**
 ************************************************************************
 * \fn void Cdr_Init_cdr(uint16_t phy_addr=1)
 * from Python
 ************************************************************************
 */
static void
Cdr_InitCdr(uint16_t phy_addr)
{
	Cdr_WriteReg(phy_addr, 0, 0X0100);
	//Cdr_WriteReg(phy_addr, 2, 0X0210);
	//Cdr_WriteReg(phy_addr, 3, 0X7401);
	Cdr_WriteReg(phy_addr, 5, 0X0000);
	Cdr_WriteReg(phy_addr, 6, 0X4000);
	Cdr_WriteReg(phy_addr, 8, 0X8000);
	Cdr_WriteReg(phy_addr, 16, 0X1414);
	Cdr_WriteReg(phy_addr, 17, 0X1414);
	Cdr_WriteReg(phy_addr, 18, 0X1414);
	Cdr_WriteReg(phy_addr, 19, 0X1414);
	Cdr_WriteReg(phy_addr, 30, 0X0000);
	Cdr_WriteReg(phy_addr, 31, 0X0000);
	Cdr_WriteReg(phy_addr, 32, 0X0000);
	Cdr_WriteReg(phy_addr, 33, 0XFFFF);
	Cdr_WriteReg(phy_addr, 37, 0X4700);
	Cdr_WriteReg(phy_addr, 38, 0X0000);
	Cdr_WriteReg(phy_addr, 44, 0XFC00);
	Cdr_WriteReg(phy_addr, 45, 0X0000);
	Cdr_WriteReg(phy_addr, 48, 0X0000);
	Cdr_WriteReg(phy_addr, 49, 0X0000);
	Cdr_WriteReg(phy_addr, 50, 0X0000);
	Cdr_WriteReg(phy_addr, 51, 0X0000);
	Cdr_WriteReg(phy_addr, 256, 0X0006);
	Cdr_WriteReg(phy_addr, 512, 0X0006);
	Cdr_WriteReg(phy_addr, 768, 0X0006);
	Cdr_WriteReg(phy_addr, 1024, 0X0006);
	Cdr_WriteReg(phy_addr, 257, 0X0000);
	Cdr_WriteReg(phy_addr, 513, 0X0000);
	Cdr_WriteReg(phy_addr, 769, 0X0000);
	Cdr_WriteReg(phy_addr, 1025, 0X0000);
	Cdr_WriteReg(phy_addr, 258, 0X0000);
	Cdr_WriteReg(phy_addr, 514, 0X0000);
	Cdr_WriteReg(phy_addr, 770, 0X0000);
	Cdr_WriteReg(phy_addr, 1026, 0X0000);
	Cdr_WriteReg(phy_addr, 384, 0X0004);
	Cdr_WriteReg(phy_addr, 640, 0X0004);
	Cdr_WriteReg(phy_addr, 896, 0X0004);
	Cdr_WriteReg(phy_addr, 1152, 0X0004);
	Cdr_WriteReg(phy_addr, 387, 0X8405);
	Cdr_WriteReg(phy_addr, 643, 0X8405);
	Cdr_WriteReg(phy_addr, 899, 0X8405);
	Cdr_WriteReg(phy_addr, 1155, 0X8405);
	Cdr_WriteReg(phy_addr, 416, 0X0000);
	Cdr_WriteReg(phy_addr, 672, 0X0000);
	Cdr_WriteReg(phy_addr, 928, 0X0000);
	Cdr_WriteReg(phy_addr, 1184, 0X0002);
	Cdr_WriteReg(phy_addr, 421, 0X000E);
	Cdr_WriteReg(phy_addr, 677, 0X000E);
	Cdr_WriteReg(phy_addr, 933, 0X000E);
	Cdr_WriteReg(phy_addr, 1189, 0X000E);
	Cdr_WriteReg(phy_addr, 422, 0X0000);
	Cdr_WriteReg(phy_addr, 678, 0X0000);
	Cdr_WriteReg(phy_addr, 934, 0X0000);
	Cdr_WriteReg(phy_addr, 1190, 0X0000);
	Cdr_WriteReg(phy_addr, 423, 0X2043);
	Cdr_WriteReg(phy_addr, 679, 0X2029);
	Cdr_WriteReg(phy_addr, 935, 0X1FFA);
	Cdr_WriteReg(phy_addr, 1191, 0X2000);
	Cdr_WriteReg(phy_addr, 424, 0X025B);
	Cdr_WriteReg(phy_addr, 680, 0X022B);
	Cdr_WriteReg(phy_addr, 936, 0X0359);
	Cdr_WriteReg(phy_addr, 1192, 0X03E2);
}

/**
************************************************************************
* \fn void iShuffleRead(unsigned* uOutp, unsigned uInp,unsigned uStartBit,unsigned uNumBit)
{
	* from Python Liest uNumBit aus  uInp von uStartBit
************************************************************************
*/

static uint16_t
iShuffleRead(uint16_t uInp, uint8_t uStartBit, uint8_t uNumBit)
{
	uint16_t uMask;
	if (uStartBit + uNumBit > 16) {
		Con_Printf("CDR Bug, startbit + numbit > 16, %u + %u \n",uStartBit,uNumBit);
		return 0;
	}
	uMask = UINT16_C(0xffff) >> (16 - uNumBit);
	uInp >>= uStartBit;
	return uInp & uMask;
}

/**
************************************************************************
* \fn void iShuffle(uint16_t* uOutp, uint16_t uInp,uint8_t uStartBit,uint8_t uNumBit,uint8_t uNumber)
{
* from Python: Ersetzt uNumBit in  uInp von uStartBit auf uNumber
************************************************************************
*/

static void
iShuffle(uint16_t * uOutp, uint16_t uInp, uint8_t uStartBit, uint8_t uNumBit, uint16_t uNumber)
{
	uint16_t uMask = 0;
	if (uStartBit + uNumBit > 16) {
		Con_Printf("CDR Bug, startbit + numbit > 16, %u + %u \n",uStartBit,uNumBit);
		return;
	}
	uMask = UINT16_C(0xffff) >> (16 - uNumBit);
	uMask <<= uStartBit;
	uNumber <<= uStartBit;
	*uOutp = (uInp & (~uMask)) | uNumber;
}

/**
************************************************************************
* \fn void Cdr_WritePart(uint16_t phy_addr,uint16_t uRegister,uint8_t uNumBit,uint8_t uStartBit,uint8_t uNumber)
* from Python startup_cdr.py
************************************************************************
*/

static void
Cdr_WritePart(uint16_t phy_addr, uint16_t uRegister, uint8_t uLastBit, uint8_t uStartBit, uint16_t uNumber)
{
	uint16_t uNumBit;
	uint16_t uVal;
	uNumBit = uLastBit - uStartBit + 1;
	uVal = Cdr_ReadReg(phy_addr, uRegister);
	iShuffle(&uVal, uVal, uStartBit, uNumBit, uNumber);
	Cdr_WriteReg(phy_addr, uRegister, uVal);
}

/**
************************************************************************
* \fn void Cdr_ReadPart(uint16_t phy_addr,uint16_t uRegister,uint8_t uNumBit,uint8_t uStartBit)
* from Python startup_cdr.py
************************************************************************
*/

static uint16_t
Cdr_ReadPart(uint16_t phy_addr, uint16_t uRegister, uint8_t uLastBit, uint8_t uStartBit)
{
	uint16_t uNumBit;
	uint16_t uVal;
	uNumBit = uLastBit - uStartBit + 1;
	uVal = Cdr_ReadReg(phy_addr, uRegister);
	uVal = iShuffleRead(uVal, uStartBit, uNumBit);
	return uVal;
}

/**
 *********************************************************************
 *  Function visible to outsize has a capital "CDR" in the name
 *********************************************************************
 */
uint16_t
CDR_Read(uint8_t cdrId,uint32_t regCode) 
{
	uint32_t phyAddr = gCDR[cdrId & 1].phyAddr;
	uint32_t uRegister, uLastBit, uFirstBit;
	uRegister = regCode >> 16;
	uFirstBit = (regCode >> 4) & 0xf;
	uLastBit =  regCode & 0xf;
	//Con_Printf("CDR %lu reg %lu, from %u to %u \n",phyAddr,uRegister,uFirstBit,uLastBit);
	return Cdr_ReadPart(phyAddr, uRegister, uLastBit, uFirstBit);
}

void
CDR_Write(uint8_t cdrId,uint32_t regCode,uint16_t value) 
{
	uint32_t phyAddr = gCDR[cdrId & 1].phyAddr;
	uint32_t uRegister, uLastBit, uFirstBit;
	uRegister = regCode >> 16;
	uFirstBit = (regCode >> 4) & 0xf;
	uLastBit =  regCode & 0xf;
	Cdr_WritePart(phyAddr,uRegister,uLastBit, uFirstBit, value);
}
/**
 ************************************************************
 * \fn static void Cdr_SoftReset(uint16_t phy_addr) 
 ************************************************************
 */
static void 
Cdr_SoftReset(uint16_t phy_addr) 
{
	Cdr_WriteReg(phy_addr, CDR_VS_DEVICE_CONTROL, 0x1020);	//Hard reset (bit 5) and MDIO init (bit 12)
	SleepMs(2);
}

/**
************************************************************************
* \fn void Cdr_Recalibrate(uint16_t phy_addr=1)
* from Python startup_cdr.py
************************************************************************
*/

static uint8_t
Cdr_Recalibrate(uint16_t phy_addr)	// Olga
{
	/*  Startup for CDR A0 and B0 */
	uint16_t uVal;
	uint8_t status = 1;
	uint8_t lane;
	uVal = Cdr_ReadPart(phy_addr, CDR_VS_DEVICE_IDENTIFIER3, 15, 4);
	if (uVal != 0x740) {
		Con_Printf("No CDR device detected at addr %u\n",phy_addr);
		return -1;
	}
	uVal = Cdr_ReadPart(phy_addr, CDR_VS_DEVICE_IDENTIFIER3, 3, 0);
	if(uVal == 0) {
		Con_Printf("Error: Unsupported CDR version A0\n");
		return 0;	
	}
#if 0
	Cdr_WriteReg(phy_addr, CDR_VS_DEVICE_CONTROL, 0x1020);	//Hard reset (bit 5) and MDIO init (bit 12)
#endif
	Cdr_WriteReg(phy_addr, CDR_VS_DEVICE_CONTROL, 0x200);	//Datapath soft reset (bit 9)
	for(lane = 0; lane < 4; lane++) {
		/* Set 128 steps for PI resolution inside reset */
		Cdr_WritePart(phy_addr, CDR_RX_1ST_ORDER_CONTROL(lane), 10, 8, 4);
	}
	/* Deassert datapath soft reset */
	Cdr_WriteReg(phy_addr, CDR_VS_DEVICE_CONTROL, 0);
	SleepMs(1);

	// CDR recal of TxPLL while PI3 is locked
	Cdr_WritePart(phy_addr, 1184, 1, 1, 1);	// Lockint Rx3 PI
	// Re-Calibrating Tx PLL
	Cdr_WritePart(phy_addr, CDR_TXPLL_CONFIG1, 15, 15, 0);
	Cdr_WritePart(phy_addr, CDR_TXPLL_CONFIG1, 15, 15, 1);
	Cdr_WritePart(phy_addr, CDR_TXPLL_CONFIG1, 15, 15, 0);
	SleepMs(100);		// wait for it to lock
	// regWrite(device + "30.1184.1",0)        # and unlock PI3
	//Cdr_WritePart(phy_addr, 1184, 1, 1, 0); // and unlock PI3
	//SleepMs(100);
	//for lane in range(4):                   # EQ offset override is set
	for (lane = 0; lane < 4; lane++) {
		Cdr_WritePart(phy_addr, 441 + 256 * lane, 0, 0, 1);	// Olga  Achtung!!!!!! Die Länge ist 0
	}

	//regWrite(device + "30.37.15",1) # Asserts FIFO reset
	Cdr_WritePart(phy_addr, CDR_FIFO_STATUS, 15, 15, 1);
	//regWrite(device + "30.37.14",1) # Enable auto-reset
	Cdr_WritePart(phy_addr, CDR_FIFO_STATUS, 14, 14, 1);
	//regWrite(device + "30.37.15",0) # De-Asserts FIFO reset
	Cdr_WritePart(phy_addr, CDR_FIFO_STATUS, 15, 15, 0);
	/* 
 	 ************************************************************************
	 * This section just checks things did complete are startup OK
 	 ************************************************************************
	 */
	uVal = Cdr_ReadPart(phy_addr, CDR_RXPLL_CONFIG2, 8, 8);
	if (uVal == 0) {
		status = 0;
		Con_Printf("*** Rx PLL of PHY %u not locked, is REFCLK present ?\n",phy_addr);
	}
	uVal = Cdr_ReadPart(phy_addr, CDR_TXPLL_CONFIG2, 8, 8);
	if (uVal == 0) {
		status = 0;
		Con_Printf("*** Tx PLL of PHY %u not locked, is REFCLK present and Rx3 PI locked ?\n",phy_addr);
	}
	uVal = Cdr_ReadPart(phy_addr, CDR_VS_DEVICE_CONTROL, 8, 8); /* Read reset seq complete bit */
	if (uVal == 0) {
		status = 0;
		Con_Printf("*** Part PHY %u did not complete GB Tx or CDR reset sequence\n",phy_addr);
	}
//print "Enabling calibrated EQ offsets"
//for lane in range(4):
//regWrite(device + "30." + str(441 + 256 * lane) + ".0",1)
	for (lane = 0; lane < 4; lane++) {
		Cdr_WritePart(phy_addr, 441 + 256 * lane, 0, 0, 1);
	}
	if(status != 1) {
		Con_Printf("Error: CDR is not ready\n");
	}
	return status;
}

/**
 ****************************************************************
 * Interface to the outside  for Recalibrating the CDR
 ****************************************************************
 */
uint8_t
CDR_Recalibrate(uint8_t cdrId)
{
	bool result;
	CDR *cdr = &gCDR[cdrId & 1];
	result = Cdr_Recalibrate(cdr->phyAddr);
	if(cdrId == CDR_ID_RX) {
		Cdr_WriteReg(cdr->phyAddr, 1184, 0X0000);
	}
	return result;
}

/**
 ****************************************************************
 * Use the Hardware reset line to reset all CDR's
 ****************************************************************
 */ 
static void
Cdr_HwReset(void) 
{
	CDR_RESET_DR = 0; 
	SleepMs(1);
	CDR_RESET_DR = 1; 
}

uint16_t
CDR_ReadEqObserve(uint8_t cdr,uint8_t lane)
{
	uint8_t phyAddr = gCDR[cdr & 1].phyAddr;
	return Cdr_ReadReg(phyAddr, CDR_RX_EQ_OBSERVE1(lane));
}
/**
 ************************************************************************
 * \fn static int8_t cmd_cdr(Interp * interp, uint8_t argc, char *argv[])
 * Allows to read or write registers of a CDR from the command shell
 **************************************************************************
 */
static int8_t
cmd_cdr(Interp * interp, uint8_t argc, char *argv[])
{
	uint8_t phyAddr;
	uint16_t val;
	uint16_t regAddr;
	if ((argc == 3) && (strcmp(argv[2], "recal") == 0)) {
		uint8_t cdr = astrtoi16(argv[1]);
		Con_Printf("Calling Olgas CDR_Recal for CDR %u\n", cdr);
		Cdr_Recalibrate(cdr);
		if(cdr == 1) {
			Cdr_WriteReg(cdr, 1184, 0X0000);
		}
		return 0;
	} else if ((argc == 3) && (strcmp(argv[2], "init") == 0)) {
		uint8_t cdr = astrtoi16(argv[1]);
		Con_Printf("Calling Olgas CDR_InitCdr for CDR %u\n", cdr);
		Cdr_InitCdr(cdr);
		return 0;
	} else if ((argc == 3) && (strcmp(argv[2],"reset") == 0)) {
		phyAddr = astrtoi16(argv[1]);
		Cdr_SoftReset(phyAddr);
	} else if ((argc == 2) && (strcmp(argv[1],"hardreset") == 0)) {
		Cdr_HwReset();
	} else if (argc == 3) {
		phyAddr = astrtoi16(argv[1]);
		regAddr = astrtoi16(argv[2]);
		val = Cdr_ReadReg(phyAddr, regAddr);
		Con_Printf("%u.%u: 0x%x\n", phyAddr, regAddr, val);
		return 0;
	} else if (argc == 4) {
		phyAddr = astrtoi16(argv[1]);
		regAddr = astrtoi16(argv[2]);
		val = astrtoi16(argv[3]);
		Cdr_WriteReg(phyAddr, regAddr, val);
	} else {
		return -EC_BADARG;
	}
	return 0;
}

INTERP_CMD(cdrCmd, "cdr", cmd_cdr, "cdr <cdrAddr> <regAddr> ?<value>?   # read write to/from cdr");

static bool
PVReg_Get (void *cbData, uint32_t adId, char *bufP,uint16_t maxlen)
{
	const CdrRegister *reg;
	CDR *cdr = cbData;
	uint16_t value;
	if(adId >= array_size(gCdrRegister)) {
		Con_Printf("CDR %s Unexpected ID %lu\n",__func__,adId);
		return false;
	}
	reg = &gCdrRegister[adId];
	value = Cdr_ReadPart(cdr->phyAddr,reg->regNo,reg->lastBit,reg->firstBit);
        bufP[uitoa16(value,bufP)] = 0;
	return true;
}

static bool
PVReg_Set(void *cbData, uint32_t adId, const char *strP)
{
	const CdrRegister *reg;
	CDR *cdr = cbData;
	uint16_t value;
	if(adId >= array_size(gCdrRegister)) {
		Con_Printf("CDR %s Unexpected ID %lu\n",__func__,adId);
		return false;
	}
	reg = &gCdrRegister[adId];
	value = astrtoi16(strP);
	Cdr_WritePart(cdr->phyAddr,reg->regNo,reg->lastBit,reg->firstBit,value);
	return true;
}

/**
 *******************************************************************************
 * Access to a register in the 256 Byte RX/TX lane private spaces.
 *******************************************************************************
 */
static bool
PVLaneReg_Get (void *cbData, uint32_t adId, char *bufP,uint16_t maxlen)
{
	const CdrRegister *reg;
	CDR *cdr = cbData;
	uint16_t value;
	uint16_t regNr = adId & 0xffff;
	uint16_t lane = adId >> 16;
	if((regNr >= array_size(gCdrLaneRegister)) || (lane > 3)) {
		Con_Printf("CDR %s Unexpected ID %lu\n",__func__,adId);
		return false;
	}
	reg = &gCdrLaneRegister[regNr];
	value = Cdr_ReadPart(cdr->phyAddr,reg->regNo + (lane << 8),reg->lastBit,reg->firstBit);
        bufP[uitoa16(value,bufP)] = 0;
	return true;
}

static bool
PVLaneReg_Set(void *cbData, uint32_t adId, const char *strP)
{
	const CdrRegister *reg;
	CDR *cdr = cbData;
	uint16_t value;
	uint16_t regNr = adId & 0xffff;
	uint16_t lane = adId >> 16;
	if((regNr >= array_size(gCdrLaneRegister)) || (lane > 3)) {
		Con_Printf("CDR %s Unexpected ID %lu\n",__func__,adId);
		return false;
	}
	reg = &gCdrLaneRegister[regNr];
	value = astrtoi16(strP);
	Cdr_WritePart(cdr->phyAddr,reg->regNo + (lane << 8),reg->lastBit,reg->firstBit,value);
	return true;
}

uint64_t 
CDR_GetErrCnt(uint8_t cdrID, uint8_t lane)
{
	CDR *cdr;
	if((cdrID != 0) || (lane >= 4)) {
		return 0;
	}	
	cdr = &gCDR[cdrID];
	return cdr->berCntr[lane];
}

static bool
PVBerCntr_Get (void *cbData, uint32_t adId, char *bufP,uint16_t maxlen)
{
	CDR *cdr = cbData;
	uint32_t lane = adId;
	uint64_t value = cdr->berCntr[lane];
        bufP[uitoa64(value,bufP)] = 0;
	return true;
}

static bool
PVBerCntr_Set(void *cbData, uint32_t adId, const char *strP)
{
	CDR *cdr = cbData;
	uint64_t value;
	uint32_t lane = adId;
	value  = astrtoi64(strP);
	cdr->berCntr[lane] = value;
	return true;
}

/**
 *****************************************************************
 * Timer Interrupt routine for reading the error rates
 *****************************************************************
 */

void
Excep_CMT1_CMI1(void)
{
	unsigned int i;
	CDR *cdr = &gCDR[0];
	ENABLE_IRQ();
	MDIO_Address(cdr->phyAddr, DEVTYPE, CDR_LANE0_ERROR_COUNTER);
	for(i = 0; i < 4; i++) {
		uint16_t errCnt;
		errCnt = MDIO_ReadInc(cdr->phyAddr, DEVTYPE);
		cdr->berCntr[i] += errCnt;
	}
}

/*
 ************************************************************************************
 * Zum Webinterface exportierte Variablen:
 * CDR status: PLL locks,
 * PRBS pattern, PRBS lock, LOL status , PRBS error counters.
 ************************************************************************************
 */
void
CDR_Init(const char *name)
{
	uint32_t i,lane;
	CDR *cdr;

	CDR_RESET_DIR = 1;
	CDR_RESET_DR = 1; 
	Cdr_HwReset();

	/* Initialize the Second CDR is the Transmitter */
	cdr = &gCDR[1];
	cdr->phyAddr = 0;

	Cdr_SoftReset(cdr->phyAddr);
	Cdr_Recalibrate(cdr->phyAddr);
	Cdr_InitCdr(cdr->phyAddr);

	/* Initialize the RX CDR */
	cdr = &gCDR[0];
	cdr->phyAddr = 1;

	Cdr_SoftReset(cdr->phyAddr);
	Cdr_Recalibrate(cdr->phyAddr);
	Cdr_InitCdr(cdr->phyAddr);
	Cdr_WriteReg(cdr->phyAddr, 1184, 0X0000);

	Interp_RegisterCmd(&cdrCmd);
	for(i = 0; i < array_size(gCdrRegister); i++) {
		const CdrRegister *reg = &gCdrRegister[i];
		PVar_New(PVReg_Get,PVReg_Set,cdr,i,"%s.%s",name,reg->name);
	}	
	for(i = 0; i < array_size(gCdrLaneRegister); i++) {
		const CdrRegister *reg = &gCdrLaneRegister[i];
		for(lane = 0; lane < 4; lane++) {
			PVar_New(PVLaneReg_Get,PVLaneReg_Set,cdr,i + (lane << 16) ,"%s.l%lu.%s",name,lane,reg->name);
		}
	}
	for(lane = 0; lane < 4; lane++) {
		PVar_New(PVBerCntr_Get,PVBerCntr_Set,cdr,lane ,"%s.l%lu.%s",name,lane,"err_cntr64");
		PVar_New(PVBerCntr_Get,PVBerCntr_Set,cdr,lane ,"bert0.L%lu.%s",lane,"errCntr");
	}

	MSTP(CMT1) = 0;
        CMT.CMSTR0.BIT.STR1 = 1;
        CMT1.CMCR.BIT.CKS = 0;
        CMT1.CMCOR = (F_PCLK / 500 / 8);
        CMT1.CMCR.BIT.CMIE = 1;
        IPR(CMT1, CMI1) = CDR_IPL;
        IEN(CMT1, CMI1) = 1;
}
