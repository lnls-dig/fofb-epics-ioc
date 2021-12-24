##################### GEN Data Waveforms ###########################

NDStdArraysConfigure("GEN_CH0_Array", $(QSIZE), 0, "$(PORT)", 0)
dbLoadRecords("$(AREA_DETECTOR_COMPAT)/db/NDStdArrays.template", "P=${P},R=${R}GEN_CH0,PORT=GEN_CH0_Array,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT),NDARRAY_ADDR=0,TYPE=Int32,FTVL=LONG,NELEMENTS=$(WAVEFORM_MAX_POINTS)")

##################### FOFB configuration ########################## 
 
dbLoadRecords("${TOP}/db/FOFB.template", "P=${P}, R=${R}, PORT=$(PORT), ADDR=0, TIMEOUT=1") 

