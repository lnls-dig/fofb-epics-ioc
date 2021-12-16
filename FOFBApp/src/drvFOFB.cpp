/*
 *  *  *  * drvFOFB.cpp
 *   *   *   *
 *    *    *    * Authors: Melissa Aguiar
 *     *     *     *
 *      *      *      * Created Dec. 03, 2021
 *       *       *       */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include <epicsTypes.h>
#include <epicsTime.h>
#include <epicsThread.h>
#include <epicsString.h>
#include <epicsTimer.h>
#include <epicsMutex.h>
#include <epicsEvent.h>
#include <iocsh.h>

#include "drvFOFB.h"
#include <epicsExport.h>

/** The polling interval when checking to see if acquisition is complete */
#define FOFB_POLL_TIME                   .1
#define FOFB_PM_POLL_TIME                1

#define PI                              3.14159265
#define FREQ_SAMPLE                     100.00              /* Hz */
#define FREQ                            1.00                /* Hz */
#define TEST_LENGTH                     4092

#define CH_DFLT_TRIGGER_CHAN            0
#define CH_DFLT_TRIGGER_SW_CHAN         18

#define CH_DFLT_TRIGGER_SEL_PM_CHAN     1
/* Number fo triggers that are relative to waveform acquisition */
#define MAX_WAVEFORM_TRIGGERS           17

#define CH_DEFAULT_PM                   CH_TBT
#define SAMPLES_PRE_DEFAULT_PM(maxPoints) \
                                        (maxPoints/2)
#define SAMPLES_POST_DEFAULT_PM(maxPoints) \
                                        (maxPoints/2)
#define NUM_SHOTS_DEFAULT_PM            1
#define TRIG_DEFAULT_PM                 ACQ_CLIENT_TRIG_EXTERNAL
#define DFLT_SAMPLE_SIZE                8 /* in bytes */
#define DFLT_NUM_ATOMS                  4
#define DFLT_ATOM_WIDTH                 2 /* in bytes */

#define SERVICE_NAME_SIZE               50

typedef struct {
    drvFOFB *drvFOFBp;
    fofb_coreID_types coreID;
    double pollTime;
    bool autoStart;
} taskParams_t;

static const boardMap_t boardMap[MAX_FOFBS+1] = {
         /* board, fofb, core_id*/
    /* 0 (INVALID)  */ {-1, -1,  -1},
    /* 1            */ {1,   0,   2},
    /* 2            */ {1,   1,   3},
    /* 3            */ {2,   0,   2},
    /* 4            */ {2,   1,   3},
    /* 5            */ {3,   0,   2},
    /* 6            */ {3,   1,   3},
    /* 7            */ {4,   0,   2},
    /* 8            */ {4,   1,   3},
    /* 9            */ {5,   0,   2},
    /* 10           */ {5,   1,   3},
    /* 11           */ {6,   0,   2},
    /* 12           */ {6,   1,   3},
    /* 13           */ {7,   0,   2},
    /* 14           */ {7,   1,   3},
    /* 15           */ {8,   0,   2},
    /* 16           */ {8,   1,   3},
    /* 17           */ {9,   0,   2},
    /* 18           */ {9,   1,   3},
    /* 19           */ {10,  0,   2},
    /* 20           */ {10,  1,   3},
    /* 21           */ {11,  0,   2},
    /* 22           */ {11,  1,   3},
    /* 23           */ {12,  0,   2},
    /* 24           */ {12,  1,   3}
};

static const channelMap_t channelMap[CH_END] = {
                        /* Amp, Phase, Pos, AmpA, AmpB, AmpC, AmpD, AmpALL */
    /* [CH_ADC] =     */ {CH_HW_ADC,                           // HwAmpChannel
                          -1,                                  // HwPhaseChannel
                          -1,                                  // HwPosChannel
                          0,                                   // CalcPos
                          {{WVF_GENAMP_A,                      // NDArrayAmp
                            WVF_GENAMP_B,
                            WVF_GENAMP_C,
                            WVF_GENAMP_D,
                            WVF_GENAMP_ALL},
                           {WVF_AMP_PM_A,
                            WVF_AMP_PM_B,
                            WVF_AMP_PM_C,
                            WVF_AMP_PM_D,
                            WVF_AMP_PM_ALL},
                          },
                          {{-1,                                 // NDArrayPhase
                            -1,
                            -1,
                            -1,
                            -1},
                            {-1,
                             -1,
                             -1,
                             -1,
                             -1},
                          },
                          {{-1,                                 // NDArrayPos
                            -1,
                            -1,
                            -1,
                            -1},
                           {-1,
                            -1,
                            -1,
                            -1,
                            -1},
                          },
                          },
    /* [CH_TBT] =     */ {CH_HW_TBT,                             // HwAmpChannel
                          -1,                                    // HwPhaseChannel
                          -1,                                    // HwPosChannel
                          1,                                     // CalcPos
                          {{WVF_GENAMP_A,                        // NDArrayAmp
                            WVF_GENAMP_B,
                            WVF_GENAMP_C,
                            WVF_GENAMP_D,
                            WVF_GENAMP_ALL},
                            {WVF_AMP_PM_A,
                             WVF_AMP_PM_B,
                             WVF_AMP_PM_C,
                             WVF_AMP_PM_D,
                             WVF_AMP_PM_ALL},
                          },
                          {{-1,                                  // NDArrayPhase
                            -1,
                            -1,
                            -1,
                            -1},
                            {-1,
                             -1,
                             -1,
                             -1,
                             -1},
                          },
                          {{WVF_GENPOS_A,                        // NDArrayPos
                            WVF_GENPOS_B,
                            WVF_GENPOS_C,
                            WVF_GENPOS_D,
                            WVF_GENPOS_ALL},
                            {WVF_POS_PM_A,
                             WVF_POS_PM_B,
                             WVF_POS_PM_C,
                             WVF_POS_PM_D,
                             WVF_POS_PM_ALL},
                          },
                          },
    /* [CH_FOFB] =    */ {CH_HW_FOFB,                            // HwAmpChannel
                          -1,                                    // HwPhaseChannel
                          -1,                                    // HwPosChannel
                          1,                                     // CalcPos
                          {{WVF_GENAMP_A,                        // NDArrayAmp
                            WVF_GENAMP_B,
                            WVF_GENAMP_C,
                            WVF_GENAMP_D,
                            WVF_GENAMP_ALL},
                           {WVF_AMP_PM_A,
                            WVF_AMP_PM_B,
                            WVF_AMP_PM_C,
                            WVF_AMP_PM_D,
                            WVF_AMP_PM_ALL},
                          },
                          {{-1,                                 // NDArrayPhase
                            -1,
                            -1,
                            -1,
                            -1},
                            {-1,
                             -1,
                             -1,
                             -1,
                             -1},
                          },
                          {{WVF_GENPOS_A,                        // NDArrayPos
                            WVF_GENPOS_B,
                            WVF_GENPOS_C,
                            WVF_GENPOS_D,
                            WVF_GENPOS_ALL},
                           {WVF_POS_PM_A,
                            WVF_POS_PM_B,
                            WVF_POS_PM_C,
                            WVF_POS_PM_D,
                            WVF_POS_PM_ALL},
                          },
                          },
    /* [CH_MONIT1] =  */ {CH_HW_MONIT1,                          // HwAmpChannel
                          -1,                                    // HwPhaseChannel
                          -1,                                    // HwPosChannel
                          1,                                     // CalcPos
                          {{WVF_GENAMP_A,                        // NDArrayAmp
                            WVF_GENAMP_B,
                            WVF_GENAMP_C,
                            WVF_GENAMP_D,
                            WVF_GENAMP_ALL},
                           {-1,
                            -1,
                            -1,
                            -1,
                            -1},
                          },
                          {{-1,                                 // NDArrayPhase
                            -1,
                            -1,
                            -1,
                            -1},
                            {-1,
                             -1,
                             -1,
                             -1,
                             -1},
                          },
                          {{WVF_GENPOS_A,                        // NDArrayPos
                            WVF_GENPOS_B,
                            WVF_GENPOS_C,
                            WVF_GENPOS_D,
                            WVF_GENPOS_ALL},
                           {-1,
                            -1,
                            -1,
                            -1,
                            -1},
                          },
                          },
    /* [CH_SP] =      */ {CH_HW_ADC,                           // HwAmpChannel
                          -1,                                  // HwPhaseChannel
                          -1,                                  // HwPosChannel
                          0,                                   // CalcPos
                          {{WVF_AMP_SP_A,                      // NDArrayAmp
                            WVF_AMP_SP_B,
                            WVF_AMP_SP_C,
                            WVF_AMP_SP_D,
                            WVF_AMP_SP_ALL},
                            {-1,
                             -1,
                             -1,
                             -1,
                             -1},
                          },
                          {{-1,                                 // NDArrayPhase
                            -1,
                            -1,
                            -1,
                            -1},
                            {-1,
                             -1,
                             -1,
                             -1,
                             -1},
                          },
                          {{-1,                                 // NDArrayPos
                            -1,
                            -1,
                            -1,
                            -1},
                           {-1,
                            -1,
                            -1,
                            -1,
                            -1},
                          },
                          },
};

/* FIXME: This reverse mapping must match the maximum HwAmpChannel for ChannelMap */
static const channelRevMap_t channelRevMap[CH_HW_END] = {
                        /* EPICS channel */
     /* [CH_HW_ADC] =       */  {CH_ADC},
     /* 1 = Unavailable     */  {-1},
     /* 2 = Unavailable     */  {-1},
     /* 3 = Unavailable     */  {-1},
     /* 4 = Unavailable     */  {-1},
     /* 5 = Unavailable     */  {-1},
     /* [CH_HW_TBT] =       */  {CH_TBT},
     /* 7 = Unavailable     */  {-1},
     /* 8 = Unavailable     */  {-1},
     /* 9 = Unavailable     */  {-1},
     /* 10 = Unavailable    */  {-1},
     /* [CH_HW_FOFB] =      */  {CH_FOFB},
     /* 12 = Unavailable    */  {-1},
     /* 13 = Unavailable    */  {-1},
     /* [CH_HW_MONIT1] =    */  {CH_MONIT1},
};
/* Int32 functions mapping */
static const functionsAny_t fofbProcessingSetGetRamWriteFunc          = {functionsUInt32_t{"FOFB_PROCESSING", halcs_set_fofb_processing_ram_write,
                                                                          halcs_get_fofb_processing_ram_write}};
static const functionsAny_t fofbProcessingSetGetRamAddrFunc           = {functionsUInt32_t{"FOFB_PROCESSING", halcs_set_fofb_processing_ram_addr,
                                                                          halcs_get_fofb_processing_ram_addr}};
static const functionsAny_t fofbProcessingSetGetRamDataInFunc         = {functionsUInt32_t{"FOFB_PROCESSING", halcs_set_fofb_processing_ram_data_in,
                                                                          halcs_get_fofb_processing_ram_data_in}};
static const functionsAny_t fofbProcessingSetGetRamDataOutFunc        = {functionsUInt32_t{"FOFB_PROCESSING", NULL,
                                                                          halcs_get_fofb_processing_ram_data_out}};
static const functionsAny_t rtmLampSetGetStatusFunc                   = {functionsUInt32_t{"RTMLAMP_OHWR", NULL ,
                                                                          halcs_get_rtmlamp_ohwr_sta}};
static const functionsAny_t rtmLampSetGetDacDataFromWbFunc            = {functionsUInt32_t{"RTMLAMP_OHWR", halcs_set_rtmlamp_ohwr_dac_data_from_wb,
                                                                          halcs_get_rtmlamp_ohwr_dac_data_from_wb}};
static const functionsAny_t rtmLampSetGetAmpIFlagLFunc                = {functionsUInt32Chan_t{"RTMLAMP_OHWR", halcs_set_rtmlamp_ohwr_amp_iflag_l,
                                                                          halcs_get_rtmlamp_ohwr_amp_iflag_l}};
static const functionsAny_t rtmLampSetGetAmpTFlagLFunc                = {functionsUInt32Chan_t{"RTMLAMP_OHWR", halcs_set_rtmlamp_ohwr_amp_tflag_l,
                                                                          halcs_get_rtmlamp_ohwr_amp_tflag_l}};
static const functionsAny_t rtmLampSetGetAmpIFlagRFunc                = {functionsUInt32Chan_t{"RTMLAMP_OHWR", halcs_set_rtmlamp_ohwr_amp_iflag_r,
                                                                          halcs_get_rtmlamp_ohwr_amp_iflag_r}};
static const functionsAny_t rtmLampSetGetAmpTFlagRFunc                = {functionsUInt32Chan_t{"RTMLAMP_OHWR", halcs_set_rtmlamp_ohwr_amp_tflag_r,
                                                                          halcs_get_rtmlamp_ohwr_amp_tflag_r}};
static const functionsAny_t rtmLampSetGetAmpEnFunc                    = {functionsUInt32Chan_t{"RTMLAMP_OHWR", halcs_set_rtmlamp_ohwr_amp_en,
                                                                          halcs_get_rtmlamp_ohwr_amp_en}};
static const functionsAny_t rtmLampSetGetPIOLTriangEnFunc             = {functionsUInt32Chan_t{"RTMLAMP_OHWR", halcs_set_rtmlamp_ohwr_pi_ol_triang_enable,
                                                                          halcs_get_rtmlamp_ohwr_pi_ol_triang_enable}};
static const functionsAny_t rtmLampSetGetPIOLSquareEnFunc             = {functionsUInt32Chan_t{"RTMLAMP_OHWR", halcs_set_rtmlamp_ohwr_pi_ol_square_enable,
                                                                          halcs_get_rtmlamp_ohwr_pi_ol_square_enable}};
static const functionsAny_t rtmLampSetGetPISPSquareEnFunc             = {functionsUInt32Chan_t{"RTMLAMP_OHWR", halcs_set_rtmlamp_ohwr_pi_sp_square_enable,
                                                                          halcs_get_rtmlamp_ohwr_pi_sp_square_enable}};
static const functionsAny_t rtmLampSetGetPIEnFunc                     = {functionsUInt32Chan_t{"RTMLAMP_OHWR", halcs_set_rtmlamp_ohwr_pi_enable,
                                                                          halcs_get_rtmlamp_ohwr_pi_enable}};
static const functionsAny_t rtmLampSetGetDacDataFunc                  = {functionsUInt32Chan_t{"RTMLAMP_OHWR", halcs_set_rtmlamp_ohwr_dac_data,
                                                                          halcs_get_rtmlamp_ohwr_dac_data}};
static const functionsAny_t rtmLampSetGetDacWrFunc                    = {functionsUInt32Chan_t{"RTMLAMP_OHWR", halcs_set_rtmlamp_ohwr_dac_wr,
                                                                          halcs_get_rtmlamp_ohwr_dac_wr}};
static const functionsAny_t rtmLampSetGetPIKPFunc                     = {functionsUInt32Chan_t{"RTMLAMP_OHWR", halcs_set_rtmlamp_ohwr_pi_kp,
                                                                          halcs_get_rtmlamp_ohwr_pi_kp}};
static const functionsAny_t rtmLampSetGetPITIFunc                     = {functionsUInt32Chan_t{"RTMLAMP_OHWR", halcs_set_rtmlamp_ohwr_pi_ti,
                                                                          halcs_get_rtmlamp_ohwr_pi_ti}};
static const functionsAny_t rtmLampSetGetPISPFunc                     = {functionsUInt32Chan_t{"RTMLAMP_OHWR", halcs_set_rtmlamp_ohwr_pi_sp,
                                                                          halcs_get_rtmlamp_ohwr_pi_sp}};
static const functionsAny_t rtmLampSetGetPIOLDacCntMaxFunc            = {functionsUInt32_t{"RTMLAMP_OHWR", halcs_set_rtmlamp_ohwr_pi_ol_dac_cnt_max,
                                                                          halcs_get_rtmlamp_ohwr_pi_ol_dac_cnt_max}};
static const functionsAny_t rtmLampSetGetPISPLimInfFunc               = {functionsUInt32_t{"RTMLAMP_OHWR", halcs_set_rtmlamp_ohwr_pi_sp_lim_inf,
                                                                          halcs_get_rtmlamp_ohwr_pi_sp_lim_inf}};
static const functionsAny_t fofbCtrlSetGetActPartFunc                 = {functionsUInt32_t{"FOFB_CTRL", halcs_set_fofb_ctrl_act_part,
                                                                          halcs_get_fofb_ctrl_act_part}};
static const functionsAny_t fofbCtrlSetGeErrClrFunc                   = {functionsUInt32_t{"FOFB_CTRL", halcs_set_fofb_ctrl_err_clr,
                                                                          halcs_get_fofb_ctrl_err_clr}};
static const functionsAny_t fofbCtrlSetGetCcEnableFunc                = {functionsUInt32_t{"FOFB_CTRL", halcs_set_fofb_ctrl_cc_enable,
                                                                          halcs_get_fofb_ctrl_cc_enable}};
static const functionsAny_t fofbCtrlSetGetTfsOverrideFunc             = {functionsUInt32_t{"FOFB_CTRL", halcs_set_fofb_ctrl_tfs_override,
                                                                          halcs_get_fofb_ctrl_tfs_override}};
static const functionsAny_t fofbCtrlSetGetBpmIdFunc                   = {functionsUInt32_t{"FOFB_CTRL", halcs_set_fofb_ctrl_bpm_id,
                                                                          halcs_get_fofb_ctrl_bpm_id}};
static const functionsAny_t fofbCtrlSetGetTimeFrameLenFunc            = {functionsUInt32_t{"FOFB_CTRL", halcs_set_fofb_ctrl_time_frame_len,
                                                                          halcs_get_fofb_ctrl_time_frame_len}};
static const functionsAny_t fofbCtrlSetGetMgtPowerdownFunc            = {functionsUInt32_t{"FOFB_CTRL", halcs_set_fofb_ctrl_mgt_powerdown,
                                                                          halcs_get_fofb_ctrl_mgt_powerdown}};
static const functionsAny_t fofbCtrlSetGetMgtLoopbackFunc             = {functionsUInt32_t{"FOFB_CTRL", halcs_set_fofb_ctrl_mgt_loopback,
                                                                          halcs_get_fofb_ctrl_mgt_loopback}};
static const functionsAny_t fofbCtrlSetGetTimeFrameDlyFunc            = {functionsUInt32_t{"FOFB_CTRL", halcs_set_fofb_ctrl_time_frame_dly,
                                                                          halcs_get_fofb_ctrl_time_frame_dly}};
static const functionsAny_t fofbCtrlSetGetGoldenOrbXFunc              = {functionsUInt32_t{"FOFB_CTRL", halcs_set_fofb_ctrl_golden_orb_x,
                                                                          halcs_get_fofb_ctrl_golden_orb_x}};
static const functionsAny_t fofbCtrlSetGetGoldenOrbYFunc              = {functionsUInt32_t{"FOFB_CTRL", halcs_set_fofb_ctrl_golden_orb_y,
                                                                          halcs_get_fofb_ctrl_golden_orb_y}};
static const functionsAny_t fofbCtrlSetGetCustFeatureFunc             = {functionsUInt32_t{"FOFB_CTRL", halcs_set_fofb_ctrl_cust_feature,
                                                                          halcs_get_fofb_ctrl_cust_feature}};
static const functionsAny_t fofbCtrlSetGetRxPolarityFunc              = {functionsUInt32_t{"FOFB_CTRL", halcs_set_fofb_ctrl_rx_polarity,
                                                                          halcs_get_fofb_ctrl_rx_polarity}};
static const functionsAny_t fofbCtrlSetGetPayloadselFunc              = {functionsUInt32_t{"FOFB_CTRL", halcs_set_fofb_ctrl_payloadsel,
                                                                          halcs_get_fofb_ctrl_payloadsel}};
static const functionsAny_t fofbCtrlSetGetFofbdataselFunc             = {functionsUInt32_t{"FOFB_CTRL", halcs_set_fofb_ctrl_fofbdatasel,
                                                                          halcs_get_fofb_ctrl_fofbdatasel}};
