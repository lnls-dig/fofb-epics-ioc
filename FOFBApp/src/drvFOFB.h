/*
 * drvFOFB.h
 *
 * Authors: Melissa Aguiar
 *
 * Created Dec. 03, 2021
 */

#include "asynNDArrayDriver.h"
#include <epicsExit.h>
#include <NDArray.h>
#include <epicsMutex.h>
#include <epicsRingBytes.h>
/* Third-party libraries */
#include <unordered_map>
#include <halcs_client.h>
#include <acq_client.h>

/* any implementation for non c++-17 compilers */
#include "any.hpp"

using linb::any;
using linb::any_cast;
using linb::bad_any_cast;

#define ARRAY_SIZE(ARRAY)             (sizeof(ARRAY)/sizeof((ARRAY)[0]))
#define MAX_ARRAY_POINTS              200000
#define FOFB_TIMEOUT                  1.0

typedef enum {
    FOFBIDReg = 0,
    FOFBIDEnd,
} fofb_coreID_types;

#define NUM_ACQ_CORES_PER_FOFB        FOFBIDEnd /* Regular Acquisition core and Post-Mortem */
#define NUM_TRIG_CORES_PER_FOFB       NUM_ACQ_CORES_PER_FOFB /* Trigger core for regular Acquisition and Post-Mortem */

/* FOFB acquisition status */
typedef enum {
    FOFBStatusIdle = 0,
    FOFBStatusWaiting,
    FOFBStatusTriggerHwExtWaiting,
    FOFBStatusTriggerHwDataWaiting,
    FOFBStatusTriggerSwWaiting,
    FOFBStatusAcquire,
    FOFBStatusErrAcq,
    FOFBStatusAborted,
    FOFBStatusErrTooManyPoints,
    FOFBStatusErrTooFewPoints,
    FOFBStatusErrNoMem,
    FOFBStatusErrAcqOFlow,
} fofb_status_types;

/* Waveform IDs */
typedef enum {
    WVF_DATA_CH0 = 0,
    WVF_DATA_CH1,
    WVF_DATA_CH2,
    WVF_DATA_CH3,
    WVF_DATA_CH4,
    WVF_DATA_CH5,
    WVF_DATA_CH6,
    WVF_DATA_CH7,
    WVF_DATA_CH8,
    WVF_DATA_CH9,
    WVF_DATA_CH10,
    WVF_DATA_CH11,
    WVF_DATA_CH12,
    WVF_DATA_CH13,
    WVF_DATA_CH14,
    WVF_DATA_CH15,
    WVF_DATA_ALL,
    WVF_MONIT_CH0,
    WVF_END
} wvf_types;

/* FIXME: This number must be at least the number of triggers
 * available on the FPGA. Although this is used to alloc the number
 * of waveforms, it's not used by getAddress () by the NDArray plugins,
 * as this function returns the address that is declared on plugin startup
 * (NDStdArraysConfigure function, NDArrayAddr). So, we are free to use all
 * of the addresses that are set by the database.
 * In summary, we use the different addresses to call different trigger channel
 * functions */
#define MAX_WAVEFORMS               WVF_END
/* FIXME FIXME: This should be read from HW. Also, this is actually less than 24,
 * but we let space for extra room */
#define MAX_TRIGGERS                24
/* This is needed so we have EPICS Asyn addresses sufficient for all of the
 * Triggers, from either ACQ core */
#define MAX_TRIGGERS_ALL_ACQ        (NUM_ACQ_CORES_PER_FOFB*MAX_TRIGGERS)
/* Get the greater between them */
#define MAX_ADDR                    MAX(MAX_WAVEFORMS,MAX_TRIGGERS_ALL_ACQ)
/* Number of Monitoring waveforms */
#define MAX_MONIT_DATA              10

/* Channel IDs */
typedef enum {
    CH_ADC = 0,
    CH_TBT = 1,
    CH_FOFB = 2,
    CH_MONIT1 = 3,
    CH_END
} ch_types;

#define MAX_CHANNELS                  CH_END

