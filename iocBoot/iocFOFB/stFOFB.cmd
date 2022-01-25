< envPaths

# Override default TOP variable
epicsEnvSet("TOP","../..")
epicsEnvSet("EPICS_DB_INCLUDE_PATH", "$(ADCORE)/db")

epicsEnvSet("FOFB_TYPE", "FOFB")

# devIOCStats vars
epicsEnvSet("ENGINEER","$(ENGINEER=Melissa Aguiar)")
epicsEnvSet("LOCATION","$(LOCATION=GCA)")
epicsEnvSet("EPICS_DB_INCLUDE_PATH", "$(ADCORE)/db")
epicsEnvSet("STARTUP","$(TOP)")

< FOFB.config

## Register all support components
dbLoadDatabase("${TOP}/dbd/FOFB.dbd")
FOFB_registerRecordDeviceDriver (pdbbase)

drvFOFBConfigure("$(FOFB_NAME)", "$(FOFB_ENDPOINT)", "$(FOFB_NUMBER)", "$(FOFB_TYPE)", "$(FOFB_VERBOSE)", "$(FOFB_TIMEOUT)", "$(WAVEFORM_MAX_POINTS)", "$(MAXBUFFERS)", "$(MAXMEMORY)")

## Load record instances
dbLoadRecords("${TOP}/FOFBApp/Db/FOFBRtm.template", "P=${P}, R=${R}, RTM_CHAN=CH0,  PORT=$(PORT), ADDR=0,  TIMEOUT=1")
dbLoadRecords("${TOP}/FOFBApp/Db/FOFBRtm.template", "P=${P}, R=${R}, RTM_CHAN=CH1,  PORT=$(PORT), ADDR=1,  TIMEOUT=1")
dbLoadRecords("${TOP}/FOFBApp/Db/FOFBRtm.template", "P=${P}, R=${R}, RTM_CHAN=CH2,  PORT=$(PORT), ADDR=2,  TIMEOUT=1")
dbLoadRecords("${TOP}/FOFBApp/Db/FOFBRtm.template", "P=${P}, R=${R}, RTM_CHAN=CH3,  PORT=$(PORT), ADDR=3,  TIMEOUT=1")
dbLoadRecords("${TOP}/FOFBApp/Db/FOFBRtm.template", "P=${P}, R=${R}, RTM_CHAN=CH4,  PORT=$(PORT), ADDR=4,  TIMEOUT=1")
dbLoadRecords("${TOP}/FOFBApp/Db/FOFBRtm.template", "P=${P}, R=${R}, RTM_CHAN=CH5,  PORT=$(PORT), ADDR=5,  TIMEOUT=1")
dbLoadRecords("${TOP}/FOFBApp/Db/FOFBRtm.template", "P=${P}, R=${R}, RTM_CHAN=CH6,  PORT=$(PORT), ADDR=6,  TIMEOUT=1")
dbLoadRecords("${TOP}/FOFBApp/Db/FOFBRtm.template", "P=${P}, R=${R}, RTM_CHAN=CH7,  PORT=$(PORT), ADDR=7,  TIMEOUT=1")
dbLoadRecords("${TOP}/FOFBApp/Db/FOFBRtm.template", "P=${P}, R=${R}, RTM_CHAN=CH8,  PORT=$(PORT), ADDR=8,  TIMEOUT=1")
dbLoadRecords("${TOP}/FOFBApp/Db/FOFBRtm.template", "P=${P}, R=${R}, RTM_CHAN=CH9,  PORT=$(PORT), ADDR=9,  TIMEOUT=1")
dbLoadRecords("${TOP}/FOFBApp/Db/FOFBRtm.template", "P=${P}, R=${R}, RTM_CHAN=CH10, PORT=$(PORT), ADDR=10, TIMEOUT=1")
dbLoadRecords("${TOP}/FOFBApp/Db/FOFBRtm.template", "P=${P}, R=${R}, RTM_CHAN=CH11, PORT=$(PORT), ADDR=11, TIMEOUT=1")
dbLoadRecords("${TOP}/FOFBApp/Db/FOFBRtm.template", "P=${P}, R=${R}, RTM_CHAN=CH12, PORT=$(PORT), ADDR=11, TIMEOUT=1")
dbLoadRecords("${TOP}/FOFBApp/Db/FOFBRtm.template", "P=${P}, R=${R}, RTM_CHAN=CH13, PORT=$(PORT), ADDR=11, TIMEOUT=1")
dbLoadRecords("${TOP}/FOFBApp/Db/FOFBRtm.template", "P=${P}, R=${R}, RTM_CHAN=CH14, PORT=$(PORT), ADDR=11, TIMEOUT=1")
dbLoadRecords("${TOP}/FOFBApp/Db/FOFBRtm.template", "P=${P}, R=${R}, RTM_CHAN=CH15, PORT=$(PORT), ADDR=11, TIMEOUT=1")

