##################### FOFB configuration ##########################

dbLoadRecords("${TOP}/db/FOFB.template", "P=${P}, R=${R}, GLOBAL_ARRAY_NAME_X=X, GLOBAL_ARRAY_NAME_Y=Y, GLOBAL_ARRAY_NAME_XY=XY, GLOBAL_ARRAY_NAME_Q=Q, GLOBAL_ARRAY_NAME_SUM=SUM, PORT=$(PORT), ADDR=0, TIMEOUT=1")