static const functionsAny_t fofbCtrlSetGetFirmwareVerFunc             = {functionsUInt32_t{"FOFB_CTRL", NULL,
                                                                          halcs_get_fofb_ctrl_firmware_ver}};
static const functionsAny_t fofbCtrlSetGetSysStatusFunc               = {functionsUInt32_t{"FOFB_CTRL", NULL,
                                                                          halcs_get_fofb_ctrl_sys_status}};
static const functionsAny_t fofbCtrlSetGetLinkPartnerFunc             = {functionsUInt32Chan_t{"FOFB_CTRL", NULL,
                                                                          halcs_get_fofb_ctrl_link_partner}};
static const functionsAny_t fofbCtrlSetGetLinkUpFunc                  = {functionsUInt32_t{"FOFB_CTRL", NULL,
                                                                          halcs_get_fofb_ctrl_link_up}};
static const functionsAny_t fofbCtrlSetGetTimeFrameCountFunc          = {functionsUInt32_t{"FOFB_CTRL", NULL,
                                                                          halcs_get_fofb_ctrl_time_frame_count}};
static const functionsAny_t fofbCtrlSetGetHardErrCntFunc              = {functionsUInt32Chan_t{"FOFB_CTRL", NULL,
                                                                          halcs_get_fofb_ctrl_hard_err_cnt}};
static const functionsAny_t fofbCtrlSetGetSoftErrCntFunc              = {functionsUInt32Chan_t{"FOFB_CTRL", NULL,
                                                                          halcs_get_fofb_ctrl_soft_err_cnt}};
static const functionsAny_t fofbCtrlSetGetFrameErrCntFunc             = {functionsUInt32Chan_t{"FOFB_CTRL", NULL,
                                                                          halcs_get_fofb_ctrl_frame_err_cnt}};
static const functionsAny_t fofbCtrlSetGetRxPckCntFunc                = {functionsUInt32Chan_t{"FOFB_CTRL", NULL,
                                                                          halcs_get_fofb_ctrl_rx_pck_cnt}};
static const functionsAny_t fofbCtrlSetGetTxPckCntFunc                = {functionsUInt32Chan_t{"FOFB_CTRL", NULL,
                                                                          halcs_get_fofb_ctrl_tx_pck_cnt}};
static const functionsAny_t fofbCtrlSetGetFodProcessTimeFunc          = {functionsUInt32_t{"FOFB_CTRL", NULL,
                                                                          halcs_get_fofb_ctrl_fod_process_time}};
static const functionsAny_t fofbCtrlSetGetBpmCntFunc                  = {functionsUInt32_t{"FOFB_CTRL", NULL,
                                                                          halcs_get_fofb_ctrl_bpm_count}};
static const functionsAny_t fofbCtrlSetGetBpmIdRdbackFunc             = {functionsUInt32_t{"FOFB_CTRL", NULL,
                                                                          halcs_get_fofb_ctrl_bpm_id_rdback}};
static const functionsAny_t fofbCtrlSetGetTfLengthRdbackFunc          = {functionsUInt32_t{"FOFB_CTRL", NULL,
                                                                          halcs_get_fofb_ctrl_tf_length_rdback}};
static const functionsAny_t fofbCtrlSetGetPowerdownRdbackFunc         = {functionsUInt32_t{"FOFB_CTRL", NULL,
                                                                          halcs_get_fofb_ctrl_powerdown_rdback}};
static const functionsAny_t fofbCtrlSetGetLoopbackRdbackFunc          = {functionsUInt32_t{"FOFB_CTRL", NULL,
                                                                          halcs_get_fofb_ctrl_loopback_rdback}};
static const functionsAny_t fofbCtrlSetGetFaivalRdbackFunc            = {functionsUInt32_t{"FOFB_CTRL", NULL,
                                                                          halcs_get_fofb_ctrl_faival_rdback}};
static const functionsAny_t fofbCtrlSetGetToaRdEnFunc                 = {functionsUInt32_t{"FOFB_CTRL", halcs_set_fofb_ctrl_toa_rd_en,
                                                                          halcs_get_fofb_ctrl_toa_rd_en}};
static const functionsAny_t fofbCtrlSetGetToaRdStrFunc                = {functionsUInt32_t{"FOFB_CTRL", halcs_set_fofb_ctrl_toa_rd_str,
                                                                          halcs_get_fofb_ctrl_toa_rd_str}};
static const functionsAny_t fofbCtrlSetGetToaDataFunc                 = {functionsUInt32_t{"FOFB_CTRL", NULL,
                                                                          halcs_get_fofb_ctrl_toa_data}};
static const functionsAny_t fofbCtrlSetGetRcbRdEnFunc                 = {functionsUInt32_t{"FOFB_CTRL", halcs_set_fofb_ctrl_rcb_rd_en,
                                                                          halcs_get_fofb_ctrl_rcb_rd_en}};
static const functionsAny_t fofbCtrlSetGetRcbRdStrFunc                = {functionsUInt32_t{"FOFB_CTRL", halcs_set_fofb_ctrl_rcb_rd_str,
                                                                          halcs_get_fofb_ctrl_rcb_rd_str}};
static const functionsAny_t fofbCtrlSetGetRcbDataFunc                 = {functionsUInt32_t{"FOFB_CTRL", NULL,
                                                                          halcs_get_fofb_ctrl_rcb_data}};
static const functionsAny_t fofbSetGetAcqDataTrigThresFunc            = {functionsUInt32Acq_t{"ACQ", acq_set_data_trig_thres,
                                                                          acq_get_data_trig_thres}};
static const functionsAny_t fofbSetGetAcqDataTrigPolFunc              = {functionsUInt32Acq_t{"ACQ", acq_set_data_trig_pol,
                                                                          acq_get_data_trig_pol}};
static const functionsAny_t fofbSetGetAcqDataTrigSelFunc              = {functionsUInt32Acq_t{"ACQ", acq_set_data_trig_sel,
                                                                          acq_get_data_trig_sel}};
static const functionsAny_t fofbSetGetAcqDataTrigFiltFunc             = {functionsUInt32Acq_t{"ACQ", acq_set_data_trig_filt,
                                                                          acq_get_data_trig_filt}};
static const functionsAny_t fofbSetGetAcqHwDlyFunc                    = {functionsUInt32Acq_t{"ACQ", acq_set_hw_trig_dly,
                                                                          acq_get_hw_trig_dly}};
static const functionsAny_t fofbSetGetAcqDataTrigChanFunc             = {functionsUInt32Acq_t{"ACQ", acq_set_data_trig_chan,
                                                                          acq_get_data_trig_chan}};
static const functionsAny_t fofbSetGetTrigDirFunc                     = {functionsUInt32Chan_t{"TRIGGER_IFACE", halcs_set_trigger_dir,
                                                                          halcs_get_trigger_dir}};
static const functionsAny_t fofbSetGetTrigDirPolFunc                  = {functionsUInt32Chan_t{"TRIGGER_IFACE", halcs_set_trigger_dir_pol,
                                                                          halcs_get_trigger_dir_pol}};
static const functionsAny_t fofbSetGetTrigRcvCntRstFunc               = {functionsUInt32Chan_t{"TRIGGER_IFACE", halcs_set_trigger_rcv_count_rst,
                                                                          halcs_get_trigger_rcv_count_rst}};
static const functionsAny_t fofbSetGetTrigTrnCntRstFunc               = {functionsUInt32Chan_t{"TRIGGER_IFACE", halcs_set_trigger_transm_count_rst,
                                                                          halcs_get_trigger_transm_count_rst}};
static const functionsAny_t fofbSetGetTrigRcvLenFunc                  = {functionsUInt32Chan_t{"TRIGGER_IFACE", halcs_set_trigger_rcv_len,
                                                                          halcs_get_trigger_rcv_len}};
static const functionsAny_t fofbSetGetTrigTrnLenFunc                  = {functionsUInt32Chan_t{"TRIGGER_IFACE", halcs_set_trigger_transm_len,
                                                                          halcs_get_trigger_transm_len}};
static const functionsAny_t fofbSetGetTrigCntRcvFunc                  = {functionsUInt32Chan_t{"TRIGGER_IFACE", halcs_set_trigger_count_rcv,
                                                                          halcs_get_trigger_count_rcv}};
static const functionsAny_t fofbSetGetTrigCntTrnFunc                  = {functionsUInt32Chan_t{"TRIGGER_IFACE", halcs_set_trigger_count_transm,
                                                                         halcs_get_trigger_count_transm}};
static const functionsAny_t fofbSetGetTrigRcvSrcFunc                  = {functionsUInt32Chan_t{"TRIGGER_MUX", halcs_set_trigger_rcv_src,
                                                                          halcs_get_trigger_rcv_src}};
static const functionsAny_t fofbSetGetTrigTrnSrcFunc                  = {functionsUInt32Chan_t{"TRIGGER_MUX", halcs_set_trigger_transm_src,
                                                                          halcs_get_trigger_transm_src}};
static const functionsAny_t fofbSetGetTrigRcvSelFunc                  = {functionsUInt32Chan_t{"TRIGGER_MUX", halcs_set_trigger_rcv_in_sel,
                                                                          halcs_get_trigger_rcv_in_sel}};
static const functionsAny_t fofbSetGetTrigTrnSelFunc                  = {functionsUInt32Chan_t{"TRIGGER_MUX", halcs_set_trigger_transm_out_sel,
                                                                          halcs_get_trigger_transm_out_sel}};

static const char *driverName="drvFOFB";

static taskParams_t taskParams[NUM_ACQ_CORES_PER_FOFB] = {
    /* Regular Core */
    {
        NULL,                           // drvFOFBp
        FOFBIDReg,                      // coreID
        FOFB_POLL_TIME,                 // pollTime
        false                           // autoStart
    },
    /* Post-Mortem Core */
    {
        NULL,                           // drvFOFBp
        FOFBIDPM,                       // coreID
        FOFB_PM_POLL_TIME,              // pollTime
        true                            // autoStart
    },
};

void acqTask(void *drvPvt);

static void exitHandlerC(void *pPvt)
{
    drvFOFB *pdrvFOFB = (drvFOFB *)pPvt;
    pdrvFOFB->~drvFOFB();
}

asynStatus drvFOFB::getServiceChan (int fofbNumber, int addr, const char *serviceName,
        epicsUInt32 *chanArg) const
{
    static const char *functionName = "getServiceChan";
    asynStatus status = asynSuccess;
    epicsUInt32 chan = 0;

    /* Static mapping. FIXME? */
    if (streq(serviceName, "TRIGGER_MUX") || streq(serviceName, "TRIGGER_IFACE")) {
        chan = addr % MAX_TRIGGERS;
    }
    else {
        chan = addr;
    }

    *chanArg = chan;
    return status;
}

asynStatus drvFOFB::getServiceID (int fofbNumber, int addr, const char *serviceName,
        int *serviceIDArg) const
{
    static const char *functionName = "getServiceID";
    asynStatus status = asynSuccess;
    int serviceID = 0;
    int addrMod = 0;

    /* Static mapping. FIXME? */
    /* For these services there's only a single core per FPGA, so serviceID is always 0.
 *      * INIT service is always 0 per HALCS instance */
    if (streq(serviceName, "TRIGGER_IFACE") || streq(serviceName, "INIT")) {
        *serviceIDArg = 0;
        return status;
    }
    if (streq(serviceName, "ACQ")) {
        addrMod = addr;
    }
    else if (streq(serviceName, "TRIGGER_MUX")) {
        addrMod = addr/MAX_TRIGGERS;
    }
    else {
        addrMod = 0;
    }

    switch (addrMod) {
        case FOFBIDReg:
            serviceID = boardMap[fofbNumber].fofb;
            break;

        case FOFBIDPM:
            serviceID = boardMap[fofbNumber].core_id;
            break;

        default:
            status = asynError;
            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s: error getting serviceID for addr = %d for service = %s\n",
                driverName, functionName, addr, serviceName);
            goto err_exit;
    }
    *serviceIDArg = serviceID;

err_exit:
    return status;
}

asynStatus drvFOFB::getFullServiceName (int fofbNumber, int addr, const char *serviceName,
        char *fullServiceName, int fullServiceNameSize) const
{
    static const char *functionName = "getFullServiceName";
    int coreID = 0;
    int errs = 0;
    asynStatus status = asynSuccess;

    status = getServiceID (fofbNumber, addr, serviceName, &coreID);
    if (status) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
            "%s:%s: error calling getServiceID, status=%d\n",
            driverName, functionName, status);
        goto get_service_id_err;
    }

    errs = snprintf(fullServiceName, fullServiceNameSize, "HALCS%d:DEVIO:%s%d",
            boardMap[fofbNumber].board, serviceName, coreID);
    if (errs < 0 || errs >= fullServiceNameSize){
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
            "%s:%s: error generating fullServiceName, errs=%d\n",
            driverName, functionName, errs);
        status = asynError;
        goto gen_full_service_name;
    }

gen_full_service_name:
get_service_id_err:
    return status;
}

/** Constructor for the drvFOFB class.
 *  *  *  * Calls constructor for the asynPortDriver base class.
 *   *   *   * \param[in] portName The name of the asyn port driver to be created.
 *    *    *    * \param[in] endpoint The device address string ]
 *     *     *     * */
