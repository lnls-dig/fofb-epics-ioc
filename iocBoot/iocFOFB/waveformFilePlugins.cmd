##################### GEN Data File Saving ##########################

NDFileNetCDFConfigure("GEN_ABCD_FileNetCDF", $(QSIZE), 0, "$(PORT)", 4)
dbLoadRecords("$(AREA_DETECTOR_COMPAT)/db/NDFileNetCDF.template","P=${P},R=${R}GEN_ABCD_NETCDF,PORT=GEN_ABCD_FileNetCDF,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT),NDARRAY_ADDR=4")

NDFileHDF5Configure("GEN_ABCD_FileHDF5", $(QSIZE), 0, "$(PORT)", 4)
dbLoadRecords("$(AREA_DETECTOR_COMPAT)/db/NDFileHDF5.template",  "P=${P},R=${R}GEN_ABCD_HDF5,PORT=GEN_ABCD_FileHDF5,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT),NDARRAY_ADDR=4")

##################### PM Data File Saving ##########################

NDFileNetCDFConfigure("PM_ABCD_FileNetCDF", $(QSIZE), 0, "$(PORT)", 10)
dbLoadRecords("$(AREA_DETECTOR_COMPAT)/db/NDFileNetCDF.template","P=${P},R=${R}PM_ABCD_NETCDF,PORT=PM_ABCD_FileNetCDF,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT),NDARRAY_ADDR=10")

NDFileHDF5Configure("PM_ABCD_FileHDF5", $(QSIZE), 0, "$(PORT)", 10)
dbLoadRecords("$(AREA_DETECTOR_COMPAT)/db/NDFileHDF5.template",  "P=${P},R=${R}PM_ABCD_HDF5,PORT=PM_ABCD_FileHDF5,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT),NDARRAY_ADDR=10")
