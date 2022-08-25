  # # # # # # # # # # # # # # # # # # # # # # # # # #
 #                                                   #
#   Description: Script to update RAM Coefficients    #
#                                                     #
#   Author: Melissa Aguiar                            #
#                                                     #
#   Created: May. 05, 2022                            #
 #                                                   #
  # # # # # # # # # # # # # # # # # # # # # # # # # #

# importing libraries
from epics import PV
import numpy as np
from fxpmath import Fxp

# constants
channels       = 8                # number of FOFB Processing channels
n_bits         = 32               # number of bits for RAM data
n_fix          = 26               # fixed point (the same value that we set in gateware)
ram_size       = 2**channels      # size of RAM (the same value tha we set in gateware)
addrs_in_use   = 80               # number of coefficients (80 horizontal and 80 vertical)
ram_addr       = []               # list with RAM addresses in use
ram_data_in    = []               # list with RAM coefficients
crates         = ["01", "02", "03", "04", "05", "06", "07", "08", "09", "10",                                                                                                               "11", "12", "13", "14", "15", "16", "17", "18", "19", "20"]

print('\n')
print('         # # # # # # # # # # # # # # # # # # # # # # # # # #')
print('        #                                                   #')
print('       #  Description: Script to update RAM Coefficients     #')
print('       #                                                     #')
print('       #                                                     #')
print('       #  Created: May. 05, 2022                             #')
print('        #                                                   #')
print('         # # # # # # # # # # # # # # # # # # # # # # # # # #\n')

for j in range(0, channels):
  for i in range(j*ram_size, j*ram_size + addrs_in_use):
    ram_addr.append(i)

# load coefficients
InvRespMat_h  = np.loadtxt('sirius_SOFB_InvRespMat_Horizontal_Plane.txt') # find the correct coefficients and change here
InvRespMat_v  = np.loadtxt('sirius_SOFB_InvRespMat_Vertical_Plane.txt')

# generate list with RAM coefficients for all racks 
for j in range(0, addrs_in_use):
	for i in InvRespMat_h[j][:]:
		s = Fxp(i, signed=True, n_word=n_bits, n_frac=n_fix)
		ram_data_in.append(s.raw())
	for i in InvRespMat_v[j][:]:
		s = Fxp(i, signed=True, n_word=n_bits, n_frac=n_fix)
		ram_data_in.append(s.raw()) 

cnt = 0
for crate_number in crates:
	print('>>> Update RAM coefficients for crate ', crate_number)
	pv_prefix    = "IA-" + crate_number + "RaBPM:BS-FOFBCtrl:"
	addr         = PV(pv_prefix + 'FofbProcessingRamAddr-SP')
	data_in      = PV(pv_prefix + 'FofbProcessingRamDataIn-SP')
	write_en     = PV(pv_prefix + 'FofbProcessingRamWriteEnable-SP')
	for i in ram_addr:
		addr.put(i, wait=True)
		data_in.put(ram_data_in[cnt], wait=True)
		write_en.put(1, wait=True)
		cnt = cnt + 1
	print('... Done!')
write_en.put(0, wait=True)

print('\n--------------------------------------------------------------------------')
print('----------------------------------- END ----------------------------------')
print('--------------------------------------------------------------------------\n')
