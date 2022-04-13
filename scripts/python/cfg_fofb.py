  # # # # # # # # # # # # # # # # # # # # # # # # # #
 #                                                   #
#   Description: Script to configure FOFB/BPM boards  #
#                                                     #
#   Author: Melissa Aguiar                            #
#                                                     #
#   Created: Apr. 13, 2022                            #
 #                                                   #
  # # # # # # # # # # # # # # # # # # # # # # # # # #

# importing libraries

from time import sleep
from epics import PV
import numpy as np
import sys

# parameters

crate_number   = sys.argv[1]                          # crate number
afc_fofb_slot  = sys.argv[2]                          # FOFB slot number (physical_slot*2-1)
trigger_chan   = ["1", "2", "20"]                     # trigger channels
boards         = [int(afc_fofb_slot), 11, 13, 15, 19] # FOFB and BPMs slot number (physical_slot*2-1)
rcv_src        = []
rcv_in_sel     = []

# PV prefixes

if afc_fofb_slot == ("03" or "05"):                   # board connected in physical slot 2 or 3
  pv_prefix    = "IA-" + crate_number + "RaBPM:BS-FOFBCtrl:" 
else:
  pv_prefix    = "XX-" + crate_number + "SL" + afc_fofb_slot + "RaBPM:BS-FOFBCtrl:"

cc_enable      = PV(pv_prefix + 'FofbCtrlCcEnable-SP')
bpm_id         = PV(pv_prefix + 'FofbCtrlBpmId-SP')
time_frame_len = PV(pv_prefix + 'FofbCtrlTimeFrameLen-SP')
bpm_id_rdback  = PV(pv_prefix + 'FofbCtrlBpmIdRdback-RB')

amc5_state     = PV('XX-'+ crate_number + 'SL01:TI-AMCFPGAEVR:AMC5State-Sel')

for i in trigger_chan:
	rcv_src.append(PV(pv_prefix    + 'TRIGGER' + i + 'RcvSrc-Sel'))
	rcv_in_sel.append(PV(pv_prefix + 'TRIGGER' + i + 'RcvInSel-SP'))

print('\n')
print('  # # # # # # # # # # # # # # # # # # # # # # # # # #')
print(' #                                                   #')
print('#   Description: Script to configure FOFB/BPM boards  #')
print('#                                                     #')
print('#                                                     #')
print('#   Created: Apr. 13, 2022                            #')
print(' #                                                   #')
print('  # # # # # # # # # # # # # # # # # # # # # # # # # #\n')

# Configure FOFB/BPM boards

amc5_state.put(0, wait=True)
print("-------------------------------------------------------")
print("amc5_state     : " + str(amc5_state.get()))

time_frame_len.put(5000, wait=True)

print("-------------------------------------------------------")
print("time_frame_len : " + str(time_frame_len.get()))

for i in boards:
	print("-------------------------------------------------------")
	bpm_id.put(i, wait=True)
	cc_enable.put(0, wait=True)

	print("bpm_id         : " + str(bpm_id.get()))
	print("\ncc_enable      : " + str(cc_enable.get()))

	sleep(0.3)
	cc_enable.put(1, wait=True)
	sleep(0.3)

	print("cc_enable      : " + str(cc_enable.get()))
	print("\nbpm_id_rdback  : " + str(bpm_id_rdback.get()))

	# Configure Trigger
	for i in range(0, np.size(trigger_chan)):
		rcv_src[i].put(0, wait=True)
		rcv_in_sel[i].put(5, wait=True)

	for i in range(0, np.size(trigger_chan)):
		print("\nrcv_src        : " + str(rcv_src[i].get()))
		print("rcv_in_sel     : " + str(rcv_in_sel[i].get()))

amc5_state.put(1, wait=True)
bpm_id.put(-1, wait=True)
print("-------------------------------------------------------")
print("amc5_state     : " + str(amc5_state.get()))
print("-------------------------------------------------------")

print('\n-------------------------------------------------------')
print('----------------------- END ---------------------------')
print('-------------------------------------------------------\n')

