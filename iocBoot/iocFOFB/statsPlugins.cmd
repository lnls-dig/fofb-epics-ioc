# Create statistics plugins

##################### GEN Data Statistics ##########################

#NDStatsConfigure("STATSGEN_CH0", $(QSIZE), 0, "$(PORT)", 0, 0, 0)
#dbLoadRecords("$(AREA_DETECTOR_COMPAT)/db/NDStats.template",     "P=${P}, R=${R}GEN_CH0_STATS, PORT=STATSGEN_CH0,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT),NDARRAY_ADDR=0,HIST_SIZE=256,XSIZE=$(RING_SIZE),YSIZE=0,NCHANS=$(NCHANS)")
#NDTimeSeriesConfigure("STATSGEN_CH0_TS", $(QSIZE), 0, "STATSGEN_CH0", 0, 27)
#dbLoadRecords("$(AREA_DETECTOR_COMPAT)/db/NDTimeSeries.template",  "P=${P}, R=${R}GEN_CH0_TS_STATS, PORT=STATSGEN_CH0_TS,ADDR=0,TIMEOUT=1,NDARRAY_PORT=STATSGEN_CH0,NDARRAY_ADDR=0,NCHANS=$(NCHANS),ENABLED=0")

