< envPaths

# Override default TOP variable
epicsEnvSet("TOP","../..")
epicsEnvSet("EPICS_DB_INCLUDE_PATH", "$(ADCORE)/db")

# devIOCStats vars
epicsEnvSet("ENGINEER","$(ENGINEER=Melissa Aguiar)")
epicsEnvSet("LOCATION","$(LOCATION=GCA)")
epicsEnvSet("EPICS_DB_INCLUDE_PATH", "$(ADCORE)/db")

< FOFB.config

## Register all support components
dbLoadDatabase("${TOP}/dbd/FOFB.dbd")
FOFB_registerRecordDeviceDriver (pdbbase)

drvFOFBConfigure("$(FOFB_NAME)", "$(FOFB_ENDPOINT)", "$(FOFB_NUMBER)",  "$(FOFB_TYPE)", "$(FOFB_VERBOSE)", "$(FOFB_TIMEOUT)", "$(WAVEFORM_MAX_POINTS)", "$(MAXBUFFERS)", "$(MAXMEMORY)")

## Load record instances
dbLoadRecords("${TOP}/FOFBApp/Db/FOFB.template", "P=${P}, R=${R}, PORT=$(PORT), ADDR=0, TIMEOUT=1")
dbLoadRecords("$(ASYN)/db/asynRecord.db","P=${P}, R=${R}asyn,PORT=$(PORT),ADDR=0,OMAX=80,IMAX=80")

# devIOCStats records
dbLoadRecords("$(DEVIOCSTATS)/db/iocAdminSoft.db","IOC=${P}${R}Stats")
dbLoadRecords("$(DEVIOCSTATS)/db/iocAdminScanMon.db","IOC=${P}${R}Stats")

< save_restore.cmd

< triggerBPM.cmd
< fmc250m_4ch.cmd
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

# save things every thirty seconds
create_monitor_set("auto_settings.req", 30,"P=${P}, R=${R}")
set_savefile_name("auto_settings.req", "auto_settings_${P}${R}.sav")