typedef enum {
    CH_HW_ADC = 0,
    CH_HW_TBT = 6,
    CH_HW_FOFB = 11,
    CH_HW_MONIT1 = 14,
    CH_HW_END
} ch_hw_types;

#define MAX_HW_CHANNELS               CH_HW_END

/* Waveform DATA types IDs */
typedef enum {
    WVF_CH0 = 0,
    WVF_CH1,
    WVF_CH2,
    WVF_CH3,
    WVF_CH4,
    WVF_CH5,
    WVF_CH6,
    WVF_CH7,
    WVF_CH8,
    WVF_CH9,
    WVF_CH10,
    WVF_CH11,
    WVF_CH12,
    WVF_CH13,
    WVF_CH14,
    WVF_CH15,
    WVF_ALL,
    WVF_DATA_END
} wvf_data_types;

#define MAX_WVF_DATA_SINGLE          (WVF_CH0+1)
#define MAX_WVF_DATA_TYPES           WVF_DATA_END

/* One dimension for each point */
#define MAX_WVF_DIMS                  2

#define MAX_SLOTS                     12
#define MAX_FOFB_PER_SLOT             2
#define MAX_FOFBS                     (MAX_SLOTS*MAX_FOFB_PER_SLOT)

#define FOFB_NUMBER_MIN               1
#define FOFB_NUMBER_MAX               MAX_FOFBS

/* FOFB Channel structure */
typedef struct {
    /* HW channel mapping. -1 means not available */
    int HwDataChannel;
    /* NDArray addresses mapping */
    int NDArrayData[NUM_ACQ_CORES_PER_FOFB][MAX_WVF_DATA_TYPES];
} channelMap_t;

/* FOFB Reverse channel mapping structure */
typedef struct {
    /* EPICS channel. -1 means not available */
    int epicsChannel;
} channelRevMap_t;

/* FOFB Acq Channel properties structure */
typedef struct {
    epicsUInt32 sampleSize;
    epicsUInt32 numAtoms;
    epicsUInt32 atomWidth;
} channelProp_t;

/* Write 32-bit function pointer */
typedef halcs_client_err_e (*writeInt32Fp)(halcs_client_t *self, char *service,
	uint32_t param);
/* Read 32-bit function pointer */
typedef halcs_client_err_e (*readInt32Fp)(halcs_client_t *self, char *service,
	uint32_t *param);

/* FOFB command dispatch table */
typedef struct {
    const char *serviceName;
    writeInt32Fp write;
    readInt32Fp read;
} functionsInt32_t;

/* Write 32-bit function pointer */
typedef halcs_client_err_e (*writeUInt32Fp)(halcs_client_t *self, char *service,
    uint32_t param);
/* Read 32-bit function pointer */
typedef halcs_client_err_e (*readUInt32Fp)(halcs_client_t *self, char *service,
    uint32_t *param);

/* FOFB command dispatch table */
typedef struct {
    const char *serviceName;
    writeUInt32Fp write;
    readUInt32Fp read;
} functionsUInt32_t;

/* Write 32-bit function pointer with channel selection */
typedef halcs_client_err_e (*writeUInt32ChanFp)(halcs_client_t *self, char *service,
    uint32_t chan, uint32_t param);
/* Read 32-bit function pointer with channel selection */
typedef halcs_client_err_e (*readUInt32ChanFp)(halcs_client_t *self, char *service,
    uint32_t chan, uint32_t *param);

/* FOFB command dispatch table */
typedef struct {
    const char *serviceName;
    writeUInt32ChanFp write;
    readUInt32ChanFp read;
} functionsUInt32Chan_t;

/* Write 32-bit function pointer with acq_client structure */
typedef halcs_client_err_e (*writeUInt32AcqFp)(acq_client_t *self, char *service,
    uint32_t param);
/* Read 32-bit function pointer with acq_client structure */
typedef halcs_client_err_e (*readUInt32AcqFp)(acq_client_t *self, char *service,
    uint32_t *param);

/* FOFB command dispatch table */
typedef struct {
    const char *serviceName;
    writeUInt32AcqFp write;
    readUInt32AcqFp read;
} functionsUInt32Acq_t;