drvFOFB::drvFOFB(const char *portName, const char *endpoint, int fofbNumber,
        const char *type, int verbose, int timeout, int maxPoints,
        int maxBuffers, size_t maxMemory)
   : asynNDArrayDriver(portName,
                    MAX_ADDR, /* maxAddr */
                    maxBuffers, maxMemory, /* maxBuffers, maxMemory */
                    asynUInt32DigitalMask | asynInt32Mask | asynInt16ArrayMask | asynFloat64Mask | asynGenericPointerMask | asynDrvUserMask,    /* Interface mask     */
                    asynUInt32DigitalMask | asynInt32Mask | asynInt16ArrayMask | asynFloat64Mask | asynGenericPointerMask,                      /* Interrupt mask     */
                    ASYN_CANBLOCK | ASYN_MULTIDEVICE, /* asynFlags.  This driver blocks it is multi-device */
                    1, /* Autoconnect */
                    0, /* Default priority */
                    0) /* Default stack size*/
{
    asynStatus status;
    const char *functionName = "drvFOFB";

    /* Create portName so we can create a new AsynUser later */
    fofbPortName = epicsStrDup(portName);
    fofbMaxPoints = maxPoints;

    this->endpoint = strdup(endpoint);
    if (this->endpoint == NULL) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s drvFOFB failure to copy endpoint\n",
                driverName, functionName);
        status = asynError;
        goto endpoint_dup_err;
    }

    if (fofbNumber < FOFB_NUMBER_MIN || fofbNumber > FOFB_NUMBER_MAX) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s drvFOFB invalid fofbNumber\n",
                driverName, functionName);
        status = asynError;
        goto invalid_fofb_number_err;
    }

    this->fofbNumber = fofbNumber;
    this->verbose = verbose;
    this->timeout = timeout;

    for (int i = 0; i < NUM_FOFB_MODES; ++i) {
        for (int j = 0; j < NUM_ACQ_CORES_PER_FOFB; ++j) {
            this->readingActive[i][j] = 0;
            this->repetitiveTrigger[i][j] = 0;
        }
    }

    for (int i = 0; i < NUM_FOFB_MODES; ++i) {

        for (int j = 0; j < NUM_ACQ_CORES_PER_FOFB; ++j) {
            /* Create events for signalling acquisition thread */
            this->startAcqEventId[i][j] = epicsEventCreate(epicsEventEmpty);
            if (!this->startAcqEventId[i][j]) {
                asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                        "%s:%s epicsEventCreate[%d] failure for start event\n",
                        driverName, functionName, i);
                return;
            }

            this->stopAcqEventId[i][j] = epicsEventCreate(epicsEventEmpty);
            if (!this->stopAcqEventId[i][j]) {
                printf("%s:%s: epicsEventCreate[%d] failure for stop event\n",
                        driverName, functionName, i);
                return;
            }

            this->abortAcqEventId[i][j] = epicsEventCreate(epicsEventEmpty);
            if (!this->abortAcqEventId[i][j]) {
                printf("%s:%s: epicsEventCreate[%d] failure for abort event\n",
                        driverName, functionName, i);
                return;
            }

            this->activeAcqEventId[i][j] = epicsEventCreate(epicsEventEmpty);
            if (!this->activeAcqEventId[i][j]) {
                printf("%s:%s: epicsEventCreate[%d] failure for active event\n",
                        driverName, functionName, i);
                return;
            }
        }
    }

    /* Create parameters for all addresses without specifying the ones that don't
 *  *  *      * make sense to be on a specified list. Without this we woudl have to create
 *   *   *           * different parameterIndex structures to store each index, as they could be
 *    *    *                * differently if created in just a few lists */

    /* CAUTION. The order of craetion must be the same as defined in .h file.
 *  *  *      * Otherwise, checking for FIRST_PARAM/LAST_PARAM won't work */

    /* Create general parameters */

    /* Create fofb_processing parameters */
    createParam(P_FofbProcessingRamWriteString,      asynParamUInt32Digital,        &P_FofbProcessingRamWrite);
    createParam(P_FofbProcessingRamAddrString,       asynParamUInt32Digital,        &P_FofbProcessingRamAddr);
    createParam(P_FofbProcessingRamDataInString,     asynParamUInt32Digital,        &P_FofbProcessingRamDataIn);
    createParam(P_FofbProcessingRamDataOutString,    asynParamUInt32Digital,        &P_FofbProcessingRamDataOut);
    /* Create rtm_lamp parameters */
    createParam(P_RtmLampStatusString,               asynParamUInt32Digital,        &P_RtmLampStatus);
    createParam(P_RtmLampDacDataFromWbString,        asynParamUInt32Digital,        &P_RtmLampDacDataFromWb);
    createParam(P_RtmLampAmpIFlagLString,            asynParamUInt32Digital,        &P_RtmLampAmpIFlagL);
    createParam(P_RtmLampAmpTFlagLString,            asynParamUInt32Digital,        &P_RtmLampAmpTFlagL);
    createParam(P_RtmLampAmpIFlagRString,            asynParamUInt32Digital,        &P_RtmLampAmpIFlagR);
    createParam(P_RtmLampAmpTFlagRString,            asynParamUInt32Digital,        &P_RtmLampAmpTFlagR);
    createParam(P_RtmLampAmpEnString,                asynParamUInt32Digital,        &P_RtmLampAmpEn);
    createParam(P_RtmLampPIOLTriangEnString,         asynParamUInt32Digital,        &P_RtmLampPIOLTriangEn);
    createParam(P_RtmLampPIOLSquareEnString,         asynParamUInt32Digital,        &P_RtmLampPIOLSquareEn);
    createParam(P_RtmLampPISPSquareEnString,         asynParamUInt32Digital,        &P_RtmLampPISPSquareEn);
    createParam(P_RtmLampPIEnString,                 asynParamUInt32Digital,        &P_RtmLampPIEn);
    createParam(P_RtmLampDacDataString,              asynParamUInt32Digital,        &P_RtmLampDacData);
    createParam(P_RtmLampDacWrString,                asynParamUInt32Digital,        &P_RtmLampDacWr);
    createParam(P_RtmLampPIKPString,                 asynParamUInt32Digital,        &P_RtmLampPIKP);
    createParam(P_RtmLampPITIString,                 asynParamUInt32Digital,        &P_RtmLampPITI);
    createParam(P_RtmLampPISPString,                 asynParamUInt32Digital,        &P_RtmLampPISP);
    createParam(P_RtmLampPIOLDacCntMaxString,        asynParamUInt32Digital,        &P_RtmLampPIOLDacCntMax);
    createParam(P_RtmLampPISPLimInfString,           asynParamUInt32Digital,        &P_RtmLampPISPLimInf);
    /* Create fofb_ctrl parameters */
    createParam(P_FofbCtrlActPartString,             asynParamUInt32Digital,        &P_FofbCtrlActPart);
    createParam(P_FofbCtrlErrClrString,              asynParamUInt32Digital,        &P_FofbCtrlErrClr);
    createParam(P_FofbCtrlCcEnableString,            asynParamUInt32Digital,        &P_FofbCtrlCcEnable);
    createParam(P_FofbCtrlTfsOverrideString,         asynParamUInt32Digital,        &P_FofbCtrlTfsOverride);
    createParam(P_FofbCtrlBpmIdString,               asynParamUInt32Digital,        &P_FofbCtrlBpmId);
    createParam(P_FofbCtrlTimeFrameLenString,        asynParamUInt32Digital,        &P_FofbCtrlTimeFrameLen);
    createParam(P_FofbCtrlMgtPowerdownString,        asynParamUInt32Digital,        &P_FofbCtrlMgtPowerdown);
    createParam(P_FofbCtrlMgtLoopbackString,         asynParamUInt32Digital,        &P_FofbCtrlMgtLoopback);
    createParam(P_FofbCtrlTimeFrameDlyString,        asynParamUInt32Digital,        &P_FofbCtrlTimeFrameDly);
    createParam(P_FofbCtrlGoldenOrbXString,          asynParamUInt32Digital,        &P_FofbCtrlGoldenOrbX);
    createParam(P_FofbCtrlGoldenOrbYString,          asynParamUInt32Digital,        &P_FofbCtrlGoldenOrbY);
    createParam(P_FofbCtrlCustFeatureString,         asynParamUInt32Digital,        &P_FofbCtrlCustFeature);
    createParam(P_FofbCtrlRxPolarityString,          asynParamUInt32Digital,        &P_FofbCtrlRxPolarity);
    createParam(P_FofbCtrlPayloadselString,          asynParamUInt32Digital,        &P_FofbCtrlPayloadsel);
    createParam(P_FofbCtrlFofbdataselString,         asynParamUInt32Digital,        &P_FofbCtrlFofbdatasel);
    createParam(P_FofbCtrlFirmwareVerString,         asynParamUInt32Digital,        &P_FofbCtrlFirmwareVer);
    createParam(P_FofbCtrlSysStatusString,           asynParamUInt32Digital,        &P_FofbCtrlSysStatus);
    createParam(P_FofbCtrlLinkPartnerString,         asynParamUInt32Digital,        &P_FofbCtrlLinkPartner);
    createParam(P_FofbCtrlLinkUpString,              asynParamUInt32Digital,        &P_FofbCtrlLinkUp);
    createParam(P_FofbCtrlTimeFrameCountString,      asynParamUInt32Digital,        &P_FofbCtrlTimeFrameCount);
    createParam(P_FofbCtrlHardErrCntString,          asynParamUInt32Digital,        &P_FofbCtrlHardErrCnt);
    createParam(P_FofbCtrlSoftErrCntString,          asynParamUInt32Digital,        &P_FofbCtrlSoftErrCnt);
    createParam(P_FofbCtrlFrameErrCntString,         asynParamUInt32Digital,        &P_FofbCtrlFrameErrCnt);
    createParam(P_FofbCtrlRxPckCntString,            asynParamUInt32Digital,        &P_FofbCtrlRxPckCnt);
    createParam(P_FofbCtrlTxPckCntString,            asynParamUInt32Digital,        &P_FofbCtrlTxPckCnt);
    createParam(P_FofbCtrlFodProcessTimeString,      asynParamUInt32Digital,        &P_FofbCtrlFodProcessTime);
    createParam(P_FofbCtrlBpmCntString,              asynParamUInt32Digital,        &P_FofbCtrlBpmCnt);
    createParam(P_FofbCtrlBpmIdRdbackString,         asynParamUInt32Digital,        &P_FofbCtrlBpmIdRdback);
    createParam(P_FofbCtrlTfLengthRdbackString,      asynParamUInt32Digital,        &P_FofbCtrlTfLengthRdback);
    createParam(P_FofbCtrlPowerdownRdbackString,     asynParamUInt32Digital,        &P_FofbCtrlPowerdownRdback);
    createParam(P_FofbCtrlLoopbackRdbackString,      asynParamUInt32Digital,        &P_FofbCtrlLoopbackRdback);
    createParam(P_FofbCtrlFaivalRdbackString,        asynParamUInt32Digital,        &P_FofbCtrlFaivalRdback);
    createParam(P_FofbCtrlToaRdEnString,             asynParamUInt32Digital,        &P_FofbCtrlToaRdEn);
    createParam(P_FofbCtrlToaRdStrString,            asynParamUInt32Digital,        &P_FofbCtrlToaRdStr);
    createParam(P_FofbCtrlToaDataString,             asynParamUInt32Digital,        &P_FofbCtrlToaData);
    createParam(P_FofbCtrlRcbRdEnString,             asynParamUInt32Digital,        &P_FofbCtrlRcbRdEn);
    createParam(P_FofbCtrlRcbRdStrString,            asynParamUInt32Digital,        &P_FofbCtrlRcbRdStr);
    createParam(P_FofbCtrlRcbDataString,             asynParamUInt32Digital,        &P_FofbCtrlRcbData);
    /* Create acquistion parameters */
    createParam(P_FOFBStatusString,                  asynParamInt32,                &P_FOFBStatus);
    createParam(P_SamplesPreString,                  asynParamUInt32Digital,        &P_SamplesPre);
    createParam(P_SamplesPostString,                 asynParamUInt32Digital,        &P_SamplesPost);
    createParam(P_NumShotsString,                    asynParamUInt32Digital,        &P_NumShots);
    createParam(P_ChannelString,                     asynParamInt32,                &P_Channel);
    createParam(P_UpdateTimeString,                  asynParamFloat64,              &P_UpdateTime);
    createParam(P_TriggerString,                     asynParamUInt32Digital,        &P_Trigger);
    createParam(P_TriggerEventString,                asynParamUInt32Digital,        &P_TriggerEvent);
    createParam(P_TriggerRepString,                  asynParamUInt32Digital,        &P_TriggerRep);
    createParam(P_TriggerDataThresString,            asynParamUInt32Digital,        &P_TriggerDataThres);
    createParam(P_TriggerDataPolString,              asynParamUInt32Digital,        &P_TriggerDataPol);
    createParam(P_TriggerDataSelString,              asynParamUInt32Digital,        &P_TriggerDataSel);
    createParam(P_TriggerDataFiltString,             asynParamUInt32Digital,        &P_TriggerDataFilt);
    createParam(P_TriggerHwDlyString,                asynParamUInt32Digital,        &P_TriggerHwDly);
    createParam(P_DataTrigChanString,                asynParamUInt32Digital,        &P_DataTrigChan);
    createParam(P_ChannelSampleSizeString,           asynParamUInt32Digital,        &P_ChannelSampleSize);
    createParam(P_ChannelNumAtomsString,             asynParamUInt32Digital,        &P_ChannelNumAtoms);
    createParam(P_ChannelAtomWidthString,            asynParamUInt32Digital,        &P_ChannelAtomWidth);
    /* Create Trigger parameters */
    createParam(P_TriggerChanString,                 asynParamInt32,                &P_TriggerChan);
    createParam(P_TriggerDirString,                  asynParamUInt32Digital,        &P_TriggerDir);
    createParam(P_TriggerDirPolString,               asynParamUInt32Digital,        &P_TriggerDirPol);
    createParam(P_TriggerRcvCntRstString,            asynParamUInt32Digital,        &P_TriggerRcvCntRst);
    createParam(P_TriggerTrnCntRstString,            asynParamUInt32Digital,        &P_TriggerTrnCntRst);
    createParam(P_TriggerRcvLenString,               asynParamUInt32Digital,        &P_TriggerRcvLen);
    createParam(P_TriggerTrnLenString,               asynParamUInt32Digital,        &P_TriggerTrnLen);
    createParam(P_TriggerCntRcvString,               asynParamUInt32Digital,        &P_TriggerCntRcv);
    createParam(P_TriggerCntTrnString,               asynParamUInt32Digital,        &P_TriggerCntTrn);
    createParam(P_TriggerRcvSrcString,               asynParamUInt32Digital,        &P_TriggerRcvSrc);
    createParam(P_TriggerTrnSrcString,               asynParamUInt32Digital,        &P_TriggerTrnSrc);
    createParam(P_TriggerRcvInSelString,             asynParamUInt32Digital,        &P_TriggerRcvInSel);
    createParam(P_TriggerTrnOutSelString,            asynParamUInt32Digital,        &P_TriggerTrnOutSel);

    /* FOFB HW Int32 Functions mapping. Functions not mapped here are just written
 *  *  *      * to the parameter library */
    fofbHwFunc.emplace(P_FofbProcessingRamWrite,      fofbProcessingSetGetRamWriteFunc);
    fofbHwFunc.emplace(P_FofbProcessingRamAddr,       fofbProcessingSetGetRamAddrFunc);
    fofbHwFunc.emplace(P_FofbProcessingRamDataIn,     fofbProcessingSetGetRamDataInFunc);
    fofbHwFunc.emplace(P_FofbProcessingRamDataOut,    fofbProcessingSetGetRamDataOutFunc);
    fofbHwFunc.emplace(P_RtmLampStatus,               rtmLampSetGetStatusFunc);
    fofbHwFunc.emplace(P_RtmLampDacDataFromWb,        rtmLampSetGetDacDataFromWbFunc);
    fofbHwFunc.emplace(P_RtmLampAmpIFlagL,            rtmLampSetGetAmpIFlagLFunc);
    fofbHwFunc.emplace(P_RtmLampAmpTFlagL,            rtmLampSetGetAmpTFlagLFunc);
    fofbHwFunc.emplace(P_RtmLampAmpIFlagR,            rtmLampSetGetAmpIFlagRFunc);
    fofbHwFunc.emplace(P_RtmLampAmpTFlagR,            rtmLampSetGetAmpTFlagRFunc);
    fofbHwFunc.emplace(P_RtmLampAmpEn,                rtmLampSetGetAmpEnFunc);
    fofbHwFunc.emplace(P_RtmLampPIOLTriangEn,         rtmLampSetGetPIOLTriangEnFunc);
    fofbHwFunc.emplace(P_RtmLampPIOLSquareEn,         rtmLampSetGetPIOLSquareEnFunc);
    fofbHwFunc.emplace(P_RtmLampPISPSquareEn,         rtmLampSetGetPISPSquareEnFunc);
    fofbHwFunc.emplace(P_RtmLampPIEn,                 rtmLampSetGetPIEnFunc);
    fofbHwFunc.emplace(P_RtmLampDacData,              rtmLampSetGetDacDataFunc);
    fofbHwFunc.emplace(P_RtmLampDacWr,                rtmLampSetGetDacWrFunc);
    fofbHwFunc.emplace(P_RtmLampPIKP,                 rtmLampSetGetPIKPFunc);
    fofbHwFunc.emplace(P_RtmLampPITI,                 rtmLampSetGetPITIFunc);
    fofbHwFunc.emplace(P_RtmLampPISP,                 rtmLampSetGetPISPFunc);
    fofbHwFunc.emplace(P_RtmLampPIOLDacCntMax,        rtmLampSetGetPIOLDacCntMaxFunc);
    fofbHwFunc.emplace(P_RtmLampPISPLimInf,           rtmLampSetGetPISPLimInfFunc);
    fofbHwFunc.emplace(P_FofbCtrlActPart,             fofbCtrlSetGetActPartFunc);
    fofbHwFunc.emplace(P_FofbCtrlErrClr,              fofbCtrlSetGeErrClrFunc);
    fofbHwFunc.emplace(P_FofbCtrlCcEnable,            fofbCtrlSetGetCcEnableFunc);
    fofbHwFunc.emplace(P_FofbCtrlTfsOverride,         fofbCtrlSetGetTfsOverrideFunc);
    fofbHwFunc.emplace(P_FofbCtrlBpmId,               fofbCtrlSetGetBpmIdFunc);
    fofbHwFunc.emplace(P_FofbCtrlTimeFrameLen,        fofbCtrlSetGetTimeFrameLenFunc);
    fofbHwFunc.emplace(P_FofbCtrlMgtPowerdown,        fofbCtrlSetGetMgtPowerdownFunc);
    fofbHwFunc.emplace(P_FofbCtrlMgtLoopback,         fofbCtrlSetGetMgtLoopbackFunc);
    fofbHwFunc.emplace(P_FofbCtrlTimeFrameDly,        fofbCtrlSetGetTimeFrameDlyFunc);
    fofbHwFunc.emplace(P_FofbCtrlGoldenOrbX,          fofbCtrlSetGetGoldenOrbXFunc);
    fofbHwFunc.emplace(P_FofbCtrlGoldenOrbY,          fofbCtrlSetGetGoldenOrbYFunc);
    fofbHwFunc.emplace(P_FofbCtrlCustFeature,         fofbCtrlSetGetCustFeatureFunc);
    fofbHwFunc.emplace(P_FofbCtrlRxPolarity,          fofbCtrlSetGetRxPolarityFunc);
    fofbHwFunc.emplace(P_FofbCtrlPayloadsel,          fofbCtrlSetGetPayloadselFunc);
    fofbHwFunc.emplace(P_FofbCtrlFofbdatasel,         fofbCtrlSetGetFofbdataselFunc);
    fofbHwFunc.emplace(P_FofbCtrlFirmwareVer,         fofbCtrlSetGetFirmwareVerFunc);
    fofbHwFunc.emplace(P_FofbCtrlSysStatus,           fofbCtrlSetGetSysStatusFunc);
    fofbHwFunc.emplace(P_FofbCtrlLinkPartner,         fofbCtrlSetGetLinkPartnerFunc);
    fofbHwFunc.emplace(P_FofbCtrlLinkUp,              fofbCtrlSetGetLinkUpFunc);
    fofbHwFunc.emplace(P_FofbCtrlTimeFrameCount,      fofbCtrlSetGetTimeFrameCountFunc);
    fofbHwFunc.emplace(P_FofbCtrlHardErrCnt,          fofbCtrlSetGetHardErrCntFunc);
    fofbHwFunc.emplace(P_FofbCtrlSoftErrCnt,          fofbCtrlSetGetSoftErrCntFunc);
    fofbHwFunc.emplace(P_FofbCtrlFrameErrCnt,         fofbCtrlSetGetFrameErrCntFunc);
    fofbHwFunc.emplace(P_FofbCtrlRxPckCnt,            fofbCtrlSetGetRxPckCntFunc);
    fofbHwFunc.emplace(P_FofbCtrlTxPckCnt,            fofbCtrlSetGetTxPckCntFunc);
    fofbHwFunc.emplace(P_FofbCtrlFodProcessTime,      fofbCtrlSetGetFodProcessTimeFunc);
    fofbHwFunc.emplace(P_FofbCtrlBpmCnt,              fofbCtrlSetGetBpmCntFunc);
    fofbHwFunc.emplace(P_FofbCtrlBpmIdRdback,         fofbCtrlSetGetBpmIdRdbackFunc);
    fofbHwFunc.emplace(P_FofbCtrlTfLengthRdback,      fofbCtrlSetGetTfLengthRdbackFunc);
    fofbHwFunc.emplace(P_FofbCtrlPowerdownRdback,     fofbCtrlSetGetPowerdownRdbackFunc);
    fofbHwFunc.emplace(P_FofbCtrlLoopbackRdback,      fofbCtrlSetGetLoopbackRdbackFunc);
    fofbHwFunc.emplace(P_FofbCtrlFaivalRdback,        fofbCtrlSetGetFaivalRdbackFunc);
    fofbHwFunc.emplace(P_FofbCtrlToaRdEn,             fofbCtrlSetGetToaRdEnFunc);
    fofbHwFunc.emplace(P_FofbCtrlToaRdStr,            fofbCtrlSetGetToaRdStrFunc);
    fofbHwFunc.emplace(P_FofbCtrlToaData,             fofbCtrlSetGetToaDataFunc);
    fofbHwFunc.emplace(P_FofbCtrlRcbRdEn,             fofbCtrlSetGetRcbRdEnFunc);
    fofbHwFunc.emplace(P_FofbCtrlRcbRdStr,            fofbCtrlSetGetRcbRdStrFunc);
    fofbHwFunc.emplace(P_FofbCtrlRcbData,             fofbCtrlSetGetRcbDataFunc);
    fofbHwFunc.emplace(P_DataTrigChan,                fofbSetGetAcqDataTrigChanFunc);
    fofbHwFunc.emplace(P_TriggerDataThres,            fofbSetGetAcqDataTrigThresFunc);
    fofbHwFunc.emplace(P_TriggerDataPol,              fofbSetGetAcqDataTrigPolFunc);
    fofbHwFunc.emplace(P_TriggerDataSel,              fofbSetGetAcqDataTrigSelFunc);
    fofbHwFunc.emplace(P_TriggerDataFilt,             fofbSetGetAcqDataTrigFiltFunc);
    fofbHwFunc.emplace(P_TriggerHwDly,                fofbSetGetAcqHwDlyFunc);
    fofbHwFunc.emplace(P_TriggerDir,                  fofbSetGetTrigDirFunc);
    fofbHwFunc.emplace(P_TriggerDirPol,               fofbSetGetTrigDirPolFunc);
    fofbHwFunc.emplace(P_TriggerRcvCntRst,            fofbSetGetTrigRcvCntRstFunc);
    fofbHwFunc.emplace(P_TriggerTrnCntRst,            fofbSetGetTrigTrnCntRstFunc);
    fofbHwFunc.emplace(P_TriggerCntRcv,               fofbSetGetTrigCntRcvFunc);
    fofbHwFunc.emplace(P_TriggerCntTrn,               fofbSetGetTrigCntTrnFunc);
    fofbHwFunc.emplace(P_TriggerRcvLen,               fofbSetGetTrigRcvLenFunc);
    fofbHwFunc.emplace(P_TriggerTrnLen,               fofbSetGetTrigTrnLenFunc);
    fofbHwFunc.emplace(P_TriggerRcvSrc,               fofbSetGetTrigRcvSrcFunc);
    fofbHwFunc.emplace(P_TriggerTrnSrc,               fofbSetGetTrigTrnSrcFunc);
    fofbHwFunc.emplace(P_TriggerRcvInSel,             fofbSetGetTrigRcvSelFunc);
    fofbHwFunc.emplace(P_TriggerTrnOutSel,            fofbSetGetTrigTrnSelFunc);

    lock();
    status = fofbClientConnect(this->pasynUserSelf);
    unlock();

    /* If we correct connect for this first time, liclient
 *  *  *      * will ensure the reconnection to server if necessary, but we
 *   *   *           * must succeed here or we must abort completely */
    if (status != asynSuccess) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
            "%s:%s: error calling fofbClientConnect, status=%d\n",
            driverName, functionName, status);
        exit(1);
    }

    /* Set the initial values of some parameters */

    setUIntDigitalParam(P_FofbProcessingRamWrite,             0,              0xFFFFFFFF);
    setUIntDigitalParam(P_FofbProcessingRamAddr,              0,              0xFFFFFFFF);
    setUIntDigitalParam(P_FofbProcessingRamDataIn,            0,              0xFFFFFFFF);
    setUIntDigitalParam(P_FofbProcessingRamDataOut,           0,              0xFFFFFFFF);
    setUIntDigitalParam(P_RtmLampStatus,                      0,              0xFFFFFFFF);
    setUIntDigitalParam(P_RtmLampDacDataFromWb,               0,              0xFFFFFFFF);
    setUIntDigitalParam(P_RtmLampAmpIFlagL,                   0,              0xFFFFFFFF);
    setUIntDigitalParam(P_RtmLampAmpTFlagL,                   0,              0xFFFFFFFF);
    setUIntDigitalParam(P_RtmLampAmpIFlagR,                   0,              0xFFFFFFFF);
    setUIntDigitalParam(P_RtmLampAmpTFlagR,                   0,              0xFFFFFFFF);
    setUIntDigitalParam(P_RtmLampAmpEn,                       0,              0xFFFFFFFF);
    setUIntDigitalParam(P_RtmLampPIOLTriangEn,                0,              0xFFFFFFFF);
    setUIntDigitalParam(P_RtmLampPIOLSquareEn,                0,              0xFFFFFFFF);
    setUIntDigitalParam(P_RtmLampPISPSquareEn,                0,              0xFFFFFFFF);
    setUIntDigitalParam(P_RtmLampPIEn,                        0,              0xFFFFFFFF);
    setUIntDigitalParam(P_RtmLampDacData,                     0,              0xFFFFFFFF);
    setUIntDigitalParam(P_RtmLampDacWr,                       0,              0xFFFFFFFF);
    setUIntDigitalParam(P_RtmLampPIKP,                        0,              0xFFFFFFFF);
    setUIntDigitalParam(P_RtmLampPITI,                        0,              0xFFFFFFFF);
    setUIntDigitalParam(P_RtmLampPISP,                        0,              0xFFFFFFFF);
    setUIntDigitalParam(P_RtmLampPIOLDacCntMax,               0,              0xFFFFFFFF);
    setUIntDigitalParam(P_RtmLampPISPLimInf,                  0,              0xFFFFFFFF);
    setUIntDigitalParam(P_FofbCtrlActPart,                    0,              0xFFFFFFFF);
    setUIntDigitalParam(P_FofbCtrlErrClr,                     0,              0xFFFFFFFF);
    setUIntDigitalParam(P_FofbCtrlCcEnable,                   0,              0xFFFFFFFF);
    setUIntDigitalParam(P_FofbCtrlTfsOverride,                0,              0xFFFFFFFF);
    setUIntDigitalParam(P_FofbCtrlBpmId,                      0,              0xFFFFFFFF);
    setUIntDigitalParam(P_FofbCtrlTimeFrameLen,               0,              0xFFFFFFFF);
    setUIntDigitalParam(P_FofbCtrlMgtPowerdown,               0,              0xFFFFFFFF);
    setUIntDigitalParam(P_FofbCtrlMgtLoopback,                0,              0xFFFFFFFF);
    setUIntDigitalParam(P_FofbCtrlTimeFrameDly,               0,              0xFFFFFFFF);
    setUIntDigitalParam(P_FofbCtrlGoldenOrbX,                 0,              0xFFFFFFFF);
    setUIntDigitalParam(P_FofbCtrlGoldenOrbY,                 0,              0xFFFFFFFF);
    setUIntDigitalParam(P_FofbCtrlCustFeature,                0,              0xFFFFFFFF);
    setUIntDigitalParam(P_FofbCtrlRxPolarity,                 0,              0xFFFFFFFF);
    setUIntDigitalParam(P_FofbCtrlPayloadsel,                 0,              0xFFFFFFFF);
    setUIntDigitalParam(P_FofbCtrlFofbdatasel,                0,              0xFFFFFFFF);
    setUIntDigitalParam(P_FofbCtrlFirmwareVer,                0,              0xFFFFFFFF);
    setUIntDigitalParam(P_FofbCtrlSysStatus,                  0,              0xFFFFFFFF);
    setUIntDigitalParam(P_FofbCtrlLinkPartner,                0,              0xFFFFFFFF);
    setUIntDigitalParam(P_FofbCtrlLinkUp,                     0,              0xFFFFFFFF);
    setUIntDigitalParam(P_FofbCtrlTimeFrameCount,             0,              0xFFFFFFFF);
    setUIntDigitalParam(P_FofbCtrlHardErrCnt,                 0,              0xFFFFFFFF);
    setUIntDigitalParam(P_FofbCtrlSoftErrCnt,                 0,              0xFFFFFFFF);
    setUIntDigitalParam(P_FofbCtrlFrameErrCnt,                0,              0xFFFFFFFF);
    setUIntDigitalParam(P_FofbCtrlRxPckCnt,                   0,              0xFFFFFFFF);
    setUIntDigitalParam(P_FofbCtrlTxPckCnt,                   0,              0xFFFFFFFF);
    setUIntDigitalParam(P_FofbCtrlFodProcessTime,             0,              0xFFFFFFFF);
    setUIntDigitalParam(P_FofbCtrlBpmCnt,                     0,              0xFFFFFFFF);
    setUIntDigitalParam(P_FofbCtrlBpmIdRdback,                0,              0xFFFFFFFF);
    setUIntDigitalParam(P_FofbCtrlTfLengthRdback,             0,              0xFFFFFFFF);
    setUIntDigitalParam(P_FofbCtrlPowerdownRdback,            0,              0xFFFFFFFF);
    setUIntDigitalParam(P_FofbCtrlLoopbackRdback,             0,              0xFFFFFFFF);
    setUIntDigitalParam(P_FofbCtrlFaivalRdback,               0,              0xFFFFFFFF);
    setUIntDigitalParam(P_FofbCtrlToaRdEn,                    0,              0xFFFFFFFF);
    setUIntDigitalParam(P_FofbCtrlToaRdStr,                   0,              0xFFFFFFFF);
    setUIntDigitalParam(P_FofbCtrlToaData,                    0,              0xFFFFFFFF);
    setUIntDigitalParam(P_FofbCtrlRcbRdEn,                    0,              0xFFFFFFFF);
    setUIntDigitalParam(P_FofbCtrlRcbRdStr,                   0,              0xFFFFFFFF);
    setUIntDigitalParam(P_FofbCtrlRcbData,                    0,              0xFFFFFFFF);

    /* Set acquisition parameters */

    for (int addr = 0; addr < NUM_ACQ_CORES_PER_FOFB; ++addr) {
        setIntegerParam(addr, P_FOFBStatus,                                FOFBStatusIdle);
    }

    for (int addr = 0; addr < NUM_ACQ_CORES_PER_FOFB; ++addr) {
        setUIntDigitalParam(addr, P_SamplesPre,           1000,               0xFFFFFFFF);
        setUIntDigitalParam(addr, P_SamplesPost,             0,               0xFFFFFFFF);
        setUIntDigitalParam(addr, P_NumShots,                1,               0xFFFFFFFF);
        setIntegerParam(    addr, P_Channel,                                      CH_ADC);
        setDoubleParam(     addr, P_UpdateTime,                                      1.0);
        setUIntDigitalParam(addr, P_Trigger,       ACQ_CLIENT_TRIG_SKIP,      0xFFFFFFFF);
        setUIntDigitalParam(addr, P_TriggerEvent,  TRIG_ACQ_STOP,             0xFFFFFFFF);
        setUIntDigitalParam(addr, P_TriggerRep,              0,               0xFFFFFFFF);
        setUIntDigitalParam(addr, P_TriggerDataThres,      100,               0xFFFFFFFF);
        setUIntDigitalParam(addr, P_TriggerDataPol,          0,               0xFFFFFFFF);
        setUIntDigitalParam(addr, P_TriggerDataSel,          0,               0xFFFFFFFF);
        setUIntDigitalParam(addr, P_TriggerDataFilt,         1,               0xFFFFFFFF);
        setUIntDigitalParam(addr, P_TriggerHwDly,            0,               0xFFFFFFFF);
        setUIntDigitalParam(addr, P_DataTrigChan,            0,               0xFFFFFFFF);
        setUIntDigitalParam(addr, P_ChannelSampleSize, DFLT_SAMPLE_SIZE,      0xFFFFFFFF);
        setUIntDigitalParam(addr, P_ChannelNumAtoms,   DFLT_NUM_ATOMS,        0xFFFFFFFF);
        setUIntDigitalParam(addr, P_ChannelAtomWidth,  DFLT_ATOM_WIDTH,       0xFFFFFFFF);
    }

    /* Acquisition PM parameters */
    setUIntDigitalParam(FOFBIDPM, P_SamplesPre, SAMPLES_PRE_DEFAULT_PM(fofbMaxPoints), 0xFFFFFFFF);
    setUIntDigitalParam(FOFBIDPM, P_SamplesPost,SAMPLES_POST_DEFAULT_PM(fofbMaxPoints),0xFFFFFFFF);
    setUIntDigitalParam(FOFBIDPM, P_NumShots,   NUM_SHOTS_DEFAULT_PM,                  0xFFFFFFFF);
    setIntegerParam(    FOFBIDPM, P_Channel,                                        CH_DEFAULT_PM);
    setDoubleParam(     FOFBIDPM, P_UpdateTime,                                               1.0);
    setUIntDigitalParam(FOFBIDPM, P_Trigger,      ACQ_CLIENT_TRIG_EXTERNAL,            0xFFFFFFFF);
    setUIntDigitalParam(FOFBIDPM, P_TriggerEvent, TRIG_ACQ_START,                      0xFFFFFFFF);
    setUIntDigitalParam(FOFBIDPM, P_TriggerRep,               1,                       0xFFFFFFFF);
    setUIntDigitalParam(FOFBIDPM, P_TriggerDataThres,       100,                       0xFFFFFFFF);
    setUIntDigitalParam(FOFBIDPM, P_TriggerDataPol,           0,                       0xFFFFFFFF);
    setUIntDigitalParam(FOFBIDPM, P_TriggerDataSel,           0,                       0xFFFFFFFF);
    setUIntDigitalParam(FOFBIDPM, P_TriggerDataFilt,          1,                       0xFFFFFFFF);
    setUIntDigitalParam(FOFBIDPM, P_TriggerHwDly,             0,                       0xFFFFFFFF);
    setUIntDigitalParam(FOFBIDPM, P_DataTrigChan,             0,                       0xFFFFFFFF);

    /* Write to HW */
    for (int i = P_SamplesPre; i < P_DataTrigChan+1; ++i) {
        setParamGeneric(i, FOFBIDPM);
    }

    /* Set parameters for all triggers, all Acquisition cores. FIXME (Caution):
 *      * We must deal with it at the Database level and be sure we are
 *           * accessing the right parameter! */
    for (int i = 0; i < NUM_TRIG_CORES_PER_FOFB; ++i) {
        for (int addr = 0; addr < MAX_TRIGGERS; ++addr) {
            setIntegerParam(    i*MAX_TRIGGERS + addr, P_TriggerChan,                           CH_DFLT_TRIGGER_CHAN);
            setUIntDigitalParam(i*MAX_TRIGGERS + addr, P_TriggerDir,                      1,              0xFFFFFFFF); /* FPGA Input */
            setUIntDigitalParam(i*MAX_TRIGGERS + addr, P_TriggerDirPol,                   1,              0xFFFFFFFF); /* Reverse Direction Polarity */
            setUIntDigitalParam(i*MAX_TRIGGERS + addr, P_TriggerRcvCntRst,                0,              0xFFFFFFFF);
            setUIntDigitalParam(i*MAX_TRIGGERS + addr, P_TriggerTrnCntRst,                0,              0xFFFFFFFF);
            setUIntDigitalParam(i*MAX_TRIGGERS + addr, P_TriggerCntRcv,                   0,              0xFFFFFFFF);
            setUIntDigitalParam(i*MAX_TRIGGERS + addr, P_TriggerCntTrn,                   0,              0xFFFFFFFF);
            setUIntDigitalParam(i*MAX_TRIGGERS + addr, P_TriggerRcvLen,                   1,              0xFFFFFFFF);
            setUIntDigitalParam(i*MAX_TRIGGERS + addr, P_TriggerTrnLen,                   1,              0xFFFFFFFF);
            setUIntDigitalParam(i*MAX_TRIGGERS + addr, P_TriggerRcvSrc,                   0,              0xFFFFFFFF);
            setUIntDigitalParam(i*MAX_TRIGGERS + addr, P_TriggerTrnSrc,                   0,              0xFFFFFFFF);
            setUIntDigitalParam(i*MAX_TRIGGERS + addr, P_TriggerRcvInSel,                 0,              0xFFFFFFFF);
            setUIntDigitalParam(i*MAX_TRIGGERS + addr, P_TriggerTrnOutSel,                0,              0xFFFFFFFF);
        }
    }

    /* Set trigger parameters for PM */
    for (int addr = 0; addr < MAX_WAVEFORM_TRIGGERS; ++addr) {
        setIntegerParam(    FOFBIDPM*MAX_TRIGGERS + addr, P_TriggerChan,                         CH_DFLT_TRIGGER_CHAN);
        setUIntDigitalParam(FOFBIDPM*MAX_TRIGGERS + addr, P_TriggerDir,                    1,              0xFFFFFFFF); /* FPGA Input */
        setUIntDigitalParam(FOFBIDPM*MAX_TRIGGERS + addr, P_TriggerDirPol,                 1,              0xFFFFFFFF); /* Reverse Direction Polarity */
        setUIntDigitalParam(FOFBIDPM*MAX_TRIGGERS + addr, P_TriggerRcvCntRst,              0,              0xFFFFFFFF);
        setUIntDigitalParam(FOFBIDPM*MAX_TRIGGERS + addr, P_TriggerTrnCntRst,              0,              0xFFFFFFFF);
        setUIntDigitalParam(FOFBIDPM*MAX_TRIGGERS + addr, P_TriggerCntRcv,                 0,              0xFFFFFFFF);
        setUIntDigitalParam(FOFBIDPM*MAX_TRIGGERS + addr, P_TriggerCntTrn,                 0,              0xFFFFFFFF);
        setUIntDigitalParam(FOFBIDPM*MAX_TRIGGERS + addr, P_TriggerRcvLen,                 1,              0xFFFFFFFF);
        setUIntDigitalParam(FOFBIDPM*MAX_TRIGGERS + addr, P_TriggerTrnLen,                 1,              0xFFFFFFFF);
        setUIntDigitalParam(FOFBIDPM*MAX_TRIGGERS + addr, P_TriggerRcvSrc,                 0,              0xFFFFFFFF);
        setUIntDigitalParam(FOFBIDPM*MAX_TRIGGERS + addr, P_TriggerTrnSrc,                 0,              0xFFFFFFFF);
        setUIntDigitalParam(FOFBIDPM*MAX_TRIGGERS + addr, P_TriggerRcvInSel,  CH_DFLT_TRIGGER_SEL_PM_CHAN, 0xFFFFFFFF);
        setUIntDigitalParam(FOFBIDPM*MAX_TRIGGERS + addr, P_TriggerTrnOutSel,              0,              0xFFFFFFFF);
    }

    /* Set Switching Trigger values */
    for (int i = 0; i < NUM_TRIG_CORES_PER_FOFB; ++i) {
        setIntegerParam(    i*MAX_TRIGGERS + CH_DFLT_TRIGGER_SW_CHAN, P_TriggerChan,            CH_DFLT_TRIGGER_CHAN);
        setUIntDigitalParam(i*MAX_TRIGGERS + CH_DFLT_TRIGGER_SW_CHAN, P_TriggerDir,       1,              0xFFFFFFFF); /* FPGA Input */
        setUIntDigitalParam(i*MAX_TRIGGERS + CH_DFLT_TRIGGER_SW_CHAN, P_TriggerDirPol,    1,              0xFFFFFFFF); /* Reverse Direction Polarity */
        setUIntDigitalParam(i*MAX_TRIGGERS + CH_DFLT_TRIGGER_SW_CHAN, P_TriggerRcvCntRst, 0,              0xFFFFFFFF);
        setUIntDigitalParam(i*MAX_TRIGGERS + CH_DFLT_TRIGGER_SW_CHAN, P_TriggerTrnCntRst, 0,              0xFFFFFFFF);
        setUIntDigitalParam(i*MAX_TRIGGERS + CH_DFLT_TRIGGER_SW_CHAN, P_TriggerCntRcv,    0,              0xFFFFFFFF);
        setUIntDigitalParam(i*MAX_TRIGGERS + CH_DFLT_TRIGGER_SW_CHAN, P_TriggerCntTrn,    0,              0xFFFFFFFF);
        setUIntDigitalParam(i*MAX_TRIGGERS + CH_DFLT_TRIGGER_SW_CHAN, P_TriggerRcvLen,    1,              0xFFFFFFFF);
        setUIntDigitalParam(i*MAX_TRIGGERS + CH_DFLT_TRIGGER_SW_CHAN, P_TriggerTrnLen,    1,              0xFFFFFFFF);
        setUIntDigitalParam(i*MAX_TRIGGERS + CH_DFLT_TRIGGER_SW_CHAN, P_TriggerRcvSrc,    1,              0xFFFFFFFF);
        setUIntDigitalParam(i*MAX_TRIGGERS + CH_DFLT_TRIGGER_SW_CHAN, P_TriggerTrnSrc,    0,              0xFFFFFFFF);
        setUIntDigitalParam(i*MAX_TRIGGERS + CH_DFLT_TRIGGER_SW_CHAN, P_TriggerRcvInSel,  1,              0xFFFFFFFF);
        setUIntDigitalParam(i*MAX_TRIGGERS + CH_DFLT_TRIGGER_SW_CHAN, P_TriggerTrnOutSel, 0,              0xFFFFFFFF);
    }

    /* Write to HW */
    for (int trig_core = 0; trig_core < NUM_TRIG_CORES_PER_FOFB; ++trig_core) {
        for (int addr = 0; addr < MAX_TRIGGERS; ++addr) {
            for (int i = P_TriggerChan; i < P_TriggerTrnOutSel+1; ++i) {
                setParamGeneric(i, trig_core*MAX_TRIGGERS + addr);
            }
        }
    }

