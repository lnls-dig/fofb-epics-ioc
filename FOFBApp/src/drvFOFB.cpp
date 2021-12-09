/*
 *  *  * drvFOFB.cpp
 *   *   *
 *    *    * Authors: Melissa Aguiar
 *     *     *
 *      *      * Created Dec. 03, 2021
 *       *       */

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

#define SERVICE_NAME_SIZE               50

static const boardMap_t boardMap[MAX_FOFBS+1] = {
    /* board,   fofb*/
    /* 0 (INVALID)  */ {-1, -1},
    /* 1            */ {1,   0},
    /* 2            */ {1,   1},
    /* 3            */ {2,   0},
    /* 4            */ {2,   1},
    /* 5            */ {3,   0},
    /* 6            */ {3,   1},
    /* 7            */ {4,   0},
    /* 8            */ {4,   1},
    /* 9            */ {5,   0},
    /* 10           */ {5,   1},
    /* 11           */ {6,   0},
    /* 12           */ {6,   1},
    /* 13           */ {7,   0},
    /* 14           */ {7,   1},
    /* 15           */ {8,   0},
    /* 16           */ {8,   1},
    /* 17           */ {9,   0},
    /* 18           */ {9,   1},
    /* 19           */ {10,  0},
    /* 20           */ {10,  1},
    /* 21           */ {11,  0},
    /* 22           */ {11,  1},
    /* 23           */ {12,  0},
    /* 24           */ {12,  1}
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
static const functionsAny_t fofbCtrlSetGetActPartFunc                 = {functionsUInt32_t{"FOFB_CC", halcs_set_fofb_ctrl_act_part,
                                                                          halcs_get_fofb_ctrl_act_part}};
static const functionsAny_t fofbCtrlSetGeErrClrFunc                   = {functionsUInt32_t{"FOFB_CC", halcs_set_fofb_ctrl_err_clr,
                                                                          halcs_get_fofb_ctrl_err_clr}};
static const functionsAny_t fofbCtrlSetGetCcEnableFunc                = {functionsUInt32_t{"FOFB_CC", halcs_set_fofb_ctrl_cc_enable,
                                                                          halcs_get_fofb_ctrl_cc_enable}};
static const functionsAny_t fofbCtrlSetGetTfsOverrideFunc             = {functionsUInt32_t{"FOFB_CC", halcs_set_fofb_ctrl_tfs_override,
                                                                          halcs_get_fofb_ctrl_tfs_override}};
static const functionsAny_t fofbCtrlSetGetBpmIdFunc                   = {functionsUInt32_t{"FOFB_CC", halcs_set_fofb_ctrl_bpm_id,
                                                                          halcs_get_fofb_ctrl_bpm_id}};
static const functionsAny_t fofbCtrlSetGetTimeFrameLenFunc            = {functionsUInt32_t{"FOFB_CC", halcs_set_fofb_ctrl_time_frame_len,
                                                                          halcs_get_fofb_ctrl_time_frame_len}};
static const functionsAny_t fofbCtrlSetGetMgtPowerdownFunc            = {functionsUInt32_t{"FOFB_CC", halcs_set_fofb_ctrl_mgt_powerdown,
                                                                          halcs_get_fofb_ctrl_mgt_powerdown}};
static const functionsAny_t fofbCtrlSetGetMgtLoopbackFunc             = {functionsUInt32_t{"FOFB_CC", halcs_set_fofb_ctrl_mgt_loopback,
                                                                          halcs_get_fofb_ctrl_mgt_loopback}};
static const functionsAny_t fofbCtrlSetGetTimeFrameDlyFunc            = {functionsUInt32_t{"FOFB_CC", halcs_set_fofb_ctrl_time_frame_dly,
                                                                          halcs_get_fofb_ctrl_time_frame_dly}};
static const functionsAny_t fofbCtrlSetGetGoldenOrbXFunc              = {functionsUInt32_t{"FOFB_CC", halcs_set_fofb_ctrl_golden_orb_x,
                                                                          halcs_get_fofb_ctrl_golden_orb_x}};
static const functionsAny_t fofbCtrlSetGetGoldenOrbYFunc              = {functionsUInt32_t{"FOFB_CC", halcs_set_fofb_ctrl_golden_orb_y,
                                                                          halcs_get_fofb_ctrl_golden_orb_y}};
static const functionsAny_t fofbCtrlSetGetCustFeatureFunc             = {functionsUInt32_t{"FOFB_CC", halcs_set_fofb_ctrl_cust_feature,
                                                                          halcs_get_fofb_ctrl_cust_feature}};
static const functionsAny_t fofbCtrlSetGetRxPolarityFunc              = {functionsUInt32_t{"FOFB_CC", halcs_set_fofb_ctrl_rx_polarity,
                                                                          halcs_get_fofb_ctrl_rx_polarity}};
static const functionsAny_t fofbCtrlSetGetPayloadselFunc              = {functionsUInt32_t{"FOFB_CC", halcs_set_fofb_ctrl_payloadsel,
                                                                          halcs_get_fofb_ctrl_payloadsel}};
static const functionsAny_t fofbCtrlSetGetFofbdataselFunc             = {functionsUInt32_t{"FOFB_CC", halcs_set_fofb_ctrl_fofbdatasel,
                                                                          halcs_get_fofb_ctrl_fofbdatasel}};
static const functionsAny_t fofbCtrlSetGetFirmwareVerFunc             = {functionsUInt32_t{"FOFB_CC", NULL,
                                                                          halcs_get_fofb_ctrl_firmware_ver}};
static const functionsAny_t fofbCtrlSetGetSysStatusFunc               = {functionsUInt32_t{"FOFB_CC", NULL,
                                                                          halcs_get_fofb_ctrl_sys_status}};
static const functionsAny_t fofbCtrlSetGetLinkPartnerFunc             = {functionsUInt32Chan_t{"FOFB_CC", NULL,
                                                                          halcs_get_fofb_ctrl_link_partner}};
static const functionsAny_t fofbCtrlSetGetLinkUpFunc                  = {functionsUInt32_t{"FOFB_CC", NULL,
                                                                          halcs_get_fofb_ctrl_link_up}};
static const functionsAny_t fofbCtrlSetGetTimeFrameCountFunc          = {functionsUInt32_t{"FOFB_CC", NULL,
                                                                          halcs_get_fofb_ctrl_time_frame_count}};
static const functionsAny_t fofbCtrlSetGetHardErrCntFunc              = {functionsUInt32Chan_t{"FOFB_CC", NULL,
                                                                          halcs_get_fofb_ctrl_hard_err_cnt}};
static const functionsAny_t fofbCtrlSetGetSoftErrCntFunc              = {functionsUInt32Chan_t{"FOFB_CC", NULL,
                                                                          halcs_get_fofb_ctrl_soft_err_cnt}};
static const functionsAny_t fofbCtrlSetGetFrameErrCntFunc             = {functionsUInt32Chan_t{"FOFB_CC", NULL,
                                                                          halcs_get_fofb_ctrl_frame_err_cnt}};
static const functionsAny_t fofbCtrlSetGetRxPckCntFunc                = {functionsUInt32Chan_t{"FOFB_CC", NULL,
                                                                          halcs_get_fofb_ctrl_rx_pck_cnt}};
static const functionsAny_t fofbCtrlSetGetTxPckCntFunc                = {functionsUInt32Chan_t{"FOFB_CC", NULL,
                                                                          halcs_get_fofb_ctrl_tx_pck_cnt}};
static const functionsAny_t fofbCtrlSetGetFodProcessTimeFunc          = {functionsUInt32_t{"FOFB_CC", NULL,
                                                                          halcs_get_fofb_ctrl_fod_process_time}};
static const functionsAny_t fofbCtrlSetGetBpmCntFunc                  = {functionsUInt32_t{"FOFB_CC", NULL,
                                                                          halcs_get_fofb_ctrl_bpm_count}};
static const functionsAny_t fofbCtrlSetGetBpmIdRdbackFunc             = {functionsUInt32_t{"FOFB_CC", NULL,
                                                                          halcs_get_fofb_ctrl_bpm_id_rdback}};
static const functionsAny_t fofbCtrlSetGetTfLengthRdbackFunc          = {functionsUInt32_t{"FOFB_CC", NULL,
                                                                          halcs_get_fofb_ctrl_tf_length_rdback}};
static const functionsAny_t fofbCtrlSetGetPowerdownRdbackFunc         = {functionsUInt32_t{"FOFB_CC", NULL,
                                                                          halcs_get_fofb_ctrl_powerdown_rdback}};
static const functionsAny_t fofbCtrlSetGetLoopbackRdbackFunc          = {functionsUInt32_t{"FOFB_CC", NULL,
                                                                          halcs_get_fofb_ctrl_loopback_rdback}};
static const functionsAny_t fofbCtrlSetGetFaivalRdbackFunc            = {functionsUInt32_t{"FOFB_CC", NULL,
                                                                          halcs_get_fofb_ctrl_faival_rdback}};
static const functionsAny_t fofbCtrlSetGetToaRdEnFunc                 = {functionsUInt32_t{"FOFB_CC", halcs_set_fofb_ctrl_toa_rd_en,
                                                                          halcs_get_fofb_ctrl_toa_rd_en}};
static const functionsAny_t fofbCtrlSetGetToaRdStrFunc                = {functionsUInt32_t{"FOFB_CC", halcs_set_fofb_ctrl_toa_rd_str,
                                                                          halcs_get_fofb_ctrl_toa_rd_str}};
static const functionsAny_t fofbCtrlSetGetToaDataFunc                 = {functionsUInt32_t{"FOFB_CC", NULL,
                                                                          halcs_get_fofb_ctrl_toa_data}};
static const functionsAny_t fofbCtrlSetGetRcbRdEnFunc                 = {functionsUInt32_t{"FOFB_CC", halcs_set_fofb_ctrl_rcb_rd_en,
                                                                          halcs_get_fofb_ctrl_rcb_rd_en}};
static const functionsAny_t fofbCtrlSetGetRcbRdStrFunc                = {functionsUInt32_t{"FOFB_CC", halcs_set_fofb_ctrl_rcb_rd_str,
                                                                          halcs_get_fofb_ctrl_rcb_rd_str}};
static const functionsAny_t fofbCtrlSetGetRcbDataFunc                 = {functionsUInt32_t{"FOFB_CC", NULL,
                                                                          halcs_get_fofb_ctrl_rcb_data}};

static const char *driverName="drvFOFB";
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

    chan = addr;

    *chanArg = chan;
    return status;
}