/* Write 2 32-bit function pointer */
typedef halcs_client_err_e (*write2UInt32Fp)(halcs_client_t *self, char *service,
    uint32_t param1, uint32_t param2);
/* Read 32-bit function pointer */
typedef halcs_client_err_e (*read2UInt32Fp)(halcs_client_t *self, char *service,
    uint32_t *param1, uint32_t *param2);

/* FOFB command dispatch table */
typedef struct {
    const char *serviceName;
    write2UInt32Fp write;
    read2UInt32Fp read;
    /* Which parameter (first or second) would trigger this function to be
 *  *  *      * executed on hardware (the other one won't be changed) */
    int parameterPos;
} functions2UInt32_t;

/* Write 64-bit float function pointer */
typedef halcs_client_err_e (*writeFloat64Fp)(halcs_client_t *self, char *service,
    double param);
/* Read 32-bit function pointer */
typedef halcs_client_err_e (*readFloat64Fp)(halcs_client_t *self, char *service,
    double *param);

/* FOFB command dispatch table */
typedef struct {
    const char *serviceName;
    writeFloat64Fp write;
    readFloat64Fp read;
} functionsFloat64_t;

typedef struct {
    union {
        epicsInt32 argInt32;
        epicsUInt32 argUInt32;
        epicsFloat64 argFloat64;
    };
} functionsArgs_t;

/* Forward declaration as struct functionsAny_t needs it */
class drvFOFB;

/* Generic Function Structure for "any" function pointer */
struct functionsAny_t {
    template<typename T>
        functionsAny_t(T const& functionFp) :
            _functionFp(functionFp),
            _executeHwReadFunction(&functionsAny_t::executeHwReadFunction<T>),
            _executeHwWriteFunction(&functionsAny_t::executeHwWriteFunction<T>),
            _getServiceNameFromFunc(&functionsAny_t::getServiceNameFromFunc<T>) {}

    asynStatus executeHwRead(const drvFOFB& drvFOFB, char *service,
        int addr, functionsArgs_t &functionParam)
    {
        return (this->*_executeHwReadFunction)(drvFOFB, _functionFp,
                service, addr, functionParam);
    }

    asynStatus executeHwWrite(const drvFOFB& drvFOFB, char *service,
        int addr, functionsArgs_t &functionParam)
    {
        return (this->*_executeHwWriteFunction)(drvFOFB, _functionFp,
                service, addr, functionParam);
    }

    const char *getServiceName(const drvFOFB& drvFOFB)
    {
        return (this->*_getServiceNameFromFunc)(drvFOFB, _functionFp);
    }

private:
    any _functionFp;

    /* Read template function for Hw execution */
    typedef asynStatus (functionsAny_t::*executeHwReadFunctionFp)
        (const drvFOFB& drvFOFB, const any& functionFp,
         char *service, int addr, functionsArgs_t &functionParam);
    executeHwReadFunctionFp _executeHwReadFunction;

    /* Write template function for Hw execution */
    typedef asynStatus (functionsAny_t::*executeHwWriteFunctionFp)
        (const drvFOFB& drvFOFB, const any& functionFp,
         char *service, int addr, functionsArgs_t &functionParam);
    executeHwWriteFunctionFp _executeHwWriteFunction;

    /* Service name utilities */
    typedef const char * (functionsAny_t::*getServiceNameFromFuncFp)
        (const drvFOFB& drvFOFB, const any& functionFp) const;
    getServiceNameFromFuncFp _getServiceNameFromFunc;

    /* Read function for Hw execution */
    template<typename T>
    asynStatus executeHwReadFunction(const drvFOFB& drvFOFB,
            const any& functionFp, char *service, int addr,
            functionsArgs_t &functionParam);

    /* Write function for Hw execution */
    template<typename T>
    asynStatus executeHwWriteFunction(const drvFOFB& drvFOFB,
            const any& functionFp, char *service, int addr,
            functionsArgs_t &functionParam);

    /* Service name utilities */
    template<typename T>
    const char *getServiceNameFromFunc(const drvFOFB& drvFOFB,
            const any& functionFp) const;
};

/* These are the drvInfo strings that are used to identify the parameters.
 * They are used by asyn clients, including standard asyn device support */