#if 0
     for (int i = 0; i < NUM_TRIG_CORES_PER_FOFB; ++i) {
        for (int addr = 0; addr < MAX_TRIGGERS; ++addr) {
            readTriggerParams(0xFFFFFFFF, i*MAX_TRIGGERS + addr);
        }
    }
#endif

    /* Do callbacks so higher layers see any changes. Call callbacks for every addr */
    for (int i = 0; i < MAX_ADDR; ++i) {
        callParamCallbacks(i);
    }

    /* Create the thread that computes the waveforms in the background */
    for (int i = 0; i < NUM_ACQ_CORES_PER_FOFB; ++i) {
        /* Assign task parameters passing the ACQ/Trigger instance ID as parameter.
 *          * The other parameters are already set-up*/
        taskParams[i].drvFOFBp = this;
        status = (asynStatus)(epicsThreadCreate("drvFOFBTask",
                    epicsThreadPriorityMedium,
                    epicsThreadGetStackSize(epicsThreadStackMedium),
                    (EPICSTHREADFUNC)::acqTask,
                    &taskParams[i]) == NULL);
        if (status) {
            printf("%s:%s: epicsThreadCreate failure\n", driverName, functionName);
            return;
        }
    }

#if 0
    /* This driver supports MAX_ADDR with autoConnect=1.  But there are only records
 *     * connected to addresses 0-3, so addresses 4-11 never show as "connected"
 *         * since nothing ever calls pasynManager->queueRequest.  So we do an
 *             * exceptionConnect to each address so asynManager will show them as connected.
 *                 * Note that this is NOT necessary for the driver to function correctly, the
 *                     * NDPlugins will still get called even for addresses that are not "connected".
 *                         * It is just to avoid confusion.
 *                             * */
    for (i=0; i<MAX_ADDR; ++i) {
        pasynUser = pasynManager->createAsynUser(0,0);
        pasynManager->connectDevice(pasynUser, portName, i);
        pasynManager->exceptionConnect(pasynUser);
    }
#endif

    epicsAtExit(exitHandlerC, this);
    return;

invalid_fofb_number_err:
    free (this->endpoint);
endpoint_dup_err:
    return;
}

/** Destructor for the drvFOFB class.
 *  *  *  */
drvFOFB::~drvFOFB()
{
    asynStatus status = asynSuccess;
    const char *functionName = "~drvFOFB";

    lock();
    status = fofbClientDisconnect(this->pasynUserSelf);
    unlock();

    if (status != asynSuccess) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
            "%s:%s: error calling fofbClientDisconnect, status=%d\n",
            driverName, functionName, status);
    }

    free (this->endpoint);
    this->endpoint = NULL;
    free (this->fofbPortName);
    this->fofbPortName = NULL;
}

asynStatus drvFOFB::connect(asynUser* pasynUser)
{
    return fofbClientConnect(pasynUser);
}

