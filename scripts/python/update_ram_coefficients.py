  # # # # # # # # # # # # # # # # # # # # # # # # # #
 #                                                   #
#   Description: Script to update RAM Coefficients    #
#                                                     #
#   Author: Melissa Aguiar                            #
#                                                     #
#   Created: Mar. 11, 2022                            #
 #                                                   #
  # # # # # # # # # # # # # # # # # # # # # # # # # #

# importing libraries
from epics import PV
import numpy as np
from time import sleep

# constants
crate_number   = "99"                            # crate number
slot_number    = "01"                            # slot number (must be equals to physical_slot*2-1)

ram_addr       = np.loadtxt('ram_addr.txt')
ram_data_in    = np.loadtxt('ram_data_in.txt')

# PV prefixes
if slot_number == ("03" or "05"):                # board connected in physical slot 2 or 3
  pv_prefix    = "IA-" + crate_number + "RaBPM:BS-FOFBCtrl:" 
else:
  pv_prefix    = "XX-" + crate_number + "SL" + slot_number + "RaBPM:BS-FOFBCtrl:"

addr           = PV(pv_prefix + 'FofbProcessingRamAddr-SP')
data_in        = PV(pv_prefix + 'FofbProcessingRamDataIn-SP')
write_en       = PV(pv_prefix + 'FofbProcessingRamWriteEnable-SP')

for i in ram_addr:
	addr.put(i, wait=True)
	data_in.put(int(ram_data_in[int(i)]), wait=True)
	write_en.put(1, wait=True)
write_en.put(0, wait=True)