#define P_RtmLampStatusString                   "RTMLAMP_OHWR_STA"                          /* asynUInt32Digital,      r/o */
#define P_RtmLampDacDataFromWbString            "RTMLAMP_OHWR_CTL_DAC_DATA_FROM_WB"         /* asynUInt32Digital,      r/w */
#define P_RtmLampAmpIFlagLString                "RTMLAMP_OHWR_CH_0_STA_AMP_IFLAG_L"         /* asynUInt32Digital,      r/o */
#define P_RtmLampAmpTFlagLString                "RTMLAMP_OHWR_CH_0_STA_AMP_TFLAG_L"         /* asynUInt32Digital,      r/o */
#define P_RtmLampAmpIFlagRString                "RTMLAMP_OHWR_CH_0_STA_AMP_IFLAG_R"         /* asynUInt32Digital,      r/o */
#define P_RtmLampAmpTFlagRString                "RTMLAMP_OHWR_CH_0_STA_AMP_TFLAG_R"         /* asynUInt32Digital,      r/o */
#define P_RtmLampAmpEnString                    "RTMLAMP_OHWR_CH_0_CTL_AMP_EN"              /* asynUInt32Digital,      r/w */
#define P_RtmLampPIOLTriangEnString             "RTMLAMP_OHWR_CH_0_CTL_PI_OL_TRIANG_ENABLE" /* asynUInt32Digital,      r/w */
#define P_RtmLampPIOLSquareEnString             "RTMLAMP_OHWR_CH_0_CTL_PI_OL_SQUARE_ENABLE" /* asynUInt32Digital,      r/w */
#define P_RtmLampPISPSquareEnString             "RTMLAMP_OHWR_CH_0_CTL_PI_SP_SQUARE_ENABLE" /* asynUInt32Digital,      r/w */
#define P_RtmLampPIEnString                     "RTMLAMP_OHWR_CH_0_CTL_PI_ENABLE"           /* asynUInt32Digital,      r/w */
#define P_RtmLampDacDataString                  "RTMLAMP_OHWR_CH_0_DAC_DATA"                /* asynUInt32Digital,      r/w */
#define P_RtmLampDacWrString                    "RTMLAMP_OHWR_CH_0_DAC_WR"                  /* asynUInt32Digital,      r/w */
#define P_RtmLampPIKPString                     "RTMLAMP_OHWR_CH_0_PI_KP_DATA"              /* asynUInt32Digital,      r/w */
#define P_RtmLampPITIString                     "RTMLAMP_OHWR_CH_0_PI_TI_DATA"              /* asynUInt32Digital,      r/w */
#define P_RtmLampPISPString                     "RTMLAMP_OHWR_CH_0_PI_SP_DATA"              /* asynUInt32Digital,      r/w */
#define P_RtmLampPIOLDacCntMaxString            "RTMLAMP_OHWR_PI_OL_DAC_CNT_MAX_DATA"       /* asynUInt32Digital,      r/w */
#define P_RtmLampPISPLimInfString               "RTMLAMP_OHWR_PI_SP_LIM_INF_DATA"           /* asynUInt32Digital,      r/w */
#define P_AdcRateString                         "INFO_ADCRATE"                              /* asynUInt32Digital,      r/o */
#define P_TbtRateString                         "INFO_TBTRATE"                              /* asynUInt32Digital,      r/o */
#define P_FofbRateString                        "INFO_FOFBRATE"                             /* asynUInt32Digital,      r/o */
#define P_MonitRateString                       "INFO_MONITRATE"                            /* asynUInt32Digital,      r/o */
#define P_Monit1RateString                      "INFO_MONIT1RATE"                           /* asynUInt32Digital,      r/o */
#define P_MonitUpdtString                       "MONIT_UPDT"                                /* asynUInt32Digital,      r/w */
#define P_MonitPollTimeString                   "MONIT_POLL_TIME"                           /* asynUInt32Digital,      r/w */
#define P_MonitEnableString                     "MONIT_ENABLE"                              /* asynInt32,              r/w */
#define P_FOFBStatusString                      "ACQ_STATUS"                                /* asynInt32,              r/o */
#define P_SamplesPreString                      "ACQ_SAMPLES_PRE"                           /* asynUInt32Digital,      r/w */
#define P_SamplesPostString                     "ACQ_SAMPLES_POST"                          /* asynUInt32Digital,      r/w */
#define P_NumShotsString                        "ACQ_NUM_SHOTS"                             /* asynUInt32Digital,      r/w */
#define P_ChannelString                         "ACQ_CHANNEL"                               /* asynInt32,              r/w */
#define P_TriggerString                         "ACQ_TRIGGER"                               /* asynUInt32Digital,      r/w */
#define P_TriggerEventString                    "ACQ_TRIGGER_EVENT"                         /* asynUInt32Digital,      r/w */
#define P_TriggerRepString                      "ACQ_TRIGGER_REP"                           /* asynUInt32Digital,      r/w */
#define P_UpdateTimeString                      "ACQ_UPDATE_TIME"                           /* asynFloat64,            r/w */
#define P_TriggerDataThresString                "ACQ_TRIGGER_THRES"                         /* asynInt32,              r/w */
#define P_TriggerDataPolString                  "ACQ_TRIGGER_POL"                           /* asynInt32,              r/w */
#define P_TriggerDataSelString                  "ACQ_TRIGGER_SEL"                           /* asynInt32,              r/w */
#define P_TriggerDataFiltString                 "ACQ_TRIGGER_FILT"                          /* asynInt32,              r/w */
#define P_TriggerHwDlyString                    "ACQ_TRIGGER_HWDLY"                         /* asynInt32,              r/w */
#define P_DataTrigChanString                    "ACQ_DATA_TRIG_CHAN"                        /* asynuint32digital,      r/w */
#define P_ChannelSampleSizeString               "ACQ_CH_SAMPLE_SIZE"                        /* asynUInt32Digital,      r/o */
#define P_ChannelNumAtomsString                 "ACQ_CH_NUM_ATOMS"                          /* asynUInt32Digital,      r/o */
#define P_ChannelAtomWidthString                "ACQ_CH_ATOM_WIDTH"                         /* asynUInt32Digital,      r/o */
#define P_TriggerChanString                     "TRIGGER_CHAN"                              /* asynUInt32Digital,      r/w */
#define P_TriggerDirString                      "TRIGGER_DIR"                               /* asynUInt32Digital,      r/w */
#define P_TriggerDirPolString                   "TRIGGER_DIR_POL"                           /* asynUInt32Digital,      r/w */
#define P_TriggerRcvCntRstString                "TRIGGER_RCV_CNT_RST"                       /* asynUInt32Digital,      r/w */
#define P_TriggerTrnCntRstString                "TRIGGER_TRN_CNT_RST"                       /* asynUInt32Digital,      r/w */
#define P_TriggerRcvLenString                   "TRIGGER_RCV_LEN"                           /* asynUInt32Digital,      r/w */
#define P_TriggerTrnLenString                   "TRIGGER_TRN_LEN"                           /* asynUInt32Digital,      r/w */
#define P_TriggerCntRcvString                   "TRIGGER_CNT_RCV"                           /* asynUInt32Digital,      r/w */
#define P_TriggerCntTrnString                   "TRIGGER_CNT_TRN"                           /* asynUInt32Digital,      r/w */
#define P_TriggerRcvSrcString                   "TRIGGER_RCV_SRC"                           /* asynUInt32Digital,      r/w */
#define P_TriggerTrnSrcString                   "TRIGGER_TRN_SRC"                           /* asynUInt32Digital,      r/w */
#define P_TriggerRcvInSelString                 "TRIGGER_RCV_IN_SEL"                        /* asynUInt32Digital,      r/w */
#define P_TriggerTrnOutSelString                "TRIGGER_TRN_OUT_SEL"                       /* asynUInt32Digital,      r/w */