dbLoadRecords("${TOP}/db/FOFBData.template", "P=${P},R=${R},ACQ_NAME=GEN,RTM_CHAN=CH0, FTVL=FLOAT,NELEMENTS=$(WAVEFORM_MAX_POINTS)")
dbLoadRecords("${TOP}/db/FOFBData.template", "P=${P},R=${R},ACQ_NAME=GEN,RTM_CHAN=CH1, FTVL=FLOAT,NELEMENTS=$(WAVEFORM_MAX_POINTS)")
dbLoadRecords("${TOP}/db/FOFBData.template", "P=${P},R=${R},ACQ_NAME=GEN,RTM_CHAN=CH2, FTVL=FLOAT,NELEMENTS=$(WAVEFORM_MAX_POINTS)")
dbLoadRecords("${TOP}/db/FOFBData.template", "P=${P},R=${R},ACQ_NAME=GEN,RTM_CHAN=CH3, FTVL=FLOAT,NELEMENTS=$(WAVEFORM_MAX_POINTS)")
dbLoadRecords("${TOP}/db/FOFBData.template", "P=${P},R=${R},ACQ_NAME=GEN,RTM_CHAN=CH4, FTVL=FLOAT,NELEMENTS=$(WAVEFORM_MAX_POINTS)")
dbLoadRecords("${TOP}/db/FOFBData.template", "P=${P},R=${R},ACQ_NAME=GEN,RTM_CHAN=CH5, FTVL=FLOAT,NELEMENTS=$(WAVEFORM_MAX_POINTS)")
dbLoadRecords("${TOP}/db/FOFBData.template", "P=${P},R=${R},ACQ_NAME=GEN,RTM_CHAN=CH6, FTVL=FLOAT,NELEMENTS=$(WAVEFORM_MAX_POINTS)")
dbLoadRecords("${TOP}/db/FOFBData.template", "P=${P},R=${R},ACQ_NAME=GEN,RTM_CHAN=CH7, FTVL=FLOAT,NELEMENTS=$(WAVEFORM_MAX_POINTS)")
dbLoadRecords("${TOP}/db/FOFBData.template", "P=${P},R=${R},ACQ_NAME=GEN,RTM_CHAN=CH8, FTVL=FLOAT,NELEMENTS=$(WAVEFORM_MAX_POINTS)")
dbLoadRecords("${TOP}/db/FOFBData.template", "P=${P},R=${R},ACQ_NAME=GEN,RTM_CHAN=CH9, FTVL=FLOAT,NELEMENTS=$(WAVEFORM_MAX_POINTS)")
dbLoadRecords("${TOP}/db/FOFBData.template", "P=${P},R=${R},ACQ_NAME=GEN,RTM_CHAN=CH10,FTVL=FLOAT,NELEMENTS=$(WAVEFORM_MAX_POINTS)")
dbLoadRecords("${TOP}/db/FOFBData.template", "P=${P},R=${R},ACQ_NAME=GEN,RTM_CHAN=CH11,FTVL=FLOAT,NELEMENTS=$(WAVEFORM_MAX_POINTS)")
dbLoadRecords("${TOP}/db/FOFBData.template", "P=${P},R=${R},ACQ_NAME=GEN,RTM_CHAN=CH12,FTVL=FLOAT,NELEMENTS=$(WAVEFORM_MAX_POINTS)")
dbLoadRecords("${TOP}/db/FOFBData.template", "P=${P},R=${R},ACQ_NAME=GEN,RTM_CHAN=CH13,FTVL=FLOAT,NELEMENTS=$(WAVEFORM_MAX_POINTS)")
dbLoadRecords("${TOP}/db/FOFBData.template", "P=${P},R=${R},ACQ_NAME=GEN,RTM_CHAN=CH14,FTVL=FLOAT,NELEMENTS=$(WAVEFORM_MAX_POINTS)")
dbLoadRecords("${TOP}/db/FOFBData.template", "P=${P},R=${R},ACQ_NAME=GEN,RTM_CHAN=CH15,FTVL=FLOAT,NELEMENTS=$(WAVEFORM_MAX_POINTS)")

dbLoadRecords("$(ASYN)/db/asynRecord.db","P=${P}, R=${R}asyn,PORT=$(PORT),ADDR=0,OMAX=80,IMAX=80")
dbLoadRecords("${TOP}/db/FOFBAcq.template", "P=${P}, R=${R}, ACQ_NAME=ACQ, PORT=$(PORT), ADDR=0, TIMEOUT=1")
dbLoadRecords("${TOP}/db/FOFBAcq.template", "P=${P}, R=${R}, ACQ_NAME=ACQ_PM, PORT=$(PORT), ADDR=1, TIMEOUT=1")

# devIOCStats records
dbLoadRecords("$(DEVIOCSTATS)/db/iocAdminSoft.db","IOC=${P}${R}Stats")
dbLoadRecords("$(DEVIOCSTATS)/db/iocAdminScanMon.db","IOC=${P}${R}Stats")

< save_restore.cmd
< triggerFOFB.cmd
< waveformPlugins.cmd
< waveformFilePlugins.cmd
< statsPlugins.cmd

# Turn on asynTraceFlow and asynTraceError for global trace, i.e. no connected asynUser.
asynSetTraceIOMask("$(FOFB_NAME)",0,0x2)
#asynSetTraceMask("", 0, 17)
#asynSetTraceMask("$(FOFB_NAME)",0,0xff)

# Disable locking virtual memory as it can increase memory usage
# unnecessarily in systems that run the IOC with SCHED_FIFO and
# supports running threads with different priorities. See:
# https://epics.anl.gov/base/R3-15/6-docs/RELEASE_NOTES.html and
# https://github.com/epics-base/epics-base/commit/e721be4ff528bc1fff35b9e0cffd2a194f3e3675
var dbThreadRealtimeLock 0

iocInit()

< initFOFBCommands

# save things every thirty seconds
create_monitor_set("auto_settings.req", 30,"P=${P}, R=${R}")
set_savefile_name("auto_settings.req", "auto_settings_${P}${R}.sav")