asynStatus drvFOFB::fofbClientConnect(asynUser* pasynUser)
{
    asynStatus status = asynSuccess;
    const char *fofbLogFile = "stdout";
    const char *functionName = "fofbClientConnect";

    /* Connect FOFB */
    if (fofbClient == NULL) {
        fofbClient = halcs_client_new_time (endpoint, verbose, fofbLogFile, timeout);
        if (fofbClient == NULL) {
            asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                    "%s:%s fofbClientConnect failure to create fofbClient instance\n",
                    driverName, functionName);
            status = asynError;
            goto create_halcs_client_err;
        }
    }

    /* Connect ACQ FOFB parameter clients*/
    for (int i = 0; i < NUM_TRIG_CORES_PER_FOFB; ++i) {
        if (fofbClientAcqParam[i] == NULL) {
            fofbClientAcqParam[i] = acq_client_new_time (endpoint, verbose, fofbLogFile, timeout);
            if (fofbClientAcqParam[i] == NULL) {
                asynPrint(pasynUser, ASYN_TRACE_ERROR,
                        "%s:%s fofbClientConnect failure to create fofbClientAcqParam[%d] instance\n",
                        driverName, functionName, i);
                status = asynError;
                goto create_halcs_client_acq_param_err;
            }
        }
    }

    /* Connect ACQ FOFB clients */
    for (int i = 0; i < NUM_TRIG_CORES_PER_FOFB; ++i) {
        if (fofbClientAcq[i] == NULL) {
            fofbClientAcq[i] = acq_client_new_time (endpoint, verbose, fofbLogFile, timeout);
            if (fofbClientAcq[i] == NULL) {
                asynPrint(pasynUser, ASYN_TRACE_ERROR,
                        "%s:%s fofbClientConnect failure to create fofbClientAcq[%d] instance\n",
                        driverName, functionName, i);
                status = asynError;
                goto create_halcs_client_acq_err;
            }
        }
    }

    asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
        "%s:%s: FOFB client connected\n",
        driverName, functionName);

    pasynManager->exceptionConnect(pasynUser);

    return status;

create_halcs_client_acq_err:
    /* Destroy possible uninitialized fofbClientAcq instances */
    for (int i = 0; i < NUM_TRIG_CORES_PER_FOFB; ++i) {
        if (fofbClientAcq[i] != NULL) {
            acq_client_destroy (&fofbClientAcq[i]);
        }
    }
create_halcs_client_acq_param_err:
    /* Destroy possible uninitialized fofbClientAcqParam instances */
    for (int i = 0; i < NUM_TRIG_CORES_PER_FOFB; ++i) {
        if (fofbClientAcqParam[i] != NULL) {
            acq_client_destroy (&fofbClientAcqParam[i]);
        }
    }

create_halcs_client_err:
    return status;
}

asynStatus drvFOFB::disconnect(asynUser* pasynUser)
{
    return fofbClientDisconnect(pasynUser);
}

asynStatus drvFOFB::fofbClientDisconnect(asynUser* pasynUser)
{
    asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
            "%s: calling fofbClientDisconnect\n",
            driverName);
    asynStatus status = asynSuccess;

    if (fofbClient != NULL) {
        halcs_client_destroy (&fofbClient);
    }

    for (int i = 0; i < NUM_TRIG_CORES_PER_FOFB; ++i) {
        if (fofbClientAcqParam[i] != NULL) {
            acq_client_destroy (&fofbClientAcqParam[i]);
        }
    }

    for (int i = 0; i < NUM_TRIG_CORES_PER_FOFB; ++i) {
        if (fofbClientAcq[i] != NULL) {
            acq_client_destroy (&fofbClientAcq[i]);
        }
    }

    pasynManager->exceptionDisconnect(pasynUser);
    return status;
}

void acqTask(void *drvPvt)
{
   taskParams_t *pPvt = (taskParams_t *)drvPvt;
   pPvt->drvFOFBp->acqTask(pPvt->coreID, pPvt->pollTime, pPvt->autoStart);
}

/********************************************************************/
/******************* FOFB Acquisition functions *********************/
/********************************************************************/

asynStatus drvFOFB::initAcqPM(int coreID)
{
    static const char *functionName = "initAcqPM";
    asynStatus status = asynSuccess;

    asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
        "%s:%s: setting Post-Mortem parameters for coreID = %d\n",
        driverName, functionName, coreID);

    /* Set paramters for Post-Mortem */
    setUIntDigitalParam(coreID, P_SamplesPre,  SAMPLES_PRE_DEFAULT_PM(fofbMaxPoints),
                                                                  0xFFFFFFFF);
    setUIntDigitalParam(coreID, P_SamplesPost, SAMPLES_POST_DEFAULT_PM(fofbMaxPoints),
                                                                  0xFFFFFFFF);
    setUIntDigitalParam(coreID, P_NumShots,    NUM_SHOTS_DEFAULT_PM,
                                                                  0xFFFFFFFF);
    setIntegerParam(    coreID, P_Channel,                         CH_DEFAULT_PM);
    setDoubleParam(     coreID, P_UpdateTime,                            1.0);
    setUIntDigitalParam(coreID, P_Trigger,     ACQ_CLIENT_TRIG_EXTERNAL,
                                                                   0xFFFFFFFF);
    setUIntDigitalParam(coreID, P_TriggerEvent,
                                               TRIG_ACQ_START,     0xFFFFFFFF);
    setUIntDigitalParam(coreID, P_TriggerRep,  1,                  0xFFFFFFFF);
    setUIntDigitalParam(coreID, P_TriggerDataThres,
                                               100,                0xFFFFFFFF);
    setUIntDigitalParam(coreID, P_TriggerDataPol,
                                               0,                  0xFFFFFFFF);
    setUIntDigitalParam(coreID, P_TriggerDataSel,
                                               0,                  0xFFFFFFFF);
    setUIntDigitalParam(coreID, P_TriggerDataFilt,
                                               1,                  0xFFFFFFFF);
    setUIntDigitalParam(coreID, P_TriggerHwDly,
                                               0,                  0xFFFFFFFF);
    setUIntDigitalParam(coreID, P_DataTrigChan,
                                               0,                  0xFFFFFFFF);

    /* Do callbacks so higher layers see any changes */
    callParamCallbacks(coreID);

    /* Start triggered acquisition */
    status = setAcqEvent(0xFFFFFFFF, coreID);
    if (status) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
            "%s:%s: error calling setAcqEvent, status=%d\n",
            driverName, functionName, status);
        goto set_acq_trig;
    }
    return status;

set_acq_trig:
    return status;
}

/* This should only be called by asyn thread, not Acquisition ones */
asynStatus drvFOFB::setAcqTrig(int coreID, acq_client_trig_e trig)
{
    static const char *functionName = "setAcqTrig";
    halcs_client_err_e err = HALCS_CLIENT_SUCCESS;
    asynStatus status = asynSuccess;
    char service[SERVICE_NAME_SIZE];

    /* Get correct service name*/
    status = getFullServiceName (this->fofbNumber, coreID, "ACQ", service, sizeof(service));
    if (status) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
            "%s:%s: error calling getFullServiceName, status=%d\n",
            driverName, functionName, status);
        goto get_service_err;
    }

    err = acq_set_trig (fofbClientAcqParam[coreID], service, trig);
    if (err != HALCS_CLIENT_SUCCESS) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s: error calling halcs_set_acq_trig for service = %s, trigger = %d\n",
                driverName, functionName, service, trig);
        status = asynError;
        goto halcs_acq_trig_err;
    }

halcs_acq_trig_err:
get_service_err:
    return status;
}

/* This can only return if the ACQ engine is IDLE or waiting
 *  * for some trigger (External, Data or Software) */
/* This should only be called by asyn thread, not Acquisition ones */
fofb_status_types drvFOFB::getFOFBInitAcqStatus(int coreID)
{
    fofb_status_types fofbStatus = FOFBStatusErrAcq;
    asynStatus status = asynSuccess;
    halcs_client_err_e herr = HALCS_CLIENT_SUCCESS;
    uint32_t trig = 0;
    const char* functionName = "getFOFBAcqStatus";
    char service[SERVICE_NAME_SIZE];

    /* Get correct service name*/
    status = getFullServiceName (this->fofbNumber, coreID, "ACQ", service, sizeof(service));
    if (status) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
            "%s:%s: error calling getFullServiceName, status=%d\n",
            driverName, functionName, status);
        goto get_service_err;
    }

    /* Have ACQ engine completed some work or is it still busy? */
    herr = acq_check (fofbClientAcqParam[coreID], service);
    if (herr == HALCS_CLIENT_SUCCESS) {
        return FOFBStatusIdle;
    }

    /* If the ACQ is doing something we need to figure it out what is it */
    herr = acq_get_trig (fofbClientAcqParam[coreID], service, &trig);
    if (herr != HALCS_CLIENT_SUCCESS) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
            "%s:%s: error calling halcs_get_acq_trig, status=%d\n",
            driverName, functionName, herr);
        goto get_service_err;
    }

    switch (trig) {
        case ACQ_CLIENT_TRIG_SKIP:
            /* If we are doing something and the trigger is set to SKIP,
 *              * then we are acquiring */
            fofbStatus = FOFBStatusAcquire;
            break;

        case ACQ_CLIENT_TRIG_EXTERNAL:
            fofbStatus = FOFBStatusTriggerHwExtWaiting;
            break;

        case ACQ_CLIENT_TRIG_DATA_DRIVEN:
            fofbStatus = FOFBStatusTriggerHwDataWaiting;
            break;

        case ACQ_CLIENT_TRIG_SOFTWARE:
            fofbStatus = FOFBStatusTriggerSwWaiting;
            break;

        default:
            fofbStatus = FOFBStatusErrAcq;
    }

get_service_err:
    return fofbStatus;
}

static bool acqIsFOFBStatusWaitSomeTrigger(int fofbStatus)
{
    if (fofbStatus == FOFBStatusTriggerHwExtWaiting ||
        fofbStatus == FOFBStatusTriggerHwDataWaiting ||
        fofbStatus == FOFBStatusTriggerSwWaiting) {
        return true;
    }

    return false;
}

static bool acqIsFOFBStatusErr(int fofbStatus)
{
    if (fofbStatus == FOFBStatusErrAcq ||
        fofbStatus == FOFBStatusAborted ||
        fofbStatus == FOFBStatusErrTooManyPoints ||
        fofbStatus == FOFBStatusErrTooFewPoints ||
        fofbStatus == FOFBStatusErrNoMem ||
        fofbStatus == FOFBStatusErrAcqOFlow) {
        return true;
    }

    return false;
}

/*
 *  * FOFB acquisition functions
 *   */

/** Acquisition task that runs as a separate thread.
 *  *  CAUTION. FIXME? Only one acquisition task is working at any given time: MultiMode or SinglePass
 *  */
void drvFOFB::acqTask(int coreID, double pollTime, bool autoStart)
{
    int status = asynSuccess;
    asynUser *pasynUser = NULL;
    epicsUInt32 num_samples_pre;
    epicsUInt32 num_samples_post;
    epicsUInt32 num_shots;
    epicsUInt32 sampleSize = 16; /* bytes */
    epicsUInt32 numAtoms = 4;
    epicsUInt32 atomWidth = 32; /* bits */
    int channel;
    epicsUInt32 trigger;
    double updateTime;
    double delay;
    int hwAmpChannel = 0;
    int acqCompleted = 0;
    int FOFBMode = 0;
    int fofbStatus = 0;
    int newAcq = 1;
    bool autoStartFirst = autoStart;
    epicsTimeStamp now;
    epicsFloat64 timeStamp;
    NDArray *pArrayAllChannels = NULL;
    NDDataType_t NDType = NDInt32;
    epicsTimeStamp startTime;
    epicsTimeStamp endTime;
    double elapsedTime;
    int arrayCounter;
    size_t dims[MAX_WVF_DIMS];
    static const char *functionName = "acqTask";

    /* Create an asynUser. FIXME: we should probably create a callback
 *      * for the processCallback, which would be called on a queuePortLock ()
 *           * so as to not block all addresses, just the ones related to that
 *                * specific BOARD */
    pasynUser = pasynManager->createAsynUser(0, 0);
    pasynUser->timeout = FOFB_TIMEOUT;
    status = pasynManager->connectDevice(pasynUser, fofbPortName, 0);
    if(status != asynSuccess) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
            "%s:%s: connectDevice failed, status=%d\n",
            driverName, functionName, status);
        return;
    }

    /* Loop forever */
    lock ();
    while (1) {
        /* Free buffers if needed*/
        if (pArrayAllChannels) {
            pArrayAllChannels->release ();
            pArrayAllChannels = NULL;
        }

        getIntegerParam(coreID, P_FOFBStatus, &fofbStatus);

        /* Check if we received a stop event */
        status = epicsEventWaitWithTimeout(this->stopAcqEventId[FOFBMode][coreID], pollTime);
        if (status == epicsEventWaitOK || !repetitiveTrigger[FOFBMode][coreID] || acqIsFOFBStatusErr(fofbStatus)) {
            /* We got a stop event, stop repetitive acquisition */
            readingActive[FOFBMode][coreID] = 0;

            /* Default to new acquisition. If we are waiting for a trigger
 *              * we will change this */
            newAcq = 1;

            /* Now, we can either be finished with the previous acquisition
 *              * (repetitive or not) or we could be waiting for a trigger armed
 *                           * outside this thread (for now, the only option is the case when
 *                                        * you set a trigger and then exit the IOC for some reason) */
            if (!acqCompleted && acqIsFOFBStatusWaitSomeTrigger(fofbStatus)) {
                asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
                        "%s:%s: waiting for trigger\n", driverName, functionName);
                newAcq = 0;
            }
            /* Only change state to IDLE if we are not in a error state and we have just acquired some data */
            else if (!acqIsFOFBStatusErr(fofbStatus)) {
                setIntegerParam(coreID, P_FOFBStatus, FOFBStatusIdle);
                callParamCallbacks(coreID);
            }

            /* We have consumed our data. This is important if we abort the next
 *              * acquisition, as we can detect that the current acquisition is completed,
 *                           * which would be wrong */
            acqCompleted = 0;

            /* Only wait for the startEvent if we are waiting for a
 *              * new acquisition */
            if (newAcq && !autoStartFirst) {
                unlock();
                /* Release the lock while we wait for an event that says acquire has started, then lock again */
                asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
                        "%s:%s: waiting for acquire to start\n", driverName, functionName);
                epicsEventWait(startAcqEventId[FOFBMode][coreID]);
                lock();
            }
            readingActive[FOFBMode][coreID] = 1;
            autoStartFirst = 0;
        }

        /* We are acquiring. Get the current time */
        epicsTimeGetCurrent(&startTime);

        /* Set the parameter in the parameter library. */
        getUIntDigitalParam(coreID , P_Trigger      , &trigger          , 0xFFFFFFFF);
        getUIntDigitalParam(coreID , P_SamplesPre   , &num_samples_pre  , 0xFFFFFFFF);
        getUIntDigitalParam(coreID , P_SamplesPost  , &num_samples_post , 0xFFFFFFFF);
        getUIntDigitalParam(coreID , P_NumShots     , &num_shots        , 0xFFFFFFFF);
        getIntegerParam(    coreID , P_Channel      , &channel);
        getDoubleParam(     coreID , P_UpdateTime   , &updateTime);
        getUIntDigitalParam(coreID,  P_ChannelSampleSize,
                                                      &sampleSize,        0xFFFFFFFF);
        getUIntDigitalParam(coreID,  P_ChannelNumAtoms,
                                                      &numAtoms,          0xFFFFFFFF);
        getUIntDigitalParam(coreID,  P_ChannelAtomWidth,
                                                      &atomWidth,         0xFFFFFFFF);

        /* Convert bit to byte */
        atomWidth = atomWidth/8;

        if(numAtoms > MAX_WVF_AMP_TYPES) {
            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                    "%s:%s: unsupported numAtoms > %d. Reduce this value in the gateware\n",
                    driverName, functionName, MAX_WVF_AMP_TYPES);
            continue;
        }

        /* Convert user channel into hw channel */
        hwAmpChannel = channelMap[channel].HwAmpChannel;
        if(hwAmpChannel < 0) {
            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                    "%s:%s: invalid HwAmpChannel channelMap for channel %d\n",
                    driverName, functionName, hwAmpChannel);
            continue;
        }

        /* Our waveform will have "num_samples_pres + num_samples_post"
 *          * samples in each dimension */
        dims[0] = numAtoms;
        dims[1] = (num_samples_pre + num_samples_post)*num_shots;

        /* We can't acquire something with 0 points */
        if (dims[1] == 0) {
            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                    "%s:%s: invalid number of points for acquisition (= 0)\n",
                    driverName, functionName);
            setIntegerParam(coreID, P_FOFBStatus, FOFBStatusErrTooFewPoints);
            callParamCallbacks(coreID);
            continue;
        }

        /* dims[1] must not exceed fofbMaxPoints, as we use this to alloc
 *          * points for the Waveform Plugins */
        if (dims[1] > fofbMaxPoints) {
            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                    "%s:%s: invalid number of points for acquisition (> %d)\n",
                    driverName, functionName, fofbMaxPoints);
            setIntegerParam(coreID, P_FOFBStatus, FOFBStatusErrTooManyPoints);
            callParamCallbacks(coreID);
            continue;
        }

        /* Waveform statistics */
        epicsTimeGetCurrent(&now);
        getIntegerParam(NDArrayCounter, &arrayCounter);
        arrayCounter++;
        setIntegerParam(NDArrayCounter, arrayCounter);

        status = getAcqNDArrayType(coreID, hwAmpChannel, atomWidth, &NDType);
        if (status != asynSuccess) {
            asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                    "%s:%s: unable to determine NDArray type for acquisition, coreID = %d\n",
                    driverName, functionName, coreID);
            continue;
        }

        pArrayAllChannels = pNDArrayPool->alloc(MAX_WVF_DIMS, dims, NDType, 0, NULL);
        if (pArrayAllChannels == NULL) {
            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s: unable to alloc pArrayAllChannels\n",
                driverName, functionName);
            setIntegerParam(coreID, P_FOFBStatus, FOFBStatusErrNoMem);
            callParamCallbacks(coreID);
            continue;
        }
        pArrayAllChannels->uniqueId = arrayCounter;
        timeStamp = now.secPastEpoch + now.nsec / 1.e9;
        pArrayAllChannels->timeStamp = timeStamp;
        pArrayAllChannels->epicsTS.secPastEpoch = now.secPastEpoch;
        pArrayAllChannels->epicsTS.nsec = now.nsec;
        getAttributes(pArrayAllChannels->pAttributeList);

        /* Just start the acquisition if we are not already acquiring */
        if (newAcq) {
            /* Tell we are acquiring just before we actually start it */
            setIntegerParam(coreID, P_FOFBStatus, FOFBStatusAcquire);
            callParamCallbacks(coreID);

            /* Do acquisition */
            unlock();
            pasynManager->lockPort(pasynUser);
            status = startAcq(coreID, hwAmpChannel, num_samples_pre, num_samples_post,
                    num_shots);
            pasynManager->unlockPort(pasynUser);
            lock();

            if (status == asynSuccess) {
                /* FIXME: Improve FOFBStatus trigger waiting. The information
 *                  * about waiting for trigger is not totally accurate here.
 *                                   * Although, we will for SW or HW trigger in a short time,
 *                                                    * we are not actually there yet ...
 *                                                                     */
                if (trigger == ACQ_CLIENT_TRIG_EXTERNAL) {
                    setIntegerParam(coreID, P_FOFBStatus, FOFBStatusTriggerHwExtWaiting);
                }
                else if (trigger == ACQ_CLIENT_TRIG_DATA_DRIVEN) {
                    setIntegerParam(coreID, P_FOFBStatus, FOFBStatusTriggerHwDataWaiting);
                }
                else if (trigger == ACQ_CLIENT_TRIG_SOFTWARE) {
                    setIntegerParam(coreID, P_FOFBStatus, FOFBStatusTriggerSwWaiting);
                }

                callParamCallbacks(coreID);
            }
            else {
                asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                        "%s:%s: unable to acquire waveform\n",
                        driverName, functionName);
                /* Could not start acquisition. Invalid parameters */
                setIntegerParam(coreID, P_FOFBStatus, FOFBStatusErrAcq);
                callParamCallbacks(coreID);
                continue;
            }
        }

        /* Wait for acquisition to complete, but allow acquire stop events to be handled */
        while (1) {
            unlock();
            status = epicsEventWaitWithTimeout(this->abortAcqEventId[FOFBMode][coreID], pollTime);
            lock();
            if (status == epicsEventWaitOK) {
                /* We got a stop event, abort acquisition */
                abortAcq(coreID);
                setIntegerParam(coreID, P_FOFBStatus, FOFBStatusAborted);
                callParamCallbacks(coreID);
                break;
            }
            else {
                acqCompleted = checkAcqCompletion(coreID);
            }

            /* Acquisition overflow */
            if (acqCompleted == -1) {
                setIntegerParam(coreID, P_FOFBStatus, FOFBStatusErrAcqOFlow);
                callParamCallbacks(coreID);
                break;
            }

            if (acqCompleted == 1) {
                /* Get curve */
                unlock();
                pasynManager->lockPort(pasynUser);
                getAcqCurve(coreID, pArrayAllChannels, hwAmpChannel, num_samples_pre,
                        num_samples_post, num_shots);
                pasynManager->unlockPort(pasynUser);
                lock();
                break;
            }
        }

        /* Only do callbacks and calculate position if we could acquire some
 *          * data */
        if (acqCompleted == 1) {
            /* Do callbacks on the full waveform (all channels interleaved) */
            doCallbacksGenericPointer(pArrayAllChannels, NDArrayData,
                    channelMap[channel].NDArrayAmp[coreID][WVF_AMP_ALL]);

            /* Copy AMP data to arrays for each type of data, do callbacks on that */
            status = deinterleaveNDArray(pArrayAllChannels, channelMap[channel].NDArrayAmp[coreID],
                    dims[0], arrayCounter, &now);
            if (status != asynSuccess) {
                asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                        "%s:%s: unable to deinterleave NDArray\n",
                        driverName, functionName);
                continue;
            }
        }

        /* Release buffers */
        pArrayAllChannels->release();
        pArrayAllChannels = NULL;
        callParamCallbacks(coreID);

        /* If we are in repetitive mode then sleep for the acquire period minus elapsed time. */
        if (repetitiveTrigger[FOFBMode][coreID]) {
            epicsTimeGetCurrent(&endTime);
            elapsedTime = epicsTimeDiffInSeconds(&endTime, &startTime);
            delay = updateTime - elapsedTime;
            asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
                     "%s:%s: delay=%f\n",
                      driverName, functionName, delay);
            if (delay >= 0.0) {
                /* We set the status to indicate we are in the period delay */
                setIntegerParam(coreID, P_FOFBStatus, FOFBStatusWaiting);
                callParamCallbacks(coreID);
                unlock();
                epicsEventWaitWithTimeout(this->stopAcqEventId[FOFBMode][coreID], delay);
                lock();
            }
        }
    }
}

