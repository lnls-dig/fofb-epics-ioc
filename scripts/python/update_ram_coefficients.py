from epics import PV
import numpy as np
from time import sleep

pv_prefix = "SI-22"

ram_addr    = np.loadtxt('ram_addr.txt')
ram_data_in = np.loadtxt('ram_data_in.txt')

addr     = PV(pv_prefix + ':DI-FOFBCtrl:FofbProcessingRamAddr-SP')
data_in  = PV(pv_prefix + ':DI-FOFBCtrl:FofbProcessingRamDataIn-SP')
write_en = PV(pv_prefix + ':DI-FOFBCtrl:FofbProcessingRamWriteEnable-SP')

for i in ram_addr:
	addr.put(i, wait=True)
	data_in.put(int(ram_data_in[int(i)]), wait=True)
	write_en.put(1, wait=True)
write_en.put(0, wait=True)
