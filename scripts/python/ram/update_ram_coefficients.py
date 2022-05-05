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
from fxpmath import Fxp
import sys

# constants
crate_number   = sys.argv[1]      # crate number (two digits)
slot_number    = sys.argv[2]      # FOFB slot number (physical_slot*2-1 = two digits)
channels       = 8                # number of FOFB Processing channels
ram_size       = 2**channels      # size of RAM fixed in gateware
addrs_in_use   = 160              # size of RAM actually in use
ram_addr       = []               # list with RAM addresses in use
ram_data_in    = []               # list with RAM coefficients

for j in range(0, channels):
  for i in range(j*ram_size, j*ram_size + addrs_in_use):
    ram_addr.append(i)

# load coefficients
RespMat_h     = np.loadtxt('sirius_SOFB_RespMat_Horizontal_Plane.txt')
RespMat_v     = np.loadtxt('sirius_SOFB_RespMat_Vertical_Plane.txt')

InvRespMat_h  = np.loadtxt('sirius_SOFB_InvRespMat_Horizontal_Plane.txt')
InvRespMat_v  = np.loadtxt('sirius_SOFB_InvRespMat_Vertical_Plane.txt')

# generate file with RAM coefficients for all channels (size = channels*addrs_in_use)
for j in range(0, channels):
	for i in InvRespMat_h[j][:]:
		s = Fxp(i, signed=True, n_word=32, n_frac=26)
		ram_data_in.append(s.raw())

# PV prefixes
if slot_number == ("03" or "05"): # board connected in physical slot 2 or 3
  pv_prefix    = "IA-" + crate_number + "RaBPM:BS-FOFBCtrl:" 
else:
  pv_prefix    = "XX-" + crate_number + "SL" + slot_number + "RaBPM:BS-FOFBCtrl:"

addr           = PV(pv_prefix + 'FofbProcessingRamAddr-SP')
data_in        = PV(pv_prefix + 'FofbProcessingRamDataIn-SP')
write_en       = PV(pv_prefix + 'FofbProcessingRamWriteEnable-SP')

cnt = 0
for i in ram_addr:
	addr.put(i, wait=True)
	data_in.put(ram_data_in[cnt], wait=True)
	write_en.put(1, wait=True)
	cnt = cnt + 1
write_en.put(0, wait=True)