asynStatus drvFOFB::deinterleaveNDArray (NDArray *pArrayAllChannels, const int *pNDArrayAddr,
        int pNDArrayAddrSize, int arrayCounter, epicsTimeStamp *timeStamp)
{
    int status = asynSuccess;
    size_t dims[MAX_WVF_DIMS];
    NDArrayInfo_t arrayInfo;
    NDDataType_t NDType;
    NDDataType_t NDTypeSingle;
    size_t arrayYStride = 0;
    NDArray *pArraySingleChannel;
    epicsFloat64 *pInFloat64;
    epicsFloat64 *pOutFloat64;
    epicsInt32 *pIn32;
    epicsInt32 *pOut32;
    epicsInt16 *pIn16;
    epicsInt8 *pIn8;
    int channelAddr;
    static const char *functionName = "deinterleaveNDArray";

    status = pArrayAllChannels->getInfo(&arrayInfo);
    if (status != asynSuccess) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s: unable to get information about pArrayAllChannels\n",
                driverName, functionName);
        status = asynError;
        goto get_info_array_err;
    }

    arrayYStride = arrayInfo.yStride;
    dims[0] = arrayInfo.ySize;
    NDType = pArrayAllChannels->dataType;
    NDTypeSingle = NDType;
    /* Convert all integer types to 32-bit */
    if (NDTypeSingle == NDInt8 || NDTypeSingle == NDInt16) {
        NDTypeSingle = NDInt32;
    }

    for (int i = 0; i < pNDArrayAddrSize; ++i) {
        channelAddr = pNDArrayAddr[i];
        pArraySingleChannel = pNDArrayPool->alloc(1, dims, NDTypeSingle, 0, 0);
        if (pArraySingleChannel == NULL) {
            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s: unable to alloc pArraySingleChannel\n",
                driverName, functionName);
            status = asynError;
            goto alloc_ndarray_err;
        }

        pArraySingleChannel->uniqueId = arrayCounter;
        pArraySingleChannel->timeStamp = timeStamp->secPastEpoch + timeStamp->nsec / 1.e9;
        pArraySingleChannel->epicsTS.secPastEpoch = timeStamp->secPastEpoch;
        pArraySingleChannel->epicsTS.nsec = timeStamp->nsec;
        getAttributes(pArraySingleChannel->pAttributeList);

        pIn8 = (epicsInt8 *)pArrayAllChannels->pData;
        pIn16 = (epicsInt16 *)pArrayAllChannels->pData;
        pIn32 = (epicsInt32 *)pArrayAllChannels->pData;
        pOut32 = (epicsInt32 *)pArraySingleChannel->pData;
        pInFloat64 = (epicsFloat64 *)pArrayAllChannels->pData;
        pOutFloat64 = (epicsFloat64 *)pArraySingleChannel->pData;

        /* Get only a single channel samples from a multi-channel
 *          * array */
        switch (NDType) {
            case NDInt8:
                for (size_t j = 0; j < dims[0]; ++j) {
                    pOut32[j] = pIn8[i];
                    pIn8 += arrayYStride;
                }
                break;

            case NDInt16:
                for (size_t j = 0; j < dims[0]; ++j) {
                    pOut32[j] = pIn16[i];
                    pIn16 += arrayYStride;
                }
                break;

            case NDInt32:
                for (size_t j = 0; j < dims[0]; ++j) {
                    pOut32[j] = pIn32[i];
                    pIn32 += arrayYStride;
                }
                break;

            case NDFloat64:
                for (size_t j = 0; j < dims[0]; ++j) {
                    pOutFloat64[j] = pInFloat64[i];
                    pInFloat64 += arrayYStride;
                }
                break;

            default:
                asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                    "%s:%s: unsupported NDType of type: %d\n",
                    driverName, functionName, NDType);
                status = asynError;
                goto unsup_ndtype_err;
        }

        doCallbacksGenericPointer(pArraySingleChannel, NDArrayData,
                channelAddr);
        pArraySingleChannel->release();
    }

    return (asynStatus)status;

unsup_ndtype_err:
alloc_ndarray_err:
get_info_array_err:
    return (asynStatus)status;
}

asynStatus drvFOFB::setAcqEvent(epicsUInt32 mask, int addr)
{
    asynStatus status = asynSuccess;
    const char* functionName = "setAcqEvent";
    epicsUInt32 triggerEvent = 0;
    epicsUInt32 triggerType = 0;
    epicsUInt32 triggerRep = 0;
    epicsUInt32 hwAmpChannel = 0;
    int channel = 0;
    channelProp_t channelProp;

    /* Get the parameter in the parameter library. */
    getUIntDigitalParam(addr, P_TriggerEvent, &triggerEvent, 0xFFFFFFFF);
    getUIntDigitalParam(addr, P_Trigger, &triggerType, 0xFFFFFFFF);
    getUIntDigitalParam(addr, P_TriggerRep, &triggerRep, 0xFFFFFFFF);
    getIntegerParam(addr, P_Channel, &channel);

    /* Convert user channel into hw channel */
    hwAmpChannel = channelMap[channel].HwAmpChannel;
    if(hwAmpChannel < 0) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s: invalid HwAmpChannel channelMap for channel %d\n",
                driverName, functionName, hwAmpChannel);
        status = asynError;
        goto halcs_inv_channel;
    }

    /* Get channel properties */
    status = getChannelProperties(addr, hwAmpChannel, &channelProp);
    if (status) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
            "%s:%s: error calling getChannelProperties, status=%d\n",
            driverName, functionName, status);
        goto get_chan_prop_err;
    }

    setUIntDigitalParam(addr, P_ChannelSampleSize, channelProp.sampleSize, 0xFFFFFFFF);
    setUIntDigitalParam(addr, P_ChannelNumAtoms, channelProp.numAtoms, 0xFFFFFFFF);
    setUIntDigitalParam(addr, P_ChannelAtomWidth, channelProp.atomWidth, 0xFFFFFFFF);

    switch (triggerEvent) {
        case TRIG_ACQ_START:

            /* Don't try to change anything is we are still acquiring.
 *              * We must stop r abort the acquisition first */
            if (readingActive[FOFBMode][addr]) {
                asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
                    "%s:%s: Not starting acquistion as acqTask is still active\n",
                    driverName, functionName);
                break;
            }

            if (triggerRep) {
                repetitiveTrigger[FOFBMode][addr] = 1;
            }
            else {
                repetitiveTrigger[FOFBMode][addr] = 0;
            }

            status = setAcqTrig(addr, (acq_client_trig_e) triggerType);
            if (status) {
                asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                    "%s:%s: error calling setAcqTrig, status=%d\n",
                    driverName, functionName, status);
                goto get_set_acq_trig_err;
            }

            /* Send event telling the current task to proceed */
            epicsEventSignal(activeAcqEventId[FOFBMode][addr]);
            /* Signal acq thread to start acquisition with the current parameters */
            asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
                    "%s:%s: trigger TRIG_ACQ_START called\n",
                    driverName, functionName);
            epicsEventSignal(startAcqEventId[FOFBMode][addr]);
            break;

        /* Stop acquisition if we are in repetitive mode and if we are currently
 *          * acquiring. Otherwise, we don't need to do anything, as the acquisition
 *                   * task will stop after the current acquisition */
        case TRIG_ACQ_STOP: /* Trigger == Stop */
            stopAcqTask(addr, FOFBMode);
            break;

        /* Send the abort event if we are reading (repetitive or regular).
 *          *  If we want to stop a repetitive trigger, we must send a stop
 *                   *  event */
        case TRIG_ACQ_ABORT: /* Trigger == Abort */
            /* abort the other acquisition task if needed */
            abortAcqTask(addr, FOFBMode, true);
            break;

        default:
            asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
                    "%s:%s: invalid trigger event\n",
                    driverName, functionName);
            status = asynError;
            goto trig_unimplemented_err;
    }

trig_unimplemented_err:
get_set_acq_trig_err:
get_chan_prop_err:
halcs_inv_channel:
    return status;
}

asynStatus drvFOFB::abortAcqTask(int addr, int fofbMode, bool abortAcqHw)
{
    asynStatus status = asynSuccess;
    const char* functionName = "abortAcqTask";

    /* we are waiting for a trigger */
    if (readingActive[fofbMode][addr]) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
                "%s:%s: trigger ACQ_ABORT called for acqTask = %d, coreID = %d\n",
                driverName, functionName, fofbMode, addr);
        epicsEventSignal(this->abortAcqEventId[fofbMode][addr]);
    }
    else {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
                "%s:%s: trigger ACQ_ABORT but with acquisition in progress, "
                "called for acqTask = %d, coreID = %d\n",
                driverName, functionName, fofbMode, addr);
        /* If we are not actively waiting for an event on acqTask,
 *          * abort the acquisition anyway, as we might have something
 *                   * going on inside the FPGA from a previous acquisition */
        if (abortAcqHw) {
            abortAcqFromPortThread(addr);
        }
    }

    return status;
}

asynStatus drvFOFB::stopAcqTask(int addr, int fofbMode)
{
    asynStatus status = asynSuccess;
    const char* functionName = "stopAcqTask";

    /* We are in repetitive mode */
    if (readingActive[fofbMode][addr]) {
        repetitiveTrigger[fofbMode][addr] = 0;
        /* Send the stop event */
        asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
                "%s:%s: trigger ACQ_STOP called for acqTask = %d, coreID = %d\n",
                driverName, functionName, fofbMode, addr);
        epicsEventSignal(this->stopAcqEventId[fofbMode][addr]);
    }

    return status;
}

asynStatus drvFOFB::startAcq(int coreID, int hwChannel, epicsUInt32 num_samples_pre,
        epicsUInt32 num_samples_post, epicsUInt32 num_shots)
{
    asynStatus status = asynSuccess;
    halcs_client_err_e err = HALCS_CLIENT_SUCCESS;
    const char* functionName = "startAcq";
    char service[SERVICE_NAME_SIZE];
    acq_trans_t acq_trans;
    acq_req_t req;
    acq_block_t block;

    if (num_samples_pre + num_samples_post > MAX_ARRAY_POINTS) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s: unable to select %u pre-trigger samples and\n"
                "%u post-trigger samples for acquisition\n",
                driverName, functionName, num_samples_pre, num_samples_post);
        status = asynError;
        goto halcs_samples_sel_err;
    }

    /* Get correct service name*/
    status = getFullServiceName (this->fofbNumber, coreID, "ACQ", service, sizeof(service));
    if (status) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
            "%s:%s: error calling getFullServiceName, status=%d\n",
            driverName, functionName, status);
        goto get_service_err;
    }

    req.num_samples_pre  = num_samples_pre;
    req.num_samples_post = num_samples_post;
    req.num_shots = num_shots;
    req.chan = (uint32_t) hwChannel;

    /* Fill FOFB acquisition transaction structure */
    acq_trans = {req, block};

#ifdef TEST_SYNTH_DATA
    double t[TEST_LENGTH];
    for (int i = 0; i < TEST_LENGTH*4; ++i) {
        t[i] = (1/FREQ_SAMPLE)*i;
        ((int16_t *)pArrayAllChannels->pData)[i] = sin(2*PI*FREQ*t[i])*(1<<15);
    }
#else
    err = acq_start (fofbClientAcq[coreID], service, &req);
    if (err != HALCS_CLIENT_SUCCESS) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s: unable to acquire waveform on hwChannel %d, with %u\n"
                "\tpre-trigger samples and %u post-trigger samples\n",
                driverName, functionName, hwChannel, num_samples_pre,
                num_samples_post);
        status = asynError;
        goto halcs_acq_err;
    }
#endif

halcs_acq_err:
get_service_err:
halcs_samples_sel_err:
    return status;
}

asynStatus drvFOFB::abortAcqRaw(int coreID, acq_client_t *acq_client)
{
    asynStatus status = asynSuccess;
    halcs_client_err_e err = HALCS_CLIENT_SUCCESS;
    const char* functionName = "abortAcq";
    char service[SERVICE_NAME_SIZE];
    uint32_t fsm_stop = 1;

    /* Get correct service name*/
    status = getFullServiceName (this->fofbNumber, coreID, "ACQ", service, sizeof(service));
    if (status) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
            "%s:%s: error calling getFullServiceName, status=%d\n",
            driverName, functionName, status);
        goto get_service_err;
    }

    err = acq_set_fsm_stop (acq_client, service, fsm_stop);
    if (err != HALCS_CLIENT_SUCCESS) {
        status = asynError;
        goto halcs_acq_stop_err;
    }

halcs_acq_stop_err:
get_service_err:
    return status;
}

/* This must be called only acquisition threads */
asynStatus drvFOFB::abortAcq(int coreID)
{
    return abortAcqRaw(coreID, fofbClientAcq[coreID]);
}

/* This must be called only from asyn PortThread*/
asynStatus drvFOFB::abortAcqFromPortThread(int coreID)
{
    return abortAcqRaw(coreID, fofbClientAcqParam[coreID]);
}

int drvFOFB::checkAcqCompletion(int coreID)
{
    int complete = 0;
    asynStatus status = asynSuccess;
    halcs_client_err_e err = HALCS_CLIENT_SUCCESS;
    const char* functionName = "checkAcqCompletion";
    char service[SERVICE_NAME_SIZE];

    /* Get correct service name*/
    status = getFullServiceName (this->fofbNumber, coreID, "ACQ", service, sizeof(service));
    if (status) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
            "%s:%s: error calling getFullServiceName, status=%d\n",
            driverName, functionName, status);
        goto get_service_err;
    }

    err = acq_check (fofbClientAcq[coreID], service);
    /* FIFO Full error */
    if (err == HALCS_CLIENT_ERR_MODULE_ERR8) {
        complete = -1;
        goto halcs_acq_not_finished;
    }

    if (err != HALCS_CLIENT_SUCCESS) {
        complete = 0;
        goto halcs_acq_not_finished;
    }

    complete = 1;

halcs_acq_not_finished:
get_service_err:
    return complete;
}

asynStatus drvFOFB::getAcqCurve(int coreID, NDArray *pArrayAllChannels, int hwChannel,
        epicsUInt32 num_samples_pre, epicsUInt32 num_samples_post,
        epicsUInt32 num_shots)
{
    asynStatus status = asynSuccess;
    halcs_client_err_e err = HALCS_CLIENT_SUCCESS;
    const char* functionName = "getAcqCurve";
    char service[SERVICE_NAME_SIZE];
    acq_trans_t acq_trans;
    acq_req_t req;
    acq_block_t block;

    /* Get correct service name*/
    status = getFullServiceName (this->fofbNumber, coreID, "ACQ", service, sizeof(service));
    if (status) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
            "%s:%s: error calling getFullServiceName, status=%d\n",
            driverName, functionName, status);
        goto get_service_err;
    }

    req.num_samples_pre  = num_samples_pre;
    req.num_samples_post = num_samples_post;
    req.num_shots = num_shots;
    req.chan = (uint32_t) hwChannel;
    block.idx = 0;
    block.data = (uint32_t *)pArrayAllChannels->pData;
    block.data_size = (uint32_t)pArrayAllChannels->dataSize;
    block.bytes_read = 0;

    /* Fill FOFB acquisition transaction structure */
    acq_trans = {req, block};

    /* This just reads the data from memory */
    err = acq_get_curve (fofbClientAcq[coreID], service, &acq_trans);
    if (err != HALCS_CLIENT_SUCCESS) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s: unable to read waveform on hwChannel %d, with %u\n"
                "\tpre-trigger samples and %u post-trigger samples\n",
                driverName, functionName, hwChannel, num_samples_pre,
                num_samples_post);
        status = asynError;
        goto halcs_acq_err;
    }

halcs_acq_err:
get_service_err:
    return status;
}

asynStatus drvFOFB::getAcqNDArrayType(int coreID, int hwChannel, epicsUInt32 atomWidth, NDDataType_t *NDType)
{
    asynStatus status = asynSuccess;
    static const char *functionName = "getAcqNDArrayType";

    /* Determine minimum data size */
    switch (atomWidth) {
        case 2: /* bytes */
            *NDType = NDInt16;
            break;
        case 4: /* bytes */
            *NDType = NDInt32;
            break;
        default:
            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                    "%s:%s: unable to determine NDType for acquisition with atomWidth = %u\n",
                    driverName, functionName, atomWidth);
            status = asynError;
            goto get_ndarray_type_err;
    }

get_ndarray_type_err:
    return status;
}