typedef enum {
    TRIG_ACQ_START,
    TRIG_ACQ_STOP,
    TRIG_ACQ_ABORT,
} trigEvent_e;

class drvFOFB : public asynNDArrayDriver {
    public:
        drvFOFB(const char *portName, const char *endpoint, int fofbNumber,
                const char *type, int verbose, int timeout,
                int maxPoints, int maxBuffers, size_t maxMemory);
        ~drvFOFB();

        /* These are the methods that we override from asynPortDriver */
        virtual asynStatus writeUInt32Digital(asynUser *pasynUser, epicsUInt32 value, epicsUInt32 mask);
        virtual asynStatus readUInt32Digital(asynUser *pasynUser, epicsUInt32 *value, epicsUInt32 mask);
        virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
        virtual asynStatus readInt32(asynUser *pasynUser, epicsInt32 *value);
        virtual asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value);
        virtual asynStatus readFloat64(asynUser *pasynUser, epicsFloat64 *value);

        /* These methods are overwritten from asynPortDriver */
        virtual asynStatus connect(asynUser* pasynUser);
        virtual asynStatus disconnect(asynUser* pasynUser);

        /* These are the methods that are new to this class */
        void acqTask(int coreID, double pollTime, bool autoStart);
        void acqMonitTask();

        /* Overloaded functions for extracting service name*/
        const char *doGetServiceNameFromFunc (functionsInt32_t &func) const
        {
            return func.serviceName;
        }

        const char *doGetServiceNameFromFunc (functionsUInt32_t &func) const
        {
            return func.serviceName;
        }

        const char *doGetServiceNameFromFunc (functionsUInt32Chan_t &func) const
        {
            return func.serviceName;
        }

        const char *doGetServiceNameFromFunc (functionsUInt32Acq_t &func) const
        {
            return func.serviceName;
        }

        const char *doGetServiceNameFromFunc (functions2UInt32_t &func) const
        {
            return func.serviceName;
        }

        const char *doGetServiceNameFromFunc (functionsFloat64_t &func) const
        {
            return func.serviceName;
        }

        /* Overloaded function mappings called by functionsAny_t */
        asynStatus doExecuteHwWriteFunction(functionsInt32_t &func, char *service,
                int addr, functionsArgs_t &functionParam) const;
        asynStatus doExecuteHwWriteFunction(functionsUInt32_t &func, char *service,
                int addr, functionsArgs_t &functionParam) const;
        asynStatus executeHwWriteFunction(int functionId, int addr,
                functionsArgs_t &functionParam);
        asynStatus doExecuteHwWriteFunction(functionsUInt32Chan_t &func, char *service,
                int addr, functionsArgs_t &functionParam) const;
        asynStatus doExecuteHwWriteFunction(functionsUInt32Acq_t &func, char *service,
                int addr, functionsArgs_t &functionParam) const;
        asynStatus doExecuteHwWriteFunction(functions2UInt32_t &func, char *service,
                int addr, functionsArgs_t &functionParam) const;
        asynStatus doExecuteHwWriteFunction(functionsFloat64_t &func, char *service,
                int addr, functionsArgs_t &functionParam) const;

        asynStatus doExecuteHwReadFunction(functionsInt32_t &func, char *service,
                int addr, functionsArgs_t &functionParam) const;
        asynStatus doExecuteHwReadFunction(functionsUInt32_t &func, char *service,
                int addr, functionsArgs_t &functionParam) const;
        asynStatus executeHwReadFunction(int functionId, int addr,
                functionsArgs_t &functionParam);
        asynStatus doExecuteHwReadFunction(functionsUInt32Chan_t &func, char *service,
                int addr, functionsArgs_t &functionParam) const;
        asynStatus doExecuteHwReadFunction(functionsUInt32Acq_t &func, char *service,
                int addr, functionsArgs_t &functionParam) const;
        asynStatus doExecuteHwReadFunction(functions2UInt32_t &func, char *service,
                int addr, functionsArgs_t &functionParam) const;
        asynStatus doExecuteHwReadFunction(functionsFloat64_t &func, char *service,
                int addr, functionsArgs_t &functionParam) const;

        /* General service name handling utilities */
        asynStatus getServiceChan (int fofbNumber, int addr, const char *serviceName,
                epicsUInt32 *chanArg) const;
        asynStatus getServiceID (int fofbNumber, int addr, const char *serviceName,
                int *serviceIDArg) const;
        asynStatus getFullServiceName (int fofbNumber, int addr, const char *serviceName,
                char *fullServiceName, int fullServiceNameSize) const;

    protected:
        /** Values used for pasynUser->reason, and indexes into the parameter library. */
        int P_RtmLampStatus;
        #define FIRST_COMMAND P_RtmLampStatus
        int P_RtmLampDacDataFromWb;
        int P_RtmLampAmpIFlagL;
        int P_RtmLampAmpTFlagL;
        int P_RtmLampAmpIFlagR;
        int P_RtmLampAmpTFlagR;
        int P_RtmLampAmpEn;
        int P_RtmLampPIOLTriangEn;
        int P_RtmLampPIOLSquareEn;
        int P_RtmLampPISPSquareEn;
        int P_RtmLampPIEn;
        int P_RtmLampDacData;
        int P_RtmLampDacWr;
        int P_RtmLampPIKP;
        int P_RtmLampPITI;
        int P_RtmLampPISP;
        int P_RtmLampPIOLDacCntMax;
        int P_RtmLampPISPLimInf;
        int P_FOFBStatus;
        int P_AdcRate;
        int P_TbtRate;
        int P_FofbRate;
        int P_MonitRate;
        int P_Monit1Rate;
        int P_MonitUpdt;
        int P_MonitPollTime;
        int P_MonitEnable;
        int P_SamplesPre;
        int P_SamplesPost;
        int P_NumShots;
        int P_Channel;
        int P_UpdateTime;
        int P_Trigger;
        int P_TriggerEvent;
        int P_TriggerRep;
        int P_TriggerDataThres;
        int P_TriggerDataPol;
        int P_TriggerDataSel;
        int P_TriggerDataFilt;
        int P_TriggerHwDly;
        int P_DataTrigChan;
        int P_ChannelSampleSize;
        int P_ChannelNumAtoms;
        int P_ChannelAtomWidth;
        int P_TriggerChan;
        int P_TriggerDir;
        int P_TriggerDirPol;
        int P_TriggerRcvCntRst;
        int P_TriggerTrnCntRst;
        int P_TriggerRcvLen;
        int P_TriggerTrnLen;
        int P_TriggerCntRcv;
        int P_TriggerCntTrn;
        int P_TriggerRcvSrc;
        int P_TriggerTrnSrc;
        int P_TriggerRcvInSel;
        int P_TriggerTrnOutSel;
