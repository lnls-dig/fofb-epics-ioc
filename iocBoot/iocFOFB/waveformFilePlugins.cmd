##################### GEN AMP File Saving ##########################

NDFileNetCDFConfigure("GEN_ABCD_FileNetCDF", $(QSIZE), 0, "$(PORT)", 4)
dbLoadRecords("$(AREA_DETECTOR_COMPAT)/db/NDFileNetCDF.template","P=${P},R=${R}GEN_ABCD_NETCDF,PORT=GEN_ABCD_FileNetCDF,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT),NDARRAY_ADDR=4")

NDFileHDF5Configure("GEN_ABCD_FileHDF5", $(QSIZE), 0, "$(PORT)", 4)
dbLoadRecords("$(AREA_DETECTOR_COMPAT)/db/NDFileHDF5.template",  "P=${P},R=${R}GEN_ABCD_HDF5,PORT=GEN_ABCD_FileHDF5,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT),NDARRAY_ADDR=4")

##################### GEN POS File Saving ##########################

NDFileNetCDFConfigure("GEN_XYQS_FileNetCDF", $(QSIZE), 0, "$(PORT)", 10)
dbLoadRecords("$(AREA_DETECTOR_COMPAT)/db/NDFileNetCDF.template","P=${P},R=${R}GEN_XYQS_NETCDF,PORT=GEN_XYQS_FileNetCDF,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT),NDARRAY_ADDR=10")

NDFileHDF5Configure("GEN_XYQS_FileHDF5", $(QSIZE), 0, "$(PORT)", 10)
dbLoadRecords("$(AREA_DETECTOR_COMPAT)/db/NDFileHDF5.template",  "P=${P},R=${R}GEN_XYQS_HDF5,PORT=GEN_XYQS_FileHDF5,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT),NDARRAY_ADDR=10")

##################### PM Amp File Saving ##########################

NDFileNetCDFConfigure("PM_ABCD_FileNetCDF", $(QSIZE), 0, "$(PORT)", 16)
dbLoadRecords("$(AREA_DETECTOR_COMPAT)/db/NDFileNetCDF.template","P=${P},R=${R}PM_ABCD_NETCDF,PORT=PM_ABCD_FileNetCDF,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT),NDARRAY_ADDR=16")

NDFileHDF5Configure("PM_ABCD_FileHDF5", $(QSIZE), 0, "$(PORT)", 16)
dbLoadRecords("$(AREA_DETECTOR_COMPAT)/db/NDFileHDF5.template",  "P=${P},R=${R}PM_ABCD_HDF5,PORT=PM_ABCD_FileHDF5,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT),NDARRAY_ADDR=16")

##################### PM POS File Saving ##########################

NDFileNetCDFConfigure("PM_XYQS_FileNetCDF", $(QSIZE), 0, "$(PORT)", 22)
dbLoadRecords("$(AREA_DETECTOR_COMPAT)/db/NDFileNetCDF.template","P=${P},R=${R}PM_XYQS_NETCDF,PORT=PM_XYQS_FileNetCDF,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT),NDARRAY_ADDR=22")

NDFileHDF5Configure("PM_XYQS_FileHDF5", $(QSIZE), 0, "$(PORT)", 22)
dbLoadRecords("$(AREA_DETECTOR_COMPAT)/db/NDFileHDF5.template",  "P=${P},R=${R}PM_XYQS_HDF5,PORT=PM_XYQS_FileHDF5,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT),NDARRAY_ADDR=22")

##################### SP File Saving ##########################

NDFileNetCDFConfigure("SP_ABCD_FileNetCDF", $(QSIZE), 0, "$(PORT)", 28)
dbLoadRecords("$(AREA_DETECTOR_COMPAT)/db/NDFileNetCDF.template","P=${P},R=${R}SP_ABCD_NETCDF,PORT=SP_ABCD_FileNetCDF,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT),NDARRAY_ADDR=28")

NDFileHDF5Configure("SP_ABCD_FileHDF5", $(QSIZE), 0, "$(PORT)", 28)
dbLoadRecords("$(AREA_DETECTOR_COMPAT)/db/NDFileHDF5.template",  "P=${P},R=${R}SP_ABCD_HDF5,PORT=SP_ABCD_FileHDF5,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT),NDARRAY_ADDR=28")