asynStatus drvFOFB::getChannelProperties(int coreID, int channel, channelProp_t *channelProp)
{
    asynStatus status = asynSuccess;
    int err = HALCS_CLIENT_SUCCESS;
    const char* functionName = "getChannelProperties";
    char service[SERVICE_NAME_SIZE];

    /* Get correct service name*/
    status = getFullServiceName (this->fofbNumber, coreID, "ACQ", service, sizeof(service));
    if (status) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
            "%s:%s: error calling getFullServiceName, status=%d\n",
            driverName, functionName, status);
        goto get_service_err;
    }

    err = halcs_get_acq_ch_sample_size (fofbClientAcqParam[coreID], service,
        channel, &channelProp->sampleSize);
    err |= halcs_get_acq_ch_num_atoms (fofbClientAcqParam[coreID], service,
        channel, &channelProp->numAtoms);
    err |= halcs_get_acq_ch_atom_width (fofbClientAcqParam[coreID], service,
        channel, &channelProp->atomWidth);
    if (err != HALCS_CLIENT_SUCCESS) {
        status = asynError;
        goto halcs_get_sample_size_err;
    }

    asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
        "%s:%s: channel properties for coreID = %u, channel = %d:\n"
        "\tsampleSize = %u\n"
        "\tnumAtoms = %u\n"
        "\tatomWidth = %u\n",
        driverName, functionName, coreID, channel,
        channelProp->sampleSize, channelProp->numAtoms, channelProp->atomWidth);

halcs_get_sample_size_err:
get_service_err:
    return status;
}

/********************************************************************/
/********************* Asyn overrided methods  **********************/
/********************************************************************/

/*
 *  *  *  * Asyn overrided methods that are called by higher layers
 *   *   *   */

/** Called when asyn clients call pasynUInt32Digital->write().
 *  *  *  * For all parameters it sets the value in the parameter library and calls any registered callbacks..
 *   *   *   * \param[in] pasynUser pasynUser structure that encodes the reason and address.
 *    *    *    * \param[in] value Value to write. */
asynStatus drvFOFB::writeUInt32Digital(asynUser *pasynUser, epicsUInt32 value,
        epicsUInt32 mask)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    int addr = 0;
    const char *paramName = NULL;
    const char* functionName = "writeUInt32Digital";

    /* Get channel for possible use */
    status = getAddress(pasynUser, &addr);

    if (status) {
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                "%s:%s: status=%d, function=%d, name=%s, value=%d",
                driverName, functionName, status, function, paramName, value);
        return status;
    }

    if (function >= FIRST_COMMAND) {
        /* Set the parameter in the parameter library. */
        setUIntDigitalParam(addr, function, value, mask);

        /* Some operations need some special handling*/
        if (function == P_TriggerEvent) {
            /* If run was set then wake up the simulation task */
            status = setAcqEvent(mask, addr);
        }
        else if (function == P_DataTrigChan) {
            /* Ah... FIXME: ugly static mapping! */
            status = setDataTrigChan(mask, addr);
        }
        else {
            /* Fetch the parameter string name for possible use in debugging */
            getParamName(function, &paramName);

            /* Do operation on HW. Some functions do not set anything on hardware */
            status = setParam32(function, mask, addr);
        }
    }
    else {
        /* Call base class */
#if 0
        status = asynNDArrayDriver::writeUInt32DigitalInt32(pasynUser, value, mask);
#endif
    }

    /* Do callbacks so higher layers see any changes */
    callParamCallbacks(addr);

    if (status)
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                "%s:%s: status=%d, function=%d, name=%s, value=%d",
                driverName, functionName, status, function, paramName, value);
    else
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
                "%s:%s: function=%d, name=%s, value=%d\n",
                driverName, functionName, function, paramName, value);
    return status;
}

/** Called when asyn clients call pasynUInt32Digital->read().
 *  *  *  * For all parameters it gets the value in the parameter library..
 *   *   *   * \param[in] pasynUser pasynUser structure that encodes the reason and address.
 *    *    *    * \param[out] value Value to read. */
asynStatus drvFOFB::readUInt32Digital(asynUser *pasynUser, epicsUInt32 *value,
        epicsUInt32 mask)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    int addr = 0;
    const char *functionName = "readUInt32Digital";
    const char *paramName = NULL;

    /* Fetch the parameter string name for possible use in debugging */
    getParamName(function, &paramName);
    /* Get channel for possible use */
    status = getAddress(pasynUser, &addr);

    if (status) {
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                "%s:%s: status=%d, function=%d, name=%s",
                driverName, functionName, status, function, paramName);
        return status;
    }

    if (function >= FIRST_COMMAND) {
        if (function == P_DataTrigChan) {
            status = getDataTrigChan(value, mask, addr);
        }
        else {
            /* Get parameter, possibly from HW */
            status = getParam32(function, value, mask, addr);
        }
    }
    else {
#if 0
        /* Call base class */
        status = asynNDArrayDriver::readUIn32Digital(pasynUser, value, mask);
#endif
    }

    if (status)
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                "%s:%s: status=%d, function=%d, name=%s",
                driverName, functionName, status, function, paramName);
    else
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
                "%s:%s: function=%d, name=%s\n",
                driverName, functionName, function, paramName);

    return status;
}

/** Called when asyn clients call pasynInt32->write().
 *   * For all parameters it sets the value in the parameter library and calls any
 *     * registered callbacks..
 *       * \param[in] pasynUser pasynUser structure that encodes the reason and address.
 *         * \param[in] value Value to write. */
asynStatus drvFOFB::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    int addr = 0;
    const char *paramName = NULL;
    const char* functionName = "writeInt32";

    /* Fetch the parameter string name for possible use in debugging */
    getParamName(function, &paramName);
    /* Get channel for possible use */
    status = getAddress(pasynUser, &addr);

    if (status) {
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                "%s:%s: status=%d, function=%d, name=%s, value=%d",
                driverName, functionName, status, function, paramName, value);
        return status;
    }

    if (function >= FIRST_COMMAND) {
        /* Set the parameter in the parameter library. */
        setIntegerParam(addr, function, value);

        /* Do operation on HW. Some functions do not set anything on hardware */
        status = setParamInteger(function, addr);
    }
    else {
        /* Call base class */
        status = asynNDArrayDriver::writeInt32(pasynUser, value);
    }

    /* Do callbacks so higher layers see any changes */
    callParamCallbacks(addr);

    if (status)
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                "%s:%s: status=%d, function=%d, name=%s, value=%d",
                driverName, functionName, status, function, paramName, value);
    else
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
                "%s:%s: function=%d, name=%s, value=%d\n",
                driverName, functionName, function, paramName, value);
    return status;
}

/** Called when asyn clients call pasynInt32->read().
 *  * This does nothing for now and just call the base implementation. If needed,
 *   * add processing before calling the base class implementation
 *     * \param[in] pasynUser pasynUser structure that encodes the reason and address.
 *       * \param[in] value Value to read */
asynStatus drvFOFB::readInt32(asynUser *pasynUser, epicsInt32 *value)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    int addr = 0;
    const char *paramName = NULL;
    const char* functionName = "readInt32";

    /* Fetch the parameter string name for possible use in debugging */
    getParamName(function, &paramName);
    /* Get channel for possible use */
    status = getAddress(pasynUser, &addr);

    if (status) {
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                "%s:%s: status=%d, function=%d, name=%s",
                driverName, functionName, status, function, paramName);
        return status;
    }

    if (function >= FIRST_COMMAND) {
        /* Get parameter in library, as some parameters are not written in HW */
        status = getParamInteger(function, value, addr);
    }
    else {
        /* Call base class */
        status = asynNDArrayDriver::readInt32(pasynUser, value);
    }

    if (status)
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                "%s:%s: status=%d, function=%d, name=%s",
                driverName, functionName, status, function, paramName);
    else
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
                "%s:%s: function=%d, name=%s\n",
                driverName, functionName, function, paramName);
    return status;
}

/** Called when asyn clients call pasynFloat64->write().
 *   * \param[in] pasynUser pasynUser structure that encodes the reason and address.
 *     * \param[in] value Value to read */
asynStatus drvFOFB::writeFloat64(asynUser *pasynUser, epicsFloat64 value)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    int addr = 0;
    const char *paramName = NULL;
    const char* functionName = "writeFloat64";

    /* Fetch the parameter string name for possible use in debugging */
    getParamName(function, &paramName);
    /* Get channel for possible use */
    status = getAddress(pasynUser, &addr);

    if (status) {
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                "%s:%s: status=%d, function=%d, name=%s, value=%f",
                driverName, functionName, status, function, paramName, value);
        return status;
    }

    if (function >= FIRST_COMMAND) {
        /* Set the parameter in the parameter library. */
        setDoubleParam(addr, function, value);

        /* Do operation on HW. Some functions do not set anything on hardware */
        status = setParamDouble(function, addr);
    }
    else {
        /* Call base class */
        status = asynNDArrayDriver::writeFloat64(pasynUser, value);
    }

    /* Do callbacks so higher layers see any changes */
    callParamCallbacks(addr);

    if (status)
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                "%s:%s: status=%d, function=%d, name=%s, value=%f",
                driverName, functionName, status, function, paramName, value);
    else
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
                "%s:%s: function=%d, name=%s, value=%f\n",
                driverName, functionName, function, paramName, value);
    return status;
}

/** Called when asyn clients call pasynFloat64->read().
 *   * \param[in] pasynUser pasynUser structure that encodes the reason and address.
 *     * \param[in] value Value to read */
asynStatus drvFOFB::readFloat64(asynUser *pasynUser, epicsFloat64 *value)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    int addr = 0;
    const char *paramName = NULL;
    const char* functionName = "readFloat64";

    /* Fetch the parameter string name for possible use in debugging */
    getParamName(function, &paramName);
    /* Get channel for possible use */
    status = getAddress(pasynUser, &addr);

    if (status) {
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                "%s:%s: status=%d, function=%d, name=%s",
                driverName, functionName, status, function, paramName);
        return status;
    }

    /* Get double param, possibly from HW */
    if (function >= FIRST_COMMAND) {
        status = getParamDouble(function, value, addr);
    }
    else {
        /* Call base class */
        status = asynNDArrayDriver::readFloat64(pasynUser, value);
    }

    if (status)
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                "%s:%s: status=%d, function=%d, name=%s",
                driverName, functionName, status, function, paramName);
    else
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
                "%s:%s: function=%d, name=%s\n",
                driverName, functionName, function, paramName);
    return status;
}

/********************************************************************/
/************ Function Mapping Overloaded Write functions ***********/
/********************************************************************/

asynStatus drvFOFB::doExecuteHwWriteFunction(functionsInt32_t &func, char *service,
        int addr, functionsArgs_t &functionParam) const
{
    const char *functionName = "doExecuteHwWriteFunction<functionsInt32_t>";
    halcs_client_err_e err = HALCS_CLIENT_SUCCESS;
    int status = asynSuccess;

    /* Execute registered function */
    err = func.write(fofbClient, service, functionParam.argUInt32);
    if (err != HALCS_CLIENT_SUCCESS) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s: failure executing write function for service %s,"
                "param = %u\n",
                driverName, functionName, service, functionParam.argUInt32);
        status = asynError;
        goto halcs_set_func_param_err;
    }

halcs_set_func_param_err:
    return (asynStatus)status;
}

asynStatus drvFOFB::doExecuteHwWriteFunction(functionsUInt32_t &func, char *service,
        int addr, functionsArgs_t &functionParam) const
{
    const char *functionName = "doExecuteHwWriteFunction<functionsUInt32_t>";
    halcs_client_err_e err = HALCS_CLIENT_SUCCESS;
    int status = asynSuccess;

    /* Execute registered function */
    err = func.write(fofbClient, service, functionParam.argUInt32);
    if (err != HALCS_CLIENT_SUCCESS) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s: failure executing write function for service %s,"
                "param = %u\n",
                driverName, functionName, service, functionParam.argUInt32);
        status = asynError;
        goto halcs_set_func_param_err;
    }

halcs_set_func_param_err:
    return (asynStatus)status;
}

asynStatus drvFOFB::executeHwWriteFunction(int functionId, int addr,
        functionsArgs_t &functionParam)
{
    int status = asynSuccess;
    const char *functionName = "executeHwWriteFunction";
    const char *funcService = NULL;
    char service[SERVICE_NAME_SIZE];
    const char *paramName = NULL;
    std::unordered_map<int,functionsAny_t>::iterator func;

    /* Lookup function on map */
    func = fofbHwFunc.find (functionId);
    if (func == fofbHwFunc.end()) {
        getParamName(functionId, &paramName);
        /* This is not an error. Exit silently */
        status = asynSuccess;
        asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
                "%s:%s: no registered function for functionID = %d, name %s\n",
                driverName, functionName, functionId, paramName);
        goto get_reg_func_err;
    }

    /* Get service name from structure */
    funcService = func->second.getServiceName(*this);
    /* Create full service name*/
    status = getFullServiceName (this->fofbNumber, addr, funcService,
            service, sizeof(service));
    if (status) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s: error calling getFullServiceName, status=%d\n",
                driverName, functionName, status);
        goto get_service_err;
    }

    /* Execute overloaded function for each function type we know of */
    status = func->second.executeHwWrite(*this, service, addr, functionParam);

get_reg_func_err:
get_service_err:
        return (asynStatus)status;
}

asynStatus drvFOFB::doExecuteHwWriteFunction(functionsUInt32Chan_t &func, char *service,
        int addr, functionsArgs_t &functionParam) const
{
    const char *functionName = "doExecuteHwWriteFunction<functionsUInt32Chan_t>";
    halcs_client_err_e err = HALCS_CLIENT_SUCCESS;
    int status = asynSuccess;
    char serviceChanStr[SERVICE_NAME_SIZE];
    epicsUInt32 serviceChan = 0;

    /* Create full service name*/
    status = getFullServiceName (this->fofbNumber, addr, func.serviceName,
            serviceChanStr, sizeof(serviceChanStr));
    if (status) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s: error calling getFullServiceName, status=%d\n",
                driverName, functionName, status);
        goto get_service_err;
    }

    /* Get correct service channel */
    getServiceChan (this->fofbNumber, addr, func.serviceName, &serviceChan);

    /* Execute registered function */
    err = func.write(fofbClient, serviceChanStr, serviceChan, functionParam.argUInt32);
    if (err != HALCS_CLIENT_SUCCESS) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s: failure executing write function for service %s,"
                "param = %u\n",
                driverName, functionName, serviceChanStr,
                functionParam.argUInt32);
        status = asynError;
        goto halcs_set_func_param_err;
    }

halcs_set_func_param_err:
get_service_err:
    return (asynStatus) status;
}

asynStatus drvFOFB::doExecuteHwWriteFunction(functionsUInt32Acq_t &func, char *service,
        int addr, functionsArgs_t &functionParam) const
{
    const char *functionName = "doExecuteHwWriteFunction<functionsUInt32Acq_t>";
    halcs_client_err_e err = HALCS_CLIENT_SUCCESS;
    int status = asynSuccess;
    int serviceID = 0;

    /* Get service ID for correct use with acquisition instance */
    status = getServiceID (this->fofbNumber, addr, func.serviceName,
            &serviceID);
    if (status) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s: error calling getServiceID, status=%d\n",
                driverName, functionName, status);
        goto get_service_id_err;
    }

    /* Execute registered function */
    err = func.write(fofbClientAcqParam[serviceID], service, functionParam.argUInt32);
    if (err != HALCS_CLIENT_SUCCESS) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s: failure executing write function for service %s,"
                "param = %u\n",
                driverName, functionName, service, functionParam.argUInt32);
        status = asynError;
        goto halcs_set_func_param_err;
    }

halcs_set_func_param_err:
get_service_id_err:
    return (asynStatus) status;
}

asynStatus drvFOFB::doExecuteHwWriteFunction(functions2UInt32_t &func, char *service,
        int addr, functionsArgs_t &functionParam) const
{
    const char *functionName = "doExecuteHwWriteFunction<functions2UInt32_t>";
    halcs_client_err_e err = HALCS_CLIENT_SUCCESS;
    int status = asynSuccess;
    epicsUInt32 param1 = 0;
    epicsUInt32 param2 = 0;

    /* Silently exit if no function is registered */
    if(!func.read) {
        status = asynSuccess;
        goto no_registered_read_func_err;
    }

    /* Read the HW values first as we need to update
 *        only one of the parameters */
    err = func.read(fofbClient, service, &param1, &param2);
    if (err != HALCS_CLIENT_SUCCESS) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s: failure executing read function for service %s\n",
                driverName, functionName, service);
        status = asynError;
        goto halcs_get_func_param_err;
    }

    /* Determine if we want to change the first or second parameter in HW */
    if (func.parameterPos == 1) {
        err = func.write(fofbClient, service, functionParam.argUInt32, param2);
    }
    else if (func.parameterPos == 2) {
        err = func.write(fofbClient, service, param1, functionParam.argUInt32);
    }

    if (err != HALCS_CLIENT_SUCCESS) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s: failure executing write function for service %s,"
                "param = %u\n",
                driverName, functionName, service, functionParam.argUInt32);
        status = asynError;
        goto halcs_set_func_param_err;
    }

halcs_set_func_param_err:
halcs_get_func_param_err:
no_registered_read_func_err:
        return (asynStatus) status;
}

asynStatus drvFOFB::doExecuteHwWriteFunction(functionsFloat64_t &func, char *service,
        int addr, functionsArgs_t &functionParam) const
{
    const char *functionName = "doExecuteHwWriteFunction<functionsFloat64_t>";
    halcs_client_err_e err = HALCS_CLIENT_SUCCESS;
    int status = asynSuccess;

    /* Execute registered function */
    err = func.write(fofbClient, service, functionParam.argFloat64);
    if (err != HALCS_CLIENT_SUCCESS) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s: failure executing write function for service %s,"
                "param = %f\n",
                driverName, functionName, service, functionParam.argFloat64);
        status = asynError;
        goto halcs_set_func_param_err;
    }

halcs_set_func_param_err:
    return (asynStatus) status;
}

/********************************************************************/
/************ Function Mapping Overloaded Read functions ************/
/********************************************************************/

asynStatus drvFOFB::doExecuteHwReadFunction(functionsInt32_t &func, char *service,
        int addr, functionsArgs_t &functionParam) const
{
    const char *functionName = "doExecuteHwReadFunction<functionsInt32_t>";
    halcs_client_err_e err = HALCS_CLIENT_SUCCESS;
    int status = asynSuccess;

    /* Execute registered function */
    err = func.read(fofbClient, service, &functionParam.argUInt32);
    if (err != HALCS_CLIENT_SUCCESS) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s: failure executing read function for service %s\n",
                driverName, functionName, service);
        status = asynError;
        goto halcs_get_func_param_err;
    }

halcs_get_func_param_err:
    return (asynStatus)status;
}
asynStatus drvFOFB::doExecuteHwReadFunction(functionsUInt32_t &func, char *service,
        int addr, functionsArgs_t &functionParam) const
{
    const char *functionName = "doExecuteHwReadFunction<functionsUInt32_t>";
    halcs_client_err_e err = HALCS_CLIENT_SUCCESS;
    int status = asynSuccess;

    /* Execute registered function */
    err = func.read(fofbClient, service, &functionParam.argUInt32);
    if (err != HALCS_CLIENT_SUCCESS) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s: failure executing read function for service %s\n",
                driverName, functionName, service);
        status = asynError;
        goto halcs_get_func_param_err;
    }

halcs_get_func_param_err:
    return (asynStatus)status;
}

asynStatus drvFOFB::executeHwReadFunction(int functionId, int addr,
        functionsArgs_t &functionParam)
{
    int status = asynSuccess;
    const char *functionName = "executeHwReadFunction";
    const char *funcService = NULL;
    char service[SERVICE_NAME_SIZE];
    const char *paramName = NULL;
    std::unordered_map<int,functionsAny_t>::iterator func;

    /* Lookup function on map */
    func = fofbHwFunc.find (functionId);
    if (func == fofbHwFunc.end()) {
        getParamName(functionId, &paramName);
        /* We use disabled to indicate the function was not found on Hw mapping */
        status = asynDisabled;
        asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
                "%s:%s: no registered function for functionID = %d, name %s\n",
                driverName, functionName, functionId, paramName);
        goto get_reg_func_err;
    }

    /* Get service name from structure */
    funcService = func->second.getServiceName(*this);
    /* Create full service name*/
    status = getFullServiceName (this->fofbNumber, addr, funcService,
            service, sizeof(service));
    if (status) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s: error calling getFullServiceName, status=%d\n",
                driverName, functionName, status);
        goto get_service_err;
    }

    /* Execute overloaded function for each function type we know of */
    status = func->second.executeHwRead(*this, service, addr, functionParam);

