  # # # # # # # # # # # # # # # # # # # # # # # # # #
 #                                                   #
#   Description: Script to set RTM-LAMP parameters    #
#                                                     #
#   Author: Melissa Aguiar                            #
#                                                     #
#   Created: Apr. 13, 2022                            #
 #                                                   #
  # # # # # # # # # # # # # # # # # # # # # # # # # #

# importing libraries

import os
import sys
import time
import numpy as np
from   epics import PV

# constants

fs                        = 1/940e-9                 # frequency for PSD calc
samples                   = 10000                    # number of samples for acquisition
channels                  = 12                       # number of channels (max 12, 8 actually in use)
pi_kp                     = 5000000                  # PI Kp parameter
pi_ti                     = 300                      # PI Ti parameter
crate_number              = sys.argv[1]              # crate number (two digits)
slot_number               = sys.argv[2]              # slot number (physical_slot*2-1 = two digits)
current_gain              = 6.25e-5                  # initial value for current gain
voltage_gain              = 1.12916762036e-4         # initial value for voltage gain
current_offset            = 0                        # initial value for current offset
voltage_offset            = 0                        # initial value for voltage offset
dac_cnt_max               = 125000                   # 2ms

# PV prefixes

if slot_number == "03" or slot_number == "05":       # board connected in physical slot 2 or 3
  prefix_fofb  = "IA-" + crate_number + "RaBPM:BS-FOFBCtrl:" 
else:
  prefix_fofb  = "XX-" + crate_number + "SL" + slot_number + "RaBPM:BS-FOFBCtrl:"

prefix_rtmch00 = "SI-" + crate_number + "M1:PS-FCH:"
prefix_rtmch01 = "SI-" + crate_number + "M1:PS-FCV:"
prefix_rtmch02 = "SI-" + crate_number + "M2:PS-FCH:"
prefix_rtmch03 = "SI-" + crate_number + "M2:PS-FCV:"
prefix_rtmch04 = "SI-" + crate_number + "C2:PS-FCH:"
prefix_rtmch05 = "SI-" + crate_number + "C2:PS-FCV:"
prefix_rtmch06 = "SI-" + crate_number + "C3:PS-FCH:"
prefix_rtmch07 = "SI-" + crate_number + "C3:PS-FCV:"
prefix_rtmch08 = "SI-" + crate_number + "XX:PS-FC08:"
prefix_rtmch09 = "SI-" + crate_number + "XX:PS-FC09:"
prefix_rtmch10 = "SI-" + crate_number + "XX:PS-FC10:"
prefix_rtmch11 = "SI-" + crate_number + "XX:PS-FC11:"

prefix_rtm = [prefix_rtmch00, prefix_rtmch01, prefix_rtmch02, prefix_rtmch03, prefix_rtmch04, prefix_rtmch05,
              prefix_rtmch06, prefix_rtmch07, prefix_rtmch08, prefix_rtmch09, prefix_rtmch10, prefix_rtmch11]

# global PVs

pv_acq_trigger_rep        = str(prefix_fofb) + str("ACQTriggerRep-Sel")
pv_acq_trigger_event      = str(prefix_fofb) + str("ACQTriggerEvent-Sel")
pv_acq_samples_pre        = str(prefix_fofb) + str("ACQSamplesPre-SP")
pv_setpoint_inf           = str(prefix_fofb) + str("TestLowLim-SP")
pv_dac_cnt_max            = str(prefix_fofb) + str("TestPIDacCntMax-SP")

# PVs per channel

pv_current_ArrayDataRAW   = []
pv_current_gain           = []
pv_voltage_gain           = []
pv_current_offset         = []
pv_voltage_offset         = []
pv_current_setpoint       = []
pv_amp_enable             = []
pv_pi_enable              = []
pv_pi_kp                  = []
pv_pi_ti                  = []
pv_square_wave_openloop   = []
pv_test_high_lim          = []

# getting lists of PV names, so we can reutilize them in all tests

for i in range(0, channels):
  pv_current_ArrayDataRAW.append(str(prefix_fofb) + str("GEN_CH") + str(i) + str("ArrayData"))