#define LAST_COMMAND P_TriggerTrnOutSel

    private:
        /* Our data */
        halcs_client_t *fofbClient;
        halcs_client_t *fofbClientMonit;
        acq_client_t *fofbClientAcqParam[NUM_ACQ_CORES_PER_FOFB];
        acq_client_t *fofbClientAcq[NUM_ACQ_CORES_PER_FOFB];
        char *endpoint;
        int fofbNumber;
        int fofbMaxPoints;
        int verbose;
        int timeout;
        char *fofbPortName;
        int readingActive[NUM_ACQ_CORES_PER_FOFB];
        int repetitiveTrigger[NUM_ACQ_CORES_PER_FOFB];
        epicsEventId startAcqEventId[NUM_ACQ_CORES_PER_FOFB];
        epicsEventId stopAcqEventId[NUM_ACQ_CORES_PER_FOFB];
        epicsEventId abortAcqEventId[NUM_ACQ_CORES_PER_FOFB];
        epicsEventId reconfSPassAcqEventId[NUM_ACQ_CORES_PER_FOFB];
        epicsEventId activeAcqEventId[NUM_ACQ_CORES_PER_FOFB];
        epicsEventId activeMonitEnableEventId;
        std::unordered_map<int, functionsAny_t> fofbHwFunc;

        /* Our private methods */

        /* Client connection management */
        asynStatus fofbClientConnect(asynUser* pasynUser);
        asynStatus fofbClientDisconnect(asynUser* pasynUser);

        /* Acquisition functions */
        asynStatus setAcqEvent(epicsUInt32 mask, int addr);
        asynStatus getAcqNDArrayType(int coreID, int channel, epicsUInt32 atomWidth, NDDataType_t *NDType);
        asynStatus getChannelProperties(int coreID, int channel, channelProp_t *channelProp);
        fofb_status_types getFOFBInitAcqStatus(int coreID);
        asynStatus startAcq(int coreID, int hwChannel, epicsUInt32 num_samples_pre,
                epicsUInt32 num_samples_post, epicsUInt32 num_shots);
        asynStatus setAcqTrig(int coreID, acq_client_trig_e trig);
        asynStatus initAcqPM(int coreID);
        asynStatus abortAcqRaw(int coreID, acq_client_t *acq_client);
        asynStatus abortAcq(int coreID);
        asynStatus abortAcqFromPortThread(int coreID);
        asynStatus abortAcqTask(int addr, bool abortAcqHw = false);
        asynStatus stopAcqTask(int addr);
        int checkAcqCompletion(int coreID);
        asynStatus getAcqCurve(int coreID, NDArray *pArrayAllChannels, int hwChannel,
                epicsUInt32 num_samples_pre, epicsUInt32 num_samples_post,
                epicsUInt32 num_shots);
        asynStatus deinterleaveNDArray (NDArray *pArrayAllChannels, const int *pNDArrayAddr,
                int pNDArrayAddrSize, int arrayCounter, epicsTimeStamp *timeStamp);

        /* General set/get hardware functions */
        asynStatus setParamGeneric(int funcionId, int addr);
        asynStatus setParam32(int functionId, epicsUInt32 mask, int addr);
        asynStatus getParam32(int functionId, epicsUInt32 *param,
                epicsUInt32 mask, int addr);
        asynStatus setParamInteger(int functionId, int addr);
        asynStatus getParamInteger(int functionId, epicsInt32 *param, int addr);
        asynStatus setParamDouble(int functionId, int addr);
        asynStatus getParamDouble(int functionId, epicsFloat64 *param, int addr);

        /* Specific hardware functions that need extra processing and don't
         * fit into the general set/get template */
        asynStatus setDataTrigChan(epicsUInt32 mask, int addr);
        asynStatus getDataTrigChan(epicsUInt32 *channel, epicsUInt32 mask, int addr);
        asynStatus updateUInt32Params(epicsUInt32 mask, int addr, int firstParam,
                int lastParam, bool acceptErrors);
        asynStatus updateIntegerParams(int addr, int firstParam,
                int lastParam, bool acceptErrors);
        asynStatus updateDoubleParams(int addr, int firstParam, int lastParam,
                bool acceptErrors);
        asynStatus readTriggerParams(epicsUInt32 mask, int addr);
};

