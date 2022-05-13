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
ram_size       = 2**channels      # size of RAM fixed in gateware
addrs_in_use   = 160              # size of RAM actually in use
ram_addr       = []               # list with RAM addresses in use
ram_data_in    = []               # list with RAM coefficients
crates         = ["01", "02", "03"]

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
InvRespMat_h  = np.loadtxt('sirius_SOFB_InvRespMat_Vertical_Plane.txt') # find the correct coefficients and change here
InvRespMat_v  = np.loadtxt('sirius_SOFB_InvRespMat_Vertical_Plane.txt')

# generate list with RAM coefficients for all channels
for j in range(0, addrs_in_use):
	for i in InvRespMat_h[j][:]:
		s = Fxp(i, signed=True, n_word=32, n_frac=26)
		ram_data_in.append(s.raw())
	for i in InvRespMat_v[j][:]:
		s = Fxp(i, signed=True, n_word=32, n_frac=26)
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
