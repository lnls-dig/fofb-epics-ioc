/*
 *  *  * drvFOFB.h
 *   *   *
 *    *    * Authors: Melissa Aguiar
 *     *     *
 *      *      * Created Dec. 03, 2021
 *       *       */

#include "asynPortDriver.h"
#include <epicsExit.h>
#include <epicsMutex.h>
/* Third-party libraries */
#include <unordered_map>
#include <halcs_client.h>

/* any implementation for non c++-17 compilers */
#include "any.hpp"

using linb::any;
using linb::any_cast;
using linb::bad_any_cast;

#define ARRAY_SIZE(ARRAY)             (sizeof(ARRAY)/sizeof((ARRAY)[0]))

#define MAX_SLOTS                     12
#define MAX_FOFB_PER_SLOT             2
#define MAX_FOFBS                     (MAX_SLOTS*MAX_FOFB_PER_SLOT)

#define FOFB_NUMBER_MIN               1
#define FOFB_NUMBER_MAX               MAX_FOFBS

#define MAX_ADDR                      8

/* FOFB Mappping structure */
typedef struct {
    int board;
    int fofb;
} boardMap_t;

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
 *  *  * They are used by asyn clients, including standard asyn device support */
#define P_FofbProcessingRamWriteString          "FOFB_PROCESSING_RAM_WRITE"                 /* asynUInt32Digital,      r/w */
#define P_FofbProcessingRamAddrString           "FOFB_PROCESSING_RAM_ADDR"                  /* asynUInt32Digital,      r/w */
#define P_FofbProcessingRamDataInString         "FOFB_PROCESSING_RAM_DATA_IN"               /* asynUInt32Digital,      r/w */
#define P_FofbProcessingRamDataOutString        "FOFB_PROCESSING_RAM_DATA_OUT"              /* asynUInt32Digital,      r/o */
#define P_RtmLampStatusString                   "RTMLAMP_OHWR_STA"                          /* asynUInt32Digital,      r/o */
#define P_RtmLampDacDataFromWbString            "RTMLAMP_OHWR_CTL_DAC_DATA_FROM_WB"         /* asynUInt32Digital,      r/w */
#define P_RtmLampAmpIFlagLString                "RTMLAMP_OHWR_CH_0_STA_AMP_IFLAG_L"         /* asynUInt32Digital,      r/w */
#define P_RtmLampAmpTFlagLString                "RTMLAMP_OHWR_CH_0_STA_AMP_TFLAG_L"         /* asynUInt32Digital,      r/w */
#define P_RtmLampAmpIFlagRString                "RTMLAMP_OHWR_CH_0_STA_AMP_IFLAG_R"         /* asynUInt32Digital,      r/w */
#define P_RtmLampAmpTFlagRString                "RTMLAMP_OHWR_CH_0_STA_AMP_TFLAG_R"         /* asynUInt32Digital,      r/w */
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

class drvFOFB : public asynPortDriver {
    public:
        drvFOFB(const char *portName, const char *endpoint,
                int FOFBNumber, int verbose, int timeout);
        ~drvFOFB();

        /* These are the methods that we override from asynPortDriver */
        virtual asynStatus writeUInt32Digital(asynUser *pasynUser, epicsUInt32 value,
                epicsUInt32 mask);
        virtual asynStatus readUInt32Digital(asynUser *pasynUser, epicsUInt32 *value,
                epicsUInt32 mask);

        /* These methods are overwritten from asynPortDriver */
        virtual asynStatus connect(asynUser* pasynUser);
        virtual asynStatus disconnect(asynUser* pasynUser);

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

        /* Overloaded function mappings called by functionsAny_t */
        asynStatus doExecuteHwWriteFunction(functionsInt32_t &func, char *service,
                int addr, functionsArgs_t &functionParam) const;
        asynStatus doExecuteHwWriteFunction(functionsUInt32_t &func, char *service,
                int addr, functionsArgs_t &functionParam) const;
        asynStatus executeHwWriteFunction(int functionId, int addr,
                functionsArgs_t &functionParam);
        asynStatus doExecuteHwWriteFunction(functionsUInt32Chan_t &func, char *service,
                int addr, functionsArgs_t &functionParam) const;

        asynStatus doExecuteHwReadFunction(functionsInt32_t &func, char *service,
                int addr, functionsArgs_t &functionParam) const;
        asynStatus doExecuteHwReadFunction(functionsUInt32_t &func, char *service,
                int addr, functionsArgs_t &functionParam) const;
        asynStatus executeHwReadFunction(int functionId, int addr,
                functionsArgs_t &functionParam);
        asynStatus doExecuteHwReadFunction(functionsUInt32Chan_t &func, char *service,
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
        int P_FofbProcessingRamWrite;
#define FIRST_COMMAND P_FofbProcessingRamWrite
        int P_FofbProcessingRamAddr;
        int P_FofbProcessingRamDataIn;
        int P_FofbProcessingRamDataOut;
        int P_RtmLampStatus;
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
#define LAST_COMMAND P_RtmLampPISPLimInf

    private:
        /* Our data */
        halcs_client_t *fofbClient;
        char *endpoint;
        int fofbNumber;
        int verbose;
        int timeout;
        char *fofbPortName;
        std::unordered_map<int, functionsAny_t> fofbHwFunc;

        /* Our private methods */

        /* Client connection management */
        asynStatus fofbClientConnect(asynUser* pasynUser);
        asynStatus fofbClientDisconnect(asynUser* pasynUser);

        /* General set/get hardware functions */
        asynStatus setParamGeneric(int funcionId, int addr);
        asynStatus setParam32(int functionId, epicsUInt32 mask, int addr);
        asynStatus getParam32(int functionId, epicsUInt32 *param,
                epicsUInt32 mask, int addr);
        asynStatus setParamDouble(int functionId, int addr);
        asynStatus getParamDouble(int functionId, epicsFloat64 *param, int addr);

        /* Specific hardware functions that need extra processing and don't
 *  *          * fit into the general set/get template */

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