#define NUM_PARAMS (&LAST_COMMAND - &FIRST_COMMAND + 1)

/********************************************************************/
/*************** fucntionsAny_t template functions ******************/
/********************************************************************/

/* Read function for Hw execution */
template<typename T>
asynStatus functionsAny_t::executeHwReadFunction(const drvFOFB& drvFOFB,
        const any& functionFp, char *service,
    int addr, functionsArgs_t &functionParam)
{
    if(!any_cast<T>(functionFp).read) {
        return asynSuccess;
    }
    auto functionFpCast = any_cast<T>(functionFp);
    return drvFOFB.doExecuteHwReadFunction(functionFpCast, service, addr, functionParam);
}

/* Write function for Hw execution */
template<typename T>
asynStatus functionsAny_t::executeHwWriteFunction(const drvFOFB& drvFOFB,
        const any& functionFp, char *service,
    int addr, functionsArgs_t &functionParam)
{
    if(!any_cast<T>(functionFp).write) {
        return asynSuccess;
    }
    auto functionFpCast = any_cast<T>(functionFp);
    return drvFOFB.doExecuteHwWriteFunction(functionFpCast, service, addr, functionParam);
}

/* Service name utilities */
template<typename T>
const char *functionsAny_t::getServiceNameFromFunc(const drvFOFB& drvFOFB,
        const any& functionFp) const
{
    auto functionFpCast = any_cast<T>(functionFp);
    return drvFOFB.doGetServiceNameFromFunc(functionFpCast);
}