get_reg_func_err:
get_service_err:
        return (asynStatus)status;
}

asynStatus drvFOFB::doExecuteHwReadFunction(functionsUInt32Chan_t &func, char *service,
        int addr, functionsArgs_t &functionParam) const
{
    const char *functionName = "doExecuteHwReadFunction<functionsUInt32Chan_t>";
    halcs_client_err_e err = HALCS_CLIENT_SUCCESS;
    char serviceChanStr[SERVICE_NAME_SIZE];
    int status = asynSuccess;
    epicsUInt32 serviceChan = 0;

    /* Create full service name*/
    status = getFullServiceName (this->fofbNumber, addr, func.serviceName,
            serviceChanStr, sizeof(serviceChanStr));
    if (status) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s: error calling getFullServiceName, status=%d\n",
                driverName, functionName, status);
        goto get_service_err;
    }

    /* Get correct service channel */
    getServiceChan (this->fofbNumber, addr, func.serviceName, &serviceChan);

    /* Execute registered function */
    err = func.read(fofbClient, serviceChanStr, serviceChan, &functionParam.argUInt32);
    if (err != HALCS_CLIENT_SUCCESS) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s: failure executing read function for service %s\n",
                driverName, functionName, serviceChanStr);
        status = asynError;
        goto halcs_get_func_param_err;
    }

halcs_get_func_param_err:
get_service_err:
    return (asynStatus) status;
}

asynStatus drvFOFB::doExecuteHwReadFunction(functionsUInt32Acq_t &func, char *service,
        int addr, functionsArgs_t &functionParam) const
{
    const char *functionName = "doExecuteHwReadFunction<functionsUInt32Acq_t>";
    halcs_client_err_e err = HALCS_CLIENT_SUCCESS;
    int status = asynSuccess;
    int serviceID = 0;

    /* Get service ID for correct use with acquisition instance */
    status = getServiceID (this->fofbNumber, addr, func.serviceName,
            &serviceID);
    if (status) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s: error calling getServiceID, status=%d\n",
                driverName, functionName, status);
        goto get_service_id_err;
    }

    /* Execute registered function */
    err = func.read(fofbClientAcqParam[serviceID], service, &functionParam.argUInt32);
    if (err != HALCS_CLIENT_SUCCESS) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s: failure executing read function for service %s\n",
                driverName, functionName, service);
        status = asynError;
        goto halcs_get_func_param_err;
    }

halcs_get_func_param_err:
get_service_id_err:
    return (asynStatus) status;
}

asynStatus drvFOFB::doExecuteHwReadFunction(functions2UInt32_t &func, char *service,
        int addr, functionsArgs_t &functionParam) const
{
    const char *functionName = "doExecuteHwReadFunction<functions2UInt32_t>";
    halcs_client_err_e err = HALCS_CLIENT_SUCCESS;
    int status = asynSuccess;
    epicsUInt32 param1 = 0;
    epicsUInt32 param2 = 0;

    /* Read the HW values first as we need to update
 *        only one of the parameters */
    err = func.read(fofbClient, service, &param1, &param2);
    if (err != HALCS_CLIENT_SUCCESS) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s: failure executing read function for service %s\n",
                driverName, functionName, service);
        status = asynError;
        goto halcs_get_func_param_err;
    }

    /* Determine if we want to read the first or second parameter */
    if (func.parameterPos == 1) {
        functionParam.argUInt32 = param1;
    }
    else if (func.parameterPos == 2) {
        functionParam.argUInt32 = param2;
    }

halcs_get_func_param_err:
        return (asynStatus) status;
}

asynStatus drvFOFB::doExecuteHwReadFunction(functionsFloat64_t &func, char *service,
        int addr, functionsArgs_t &functionParam) const
{
    const char *functionName = "doExecuteHwReadFunction<functionsFloat64_t>";
    halcs_client_err_e err = HALCS_CLIENT_SUCCESS;
    int status = asynSuccess;

    /* Execute registered function */
    err = func.read(fofbClient, service, &functionParam.argFloat64);
    if (err != HALCS_CLIENT_SUCCESS) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s: failure executing read function for service %s\n",
                driverName, functionName, service);
        status = asynError;
        goto halcs_get_func_param_err;
    }

halcs_get_func_param_err:
    return (asynStatus) status;
}

/********************************************************************/
/********* Generic 32-bit/Double FOFB Processing Operations *********/
/********************************************************************/

/*
 *  *  * * 32-bit/Double generic FOFB Processing operations. These will map to real
 *   *   * * functions defined in the structures. e.g., functionsInt32_t
 *    *    * * and functionsFloat64_t
 *     *     * */

asynStatus drvFOFB::setParamGeneric(int functionId, int addr)
{
    int status = asynSuccess;
    const char *functionName = "setParamGeneric";
    const char *paramName = NULL;
    asynParamType asynType = asynParamNotDefined;

    getParamName(functionId, &paramName);
    status = getParamType(addr, functionId, &asynType);
    if (status != asynSuccess) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s: getParamType failure retrieving asynParamType, "
                "functionId = %d, paramName = %s\n",
                driverName, functionName, functionId, paramName);
        goto get_type_err;
    }

    switch (asynType) {
        case asynParamInt32:
            status = setParamInteger(functionId, addr);
        break;

        case asynParamUInt32Digital:
            status = setParam32(functionId, 0xFFFFFFFF, addr);
        break;

        case asynParamFloat64:
            status = setParamDouble(functionId, addr);
        break;

        default:
            asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                    "%s:%s: unsupported type for asynParamType: %d, "
                    "functionId = %d, paramName = %s\n",
                    driverName, functionName, asynType,
                    functionId, paramName);
            goto unsup_asyn_type;
    }

    if (status != asynSuccess) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s: setParam32/setParamDouble failure setting value %d, "
                "for functionId = %d, paramName = %s\n",
                driverName, functionName, status, functionId, paramName);
        goto set_type_err;
    }

set_type_err:
unsup_asyn_type:
get_type_err:
    return (asynStatus)status;
}


asynStatus drvFOFB::setParam32(int functionId, epicsUInt32 mask, int addr)
{
    int status = asynSuccess;
    functionsArgs_t functionArgs = {0};
    const char *functionName = "setParam32";
    const char *paramName = NULL;

    status = getUIntDigitalParam(addr, functionId, &functionArgs.argUInt32, mask);
    if (status != asynSuccess) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s: getUIntDigitalParam failure for retrieving parameter %s\n",
                driverName, functionName, paramName);
        goto get_param_err;
    }

    status = executeHwWriteFunction(functionId, addr, functionArgs);

get_param_err:
    return (asynStatus)status;
}

asynStatus drvFOFB::getParam32(int functionId, epicsUInt32 *param,
        epicsUInt32 mask, int addr)
{
    int status = asynSuccess;
    functionsArgs_t functionArgs = {0};
    const char *functionName = "getParam32";
    const char *paramName = NULL;

    /* Get parameter in library, as some parameters are not written in HW */
    status = getUIntDigitalParam(addr, functionId, param, mask);
    if (status != asynSuccess) {
        if (status != asynParamUndefined) {
            getParamName(functionId, &paramName);
            asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                    "%s:%s: getUIntDigitalParam failure for retrieving parameter %s, status = %d\n",
                    driverName, functionName, paramName, status);
        }
        goto get_param_err;
    }

    status = executeHwReadFunction(functionId, addr, functionArgs);
    if (status == asynSuccess) {
        /* Mask parameter according to the received mask */
        functionArgs.argUInt32 &= mask;
        *param = functionArgs.argUInt32;
    }
    /* We recover from asynDisabled just by retrieving
 *  *  *      * the parameter from the list */
    else if (status == asynDisabled){
        status = asynSuccess;
    }

get_param_err:
    return (asynStatus)status;
}

asynStatus drvFOFB::setParamInteger(int functionId, int addr)
{
    int status = asynSuccess;
    functionsArgs_t functionArgs = {0};
    const char *functionName = "setParamInteger";

    status = getIntegerParam(addr, functionId, &functionArgs.argInt32);
    if (status != asynSuccess) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s: getIntegerParam failure for retrieving Parameter\n",
                driverName, functionName);
        goto get_param_err;
    }

    status = executeHwWriteFunction(functionId, addr, functionArgs);

get_param_err:
    return (asynStatus)status;
}

asynStatus drvFOFB::getParamInteger(int functionId, epicsInt32 *param,
        int addr)
{
    int status = asynSuccess;
    functionsArgs_t functionArgs = {0};
    const char *functionName = "getParamInteger";
    const char *paramName;

    /* Get parameter in library, as some parameters are not written in HW */
    status = getIntegerParam(addr, functionId, param);
    if (status != asynSuccess) {
        if (status != asynParamUndefined) {
            getParamName(functionId, &paramName);
            asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                    "%s:%s: getIntegerParam failure for retrieving parameter %s, status = %d\n",
                    driverName, functionName, paramName, status);
        }
        goto get_param_err;
    }

    status = executeHwReadFunction(functionId, addr, functionArgs);
    if (status == asynSuccess) {
        *param = functionArgs.argInt32;
    }
    /* We recover from asynDisabled just by retrieving
 *      * the parameter from the list */
    else if (status == asynDisabled){
        status = asynSuccess;
    }

get_param_err:
    return (asynStatus)status;
}

asynStatus drvFOFB::setParamDouble(int functionId, int addr)
{
    asynStatus status = asynSuccess;
    functionsArgs_t functionArgs = {0};
    const char *functionName = "setParamDouble";

    status = getDoubleParam(addr, functionId, &functionArgs.argFloat64);
    if (status != asynSuccess) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s: getUIntDigitalParam failure for retrieving Parameter\n",
                driverName, functionName);
        goto get_param_err;
    }

    status = executeHwWriteFunction(functionId, addr, functionArgs);

get_param_err:
    return status;
}

asynStatus drvFOFB::getParamDouble(int functionId, epicsFloat64 *param, int addr)
{
    asynStatus status = asynSuccess;
    functionsArgs_t functionArgs = {0};
    const char *functionName = "getParamDouble";
    const char *paramName;

    /* Get parameter in library, as some parameters are not written in HW */
    status = getDoubleParam(addr, functionId, param);
    if (status != asynSuccess) {
        if (status != asynParamUndefined) {
            getParamName(functionId, &paramName);
            asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                    "%s:%s: getDoubleParam failure for retrieving parameter %s\n",
                    driverName, functionName, paramName);
        }
        goto get_param_err;
    }

    status = executeHwReadFunction(functionId, addr, functionArgs);
    if (status == asynSuccess) {
        *param = functionArgs.argFloat64;
    }
    /* We recover from asynDisabled just by retrieving
 *      * the parameter from the list */
    else if (status == asynDisabled){
        status = asynSuccess;
    }

get_param_err:
    return status;
}

/********************************************************************/
/*********************** Misc FOFB Operations ***********************/
/********************************************************************/

/*
 *  *  *  * Miscellaneous functions that don't map easily
 *   *   *   * to our generic handlers get/setParam[32/Double]
 *    *    *    */

asynStatus drvFOFB::setDataTrigChan(epicsUInt32 mask, int addr)
{
    halcs_client_err_e err = HALCS_CLIENT_SUCCESS;
    char service[SERVICE_NAME_SIZE];
    int status = asynSuccess;
    const char* functionName = "setDataTrigChan";
    epicsUInt32 dataTrigChan = 0;
    int hwAmpChannel = 0;
    int serviceID = 0;

    /* Set the parameter in the parameter library. */
    getUIntDigitalParam(addr, P_DataTrigChan, &dataTrigChan, mask);

    /* Convert user channel into hw channel */
    hwAmpChannel = channelMap[dataTrigChan].HwAmpChannel;
    if(hwAmpChannel < 0) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s: invalid HwAmpChannel channelMap for channel %d\n",
                driverName, functionName, hwAmpChannel);
        status = asynError;
        goto halcs_inv_channel;
    }

    /* Get correct service name*/
    status = getFullServiceName (this->fofbNumber, addr, "ACQ",
            service, sizeof(service));
    status |= getServiceID (this->fofbNumber, addr, "ACQ", &serviceID);
    if (status) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s: error calling getFullServiceName, status=%d\n",
                driverName, functionName, status);
        goto get_service_err;
    }

    err = acq_set_data_trig_chan (fofbClientAcqParam[serviceID], service, hwAmpChannel);
    if (err != HALCS_CLIENT_SUCCESS) {
        status = asynError;
        goto halcs_set_data_trig_chan_err;
    }

halcs_set_data_trig_chan_err:
get_service_err:
halcs_inv_channel:
    return (asynStatus)status;
}

asynStatus drvFOFB::getDataTrigChan(epicsUInt32 *channel, epicsUInt32 mask, int addr)
{
    halcs_client_err_e err = HALCS_CLIENT_SUCCESS;
    char service[SERVICE_NAME_SIZE];
    int status = asynSuccess;
    const char* functionName = "getDataTrigChan";
    epicsUInt32 dataTrigChan = 0;
    epicsUInt32 hwAmpChannel = 0;
    int serviceID = 0;

    /* Get correct service name*/
    status = getFullServiceName (this->fofbNumber, addr, "ACQ",
            service, sizeof(service));
    status |= getServiceID (this->fofbNumber, addr, "ACQ", &serviceID);
    if (status) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s: error calling getFullServiceName, status=%d\n",
                driverName, functionName, status);
        goto get_service_err;
    }

    /* Clear parameter in case of an error occurs */
    *channel = 0;

    err = acq_get_data_trig_chan (fofbClientAcqParam[serviceID], service, &hwAmpChannel);
    if (err != HALCS_CLIENT_SUCCESS) {
        status = asynError;
        goto halcs_get_data_trig_chan_err;
    }

    if (hwAmpChannel > CH_HW_END-1) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s: invalid HwAmpChannel channelRevMap for channel %d\n",
                driverName, functionName, hwAmpChannel);
        status = asynError;
        goto halcs_inv_hw_channel;
    }

    /* Convert user channel into hw channel */
    dataTrigChan = channelRevMap[hwAmpChannel].epicsChannel;
    if(dataTrigChan < 0) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s: invalid channel channelRevMap for channel %d\n",
                driverName, functionName, dataTrigChan);
        status = asynError;
        goto halcs_inv_channel;
    }

    /* Mask parameter according to the received mask */
    dataTrigChan &= mask;
    *channel = dataTrigChan;

halcs_inv_channel:
halcs_inv_hw_channel:
halcs_get_data_trig_chan_err:
get_service_err:
    return (asynStatus)status;
}

asynStatus drvFOFB::updateUInt32Params(epicsUInt32 mask, int addr, int firstParam,
        int lastParam, bool acceptErrors)
{
    int status = asynSuccess;
    int errs = 0;
    const char* functionName = "updateUInt32Params";
    epicsUInt32 param = 0;

    asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
        "%s:%s: updating UInt32 parameters with firstParam = %d, lastParam = %d, "
        "addr = %d\n",
        driverName, functionName, firstParam, lastParam, addr);

    for (int i = firstParam; i < lastParam+1; ++i) {
        status = getParam32(i, &param, mask, addr);
        /* Only write values if there is no error */
        if (status) {
            if (status != asynParamUndefined) {
                asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                        "%s:%s: error getting UInt32 parameter for function = %d, "
                        "addr = %d status = %d\n",
                        driverName, functionName, i, addr, status);
            }
            ++errs;
        }
        else {
            setUIntDigitalParam(addr, i, param, mask);
        }
    }

    if (acceptErrors) {
        return asynSuccess;
    }

    return (errs == 0)? asynSuccess : asynError;
}

asynStatus drvFOFB::updateIntegerParams(int addr, int firstParam,
        int lastParam, bool acceptErrors)
{
    int status = asynSuccess;
    int errs = 0;
    const char* functionName = "updateIntegerParams";
    epicsInt32 param = 0;

    asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
        "%s:%s: updating Int32 parameters with firstParam = %d, lastParam = %d, "
        "addr = %d\n",
        driverName, functionName, firstParam, lastParam, addr);

    for (int i = firstParam; i < lastParam+1; ++i) {
        status = getParamInteger(i, &param, addr);
        /* Only write values if there is no error */
        if (status) {
            if (status != asynParamUndefined) {
                asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                        "%s:%s: error getting Int32 parameter for function = %d, "
                        "addr = %d status = %d\n",
                        driverName, functionName, i, addr, status);
            }
            ++errs;
        }
        else {
            setIntegerParam(addr, i, param);
        }
    }

    if (acceptErrors) {
        return asynSuccess;
    }

    return (errs == 0)? asynSuccess : asynError;
}

asynStatus drvFOFB::updateDoubleParams(int addr, int firstParam, int lastParam,
        bool acceptErrors)
{
    int status = asynSuccess;
    int errs = 0;
    const char* functionName = "updateDoubleParams";
    epicsFloat64 param = 0.0;

    asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
        "%s:%s: updating Double parameters with firstParam = %d, lastParam = %d, "
        "addr = %d\n",
        driverName, functionName, firstParam, lastParam, addr);

    for (int i = firstParam; i < lastParam+1; ++i) {
        status = getParamDouble(i, &param, addr);
        /* Only write values is there is no error */
        if (status) {
            if (status != asynParamUndefined) {
                asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                        "%s:%s: error getting Double parameter for function = %d, "
                        "addr = %d status = %d\n",
                        driverName, functionName, i, addr, status);
            }
            ++errs;
        }
        else {
            setDoubleParam(addr, i, param);
        }
    }

    if (acceptErrors) {
        return asynSuccess;
    }

    return (errs == 0)? asynSuccess : asynError;
}

asynStatus drvFOFB::readTriggerParams(epicsUInt32 mask, int addr)
{
    return updateUInt32Params(mask, addr, P_TriggerDir, P_TriggerTrnOutSel, false);
}

/* Configuration routine.  Called directly, or from the iocsh function below */
extern "C" {

    /** EPICS iocsh callable function to call constructor for the drvFOFB class.
 *  *  *      * \param[in] portName The name of the asyn port driver to be created.
 *   *   *           * \param[in] endpoint The address device string */
    int drvFOFBConfigure(const char *portName, const char *endpoint,
            int fofbNumber, const char *type, int verbose, int timeout,
            int maxPoints, int maxBuffers, size_t maxMemory)
    {
        new drvFOFB(portName, endpoint, fofbNumber, type, verbose,
                timeout, maxPoints, maxBuffers, maxMemory);
        return(asynSuccess);
    }

    /* EPICS iocsh shell commands */
    static const iocshArg initArg0 = { "portName", iocshArgString};
    static const iocshArg initArg1 = { "endpoint", iocshArgString};
    static const iocshArg initArg2 = { "fofbNumber", iocshArgInt};
    static const iocshArg initArg3 = { "type", iocshArgString};
    static const iocshArg initArg4 = { "verbose", iocshArgInt};
    static const iocshArg initArg5 = { "timeout", iocshArgInt};
    static const iocshArg initArg6 = { "maxPoints", iocshArgInt};
    static const iocshArg initArg7 = { "maxBuffers", iocshArgInt};
    static const iocshArg initArg8 = { "maxMemory", iocshArgInt};
    static const iocshArg * const initArgs[] = {&initArg0,
        &initArg1,
        &initArg2,
        &initArg3,
        &initArg4,
        &initArg5,
        &initArg6,
        &initArg7,
        &initArg8};
    static const iocshFuncDef initFuncDef = {"drvFOFBConfigure",9,initArgs};
    static void initCallFunc(const iocshArgBuf *args)
    {
        drvFOFBConfigure(args[0].sval, args[1].sval, args[2].ival,
                args[3].sval, args[4].ival, args[5].ival, args[6].ival,
                args[7].ival, args[8].ival);
    }

    void drvFOFBRegister(void)
    {
        iocshRegister(&initFuncDef,initCallFunc);
    }

    epicsExportRegistrar(drvFOFBRegister);
}