asynStatus drvFOFB::getServiceID (int fofbNumber, int addr, const char *serviceName,
        int *serviceIDArg) const
{
    static const char *functionName = "getServiceID";
    asynStatus status = asynSuccess;

    *serviceIDArg = boardMap[fofbNumber].fofb;

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
 *  *  * Calls constructor for the asynPortDriver base class.
 *   *   * \param[in] portName The name of the asyn port driver to be created.
 *    *    * \param[in] endpoint The device address string ]
 *     *     * */
drvFOFB::drvFOFB(const char *portName, const char *endpoint, int fofbNumber,
        int verbose, int timeout)
   : asynPortDriver(portName,
                    MAX_ADDR, /* maxAddr */
                    asynUInt32DigitalMask | asynFloat64Mask  | asynDrvUserMask,    /* Interface mask     */
                    asynUInt32DigitalMask | asynFloat64Mask ,                      /* Interrupt mask     */
                    ASYN_CANBLOCK | ASYN_MULTIDEVICE, /* asynFlags.  This driver blocks it is multi-device */
                    1, /* Autoconnect */
                    0, /* Default priority */
                    0) /* Default stack size*/
{
    asynStatus status;
    const char *functionName = "drvFOFB";

    /* Create portName so we can create a new AsynUser later */
    fofbPortName = epicsStrDup(portName);

    this->endpoint = strdup(endpoint);
    if (this->endpoint == NULL) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s fofb failure to copy endpoint\n",
                driverName, functionName);
        status = asynError;
        goto endpoint_dup_err;
    }

    if (fofbNumber < FOFB_NUMBER_MIN || fofbNumber > FOFB_NUMBER_MAX) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s drvfofb invalid fofbNumber\n",
                driverName, functionName);
        status = asynError;
        goto invalid_fofb_number_err;
    }

    this->fofbNumber = fofbNumber;
    this->verbose = verbose;
    this->timeout = timeout;

    /* Create parameters for all addresses without specifying the ones that don't
 *  *      * make sense to be on a specified list. Without this we woudl have to create
 *   *           * different parameterIndex structures to store each index, as they could be
 *    *                * differently if created in just a few lists */

    /* CAUTION. The order of craetion must be the same as defined in .h file.
 *  *      * Otherwise, checking for FIRST_PARAM/LAST_PARAM won't work */

    /* Create general parameters */
    createParam(P_FofbProcessingRamWriteString,      asynParamUInt32Digital,        &P_FofbProcessingRamWrite);
    createParam(P_FofbProcessingRamAddrString,       asynParamUInt32Digital,        &P_FofbProcessingRamAddr);
    createParam(P_FofbProcessingRamDataInString,     asynParamUInt32Digital,        &P_FofbProcessingRamDataIn);
    createParam(P_FofbProcessingRamDataOutString,    asynParamUInt32Digital,        &P_FofbProcessingRamDataOut);
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

    /* FOFB HW Int32 Functions mapping. Functions not mapped here are just written
 *  *      * to the parameter library */
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

    lock();
    status = fofbClientConnect(this->pasynUserSelf);
    unlock();

    /* If we correct connect for this first time, liclient
 *  *      * will ensure the reconnection to server if necessary, but we
 *   *           * must succeed here or we must abort completely */
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

    /* Do callbacks so higher layers see any changes. Call callbacks for every addr */
    for (int i = 0; i < MAX_ADDR; ++i) {
        callParamCallbacks(i);
    }

    epicsAtExit(exitHandlerC, this);