for pv_prefix in prefix_rtm:
  pv_current_gain.append(        str(pv_prefix)   + str("CurrGain")           + str("-SP"))
  pv_voltage_gain.append(        str(pv_prefix)   + str("VoltGain")           + str("-SP"))
  pv_current_offset.append(      str(pv_prefix)   + str("CurrOffset")         + str("-SP"))
  pv_voltage_offset.append(      str(pv_prefix)   + str("VoltOffset")         + str("-SP"))
  pv_current_setpoint.append(    str(pv_prefix)   + str("Current")            + str("-SP"))
  pv_test_high_lim.append(       str(pv_prefix)   + str("TestHighLim")        + str("-SP"))
  pv_amp_enable.append(          str(pv_prefix)   + str("PwrState")           + str("-Sel"))
  pv_pi_enable.append(           str(pv_prefix)   + str("CtrlLoop")           + str("-Sel"))
  pv_pi_kp.append(               str(pv_prefix)   + str("CtrlLoopKp")         + str("-SP"))
  pv_pi_ti.append(               str(pv_prefix)   + str("CtrlLoopTi")         + str("-SP"))
  pv_square_wave_openloop.append(str(pv_prefix)   + str("TestOpenLoopSquare") + str("-Sel"))

print('\n')
print('         # # # # # # # # # # # # # # # # # # # # # # # # # #')
print('        #                                                   #')
print('       #   Description: Script to set RTM-LAMP parameters    #')
print('       #                                                     #')
print('       #                                                     #')
print('       #   Created: Apr. 13, 2022                            #')
print('        #                                                   #')
print('         # # # # # # # # # # # # # # # # # # # # # # # # # #\n')

print('>>> Set initial values for gain, offset, PI Kp and PI Ti ...')

# initializing some PVs with default values

for i in range(0, channels):
  PV(pv_current_gain[i]).put(current_gain,     wait=True)
  PV(pv_current_offset[i]).put(current_offset, wait=True)
  PV(pv_voltage_gain[i]).put(voltage_gain,     wait=True)
  PV(pv_voltage_offset[i]).put(voltage_offset, wait=True)
  PV(pv_pi_kp[i]).put(pi_kp,                   wait=True)
  PV(pv_pi_ti[i]).put(pi_ti,                   wait=True)
  PV(pv_amp_enable[i]).put(0,                  wait=True)
  PV(pv_pi_enable[i]).put(0,                   wait=True)
  PV(pv_square_wave_openloop[i]).put(0,        wait=True)

print('>>> Set initial values for gain, offset, PI Kp and PI Ti... Done!\n')

print('>>> Set the period for 10ms...')

PV(pv_dac_cnt_max).put(dac_cnt_max, wait=True)

print('>>> Set the period for 10ms... Done!\n')

print('>>> Set zero for the current setpoints...')

for i in range(0, channels):
    PV(pv_current_setpoint[i]).put(0, wait=True)
    PV(pv_test_high_lim[i]).put(0,    wait=True)
PV(pv_setpoint_inf).put(0,            wait=True)

print('>>> Set zero for the current setpoints... Done!')

print('\n>>> New acquisition...')

# do an acquisition and stop the event,
# so the array data will be the same until we do another acquisition

PV(pv_acq_samples_pre).put(samples, wait=True)
PV(pv_acq_trigger_rep).put(0,       wait=True)
PV(pv_acq_trigger_event).put(0,     wait=True)
time.sleep(0.5) # just to see the waveform change in graphical interface
PV(pv_acq_trigger_event).put(1,     wait=True)

print('>>> New acquisition... Done!')

print('\n--------------------------------------------------------------------------')
print('--------------------- Calculate the current offset -----------------------')
print('--------------------------------------------------------------------------\n')

new_offset = np.zeros(channels)
for i in range(0, channels):
  new_offset[i] = int(np.mean(PV(pv_current_ArrayDataRAW[i]).get()))

print('>> New current offset values: \n', new_offset)

print('\n>>> Set the new current offset for all channels...')

for i in range(0, channels):
  PV(pv_current_offset[i]).put(new_offset[i])

print('>>> Set the new current offset for all channels... Done!')

print('\n--------------------------------------------------------------------------')
print('----------------------------------- END ----------------------------------')
print('--------------------------------------------------------------------------\n')

