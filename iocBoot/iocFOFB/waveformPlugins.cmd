##################### FOFB configuration ##########################

dbLoadRecords("${TOP}/db/FOFB.template", "P=${P}, R=${R}, GLOBAL_ARRAY_NAME_X=X, GLOBAL_ARRAY_NAME_Y=Y, GLOBAL_ARRAY_NAME_XY=XY, GLOBAL_ARRAY_NAME_Q=Q, GLOBAL_ARRAY_NAME_SUM=SUM, PORT=$(PORT), ADDR=0, TIMEOUT=1")

##################### GEN Data Waveforms ###########################

NDStdArraysConfigure("GEN_CH0_Array", $(QSIZE), 0, "$(PORT)", 0)
dbLoadRecords("$(AREA_DETECTOR_COMPAT)/db/NDStdArrays.template", "P=${P},R=${R}GEN_CH0,PORT=GEN_CH0_Array,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT),NDARRAY_ADDR=0,TYPE=Int32,FTVL=LONG,NELEMENTS=$(WAVEFORM_MAX_POINTS)")