invalid_fofb_number_err:
    free (this->endpoint);
endpoint_dup_err:
    return;
}

/** Destructor for the drvFOFB class.
 *  *  */
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

    asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
        "%s:%s: FOFB Processing client connected\n",
        driverName, functionName);

    pasynManager->exceptionConnect(pasynUser);

    return status;

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

    pasynManager->exceptionDisconnect(pasynUser);
    return status;
}

/********************************************************************/
/********************* Asyn overrided methods  **********************/
/********************************************************************/

/*
 *  *  * Asyn overrided methods that are called by higher layers
 *   *   */

/** Called when asyn clients call pasynUInt32Digital->write().
 *  *  * For all parameters it sets the value in the parameter library and calls any registered callbacks..
 *   *   * \param[in] pasynUser pasynUser structure that encodes the reason and address.
 *    *    * \param[in] value Value to write. */
asynStatus drvFOFB::writeUInt32Digital(asynUser *pasynUser, epicsUInt32 value,
        epicsUInt32 mask)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    int addr = 0;
    const char *paramName;
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
        /* Fetch the parameter string name for possible use in debugging */
        getParamName(function, &paramName);

        /* Do operation on HW. Some functions do not set anything on hardware */
        status = setParam32(function, mask, addr);
    }
    else {
        /* Call base class */
        status = asynPortDriver::writeUInt32Digital(pasynUser, value, mask);
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
 *  *  * For all parameters it gets the value in the parameter library..
 *   *   * \param[in] pasynUser pasynUser structure that encodes the reason and address.
 *    *    * \param[out] value Value to read. */
asynStatus drvFOFB::readUInt32Digital(asynUser *pasynUser, epicsUInt32 *value,
        epicsUInt32 mask)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    int addr = 0;
    const char *functionName = "readUInt32Digital";
    const char *paramName;

    /* Get channel for possible use */
    status = getAddress(pasynUser, &addr);
    if (status) {
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                "%s:%s: status=%d, function=%d, name=%s",
                driverName, functionName, status, function, paramName);
        return status;
    }
    /* Fetch the parameter string name for possible use in debugging */
    getParamName(function, &paramName);

    if (function >= FIRST_COMMAND) {
        /* Get parameter, possibly from HW */
        status = getParam32(function, value, mask, addr);
    }
    else {
        /* Call base class */
        status = asynPortDriver::readUInt32Digital(pasynUser, value, mask);
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
    int serviceID = 0;
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
    int serviceID = 0;
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

/********************************************************************/
/********* Generic 32-bit/Double FOFB Processing Operations *********/
/********************************************************************/

/*
 *  * * 32-bit/Double generic FOFB Processing operations. These will map to real
 *   * * functions defined in the structures. e.g., functionsInt32_t
 *    * * and functionsFloat64_t
 *     * */

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
        case asynParamUInt32Digital:
            status = setParam32(functionId, 0xFFFFFFFF, addr);
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
 *  *      * the parameter from the list */
    else if (status == asynDisabled){
        status = asynSuccess;
    }

get_param_err:
    return (asynStatus)status;
}

/********************************************************************/
/*********************** Misc BPM Operations ************************/
/********************************************************************/

/*
 *  *  * Miscellaneous functions that don't map easily
 *   *   * to our generic handlers get/setParam[32/Double]
 *    *    */


/* Configuration routine.  Called directly, or from the iocsh function below */
extern "C" {

    /** EPICS iocsh callable function to call constructor for the drvFOFB class.
 *  *      * \param[in] portName The name of the asyn port driver to be created.
 *   *           * \param[in] endpoint The address device string */
    int drvFOFBConfigure(const char *portName, const char *endpoint,
            int fofbNumber, int verbose, int timeout)
    {
        new drvFOFB(portName, endpoint, fofbNumber, verbose, timeout);
        return(asynSuccess);
    }

    /* EPICS iocsh shell commands */
    static const iocshArg initArg0 = { "portName", iocshArgString};
    static const iocshArg initArg1 = { "endpoint", iocshArgString};
    static const iocshArg initArg2 = { "fofbNumber", iocshArgInt};
    static const iocshArg initArg3 = { "verbose", iocshArgInt};
    static const iocshArg initArg4 = { "timeout", iocshArgInt};
    static const iocshArg * const initArgs[] = {&initArg0,
        &initArg1,
        &initArg2,
        &initArg3,
        &initArg4};
    static const iocshFuncDef initFuncDef = {"drvFOFBConfigure",5,initArgs};
    static void initCallFunc(const iocshArgBuf *args)
    {
        drvFOFBConfigure(args[0].sval, args[1].sval, args[2].ival,
                args[3].ival, args[4].ival);
    }

    void drvFOFBRegister(void)
    {
        iocshRegister(&initFuncDef,initCallFunc);
    }

    epicsExportRegistrar(drvFOFBRegister);
}
