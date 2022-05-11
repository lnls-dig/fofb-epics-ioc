  # # # # # # # # # # # # # # # # # # # # # # # # # #
 #                                                   #
#   Description: Script to test RTM LAMP open loop    #
#                                                     #
#   Author: Melissa Aguiar                            #
#                                                     #
#   Created: May. 10, 2022                            #
 #                                                   #
  # # # # # # # # # # # # # # # # # # # # # # # # # #

# importing libraries

import os
import sys
import time
import math
import heapq
import numpy as np
from epics import PV
from datetime import datetime

# constants

fs                        = 1/940e-9                 # frequency for PSD calc
samples                   = 2000                     # number of samples for acquisition
channels                  = 8                        # number of channels (max 12, 8 actually in use)
pi_kp                     = 5000000                  # PI Kp parameter
pi_ti                     = 300                      # PI Ti parameter
slot_number               = "03"                     # slot number (physical_slot*2-1 = two digits)
current_gain              = 6.25e-5                  # initial value for current gain
voltage_gain              = 1.12916762036e-4         # initial value for voltage gain
current_offset            = 0                        # initial value for current offset
voltage_offset            = 0                        # initial value for voltage offset
dac_cnt_max               = 62500                    # 1ms
dac_data                  = 4428                     # 0.5V
sp_data                   = 22140                    # 2.5V
data                      = {}                       # json data

crates                    = ["01", "02", "03", "04", "05", "06", "07", "08", "09", "10",
                             "11", "12", "13", "14", "15", "16", "17", "18", "19", "20"]

for crate_number in crates:

  # PV prefixes

  prefix_fofb    = "IA-" + crate_number + "RaBPM:BS-FOFBCtrl:"

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
  pv_current_setpoint_inf   = str(prefix_fofb) + str("TestCurrLowLim-SP")
  pv_dac_cnt_max            = str(prefix_fofb) + str("TestPIDacCntMax-SP")
  pv_dac_data_wb            = str(prefix_fofb) + str("DacDataSrc-Sel")

  # PVs per channel

  pv_current_ArrayDataRAW   = []
  pv_current_ArrayData      = []
  pv_current_gain           = []
  pv_voltage_gain           = []
  pv_current_offset         = []
  pv_voltage_offset         = []
  pv_current_setpoint       = []
  pv_amp_enable             = []
  pv_pi_enable              = []
  pv_pi_kp                  = []
  pv_pi_ti                  = []
  pv_dac_data               = []
  pv_amp_enable             = []
  pv_pi_enable              = []
  pv_dac_write              = []
  pv_square_wave_openloop   = []

  # getting lists of PV names, so we can reutilize them in all tests

  for i in range(0, channels):
    pv_current_ArrayDataRAW.append(str(prefix_fofb) + str("GEN_CH")   + str(i) + str("ArrayData"))
    pv_current_ArrayData.append(   str(prefix_fofb) + str("GENConvArrayDataCH")  + str(i))

  for pv_prefix in prefix_rtm:
    pv_current_gain.append(        str(pv_prefix) + str("CurrGain")   + str("-SP"))
    pv_voltage_gain.append(        str(pv_prefix) + str("VoltGain")   + str("-SP"))
    pv_current_offset.append(      str(pv_prefix) + str("CurrOffset") + str("-SP"))
    pv_voltage_offset.append(      str(pv_prefix) + str("VoltOffset") + str("-SP"))
    pv_current_setpoint.append(    str(pv_prefix) + str("CurrentRaw") + str("-SP"))
    pv_amp_enable.append(          str(pv_prefix) + str("PwrState")   + str("-Sel"))
    pv_pi_enable.append(           str(pv_prefix) + str("CtrlLoop")   + str("-Sel"))
    pv_pi_kp.append(               str(pv_prefix) + str("CtrlLoopKp") + str("-SP"))
    pv_pi_ti.append(               str(pv_prefix) + str("CtrlLoopTi") + str("-SP"))
    pv_dac_data.append(            str(pv_prefix) + str("DacData")    + str("-SP"))
    pv_dac_write.append(           str(pv_prefix) + str("DacWr")      + str("-Cmd"))
    pv_square_wave_openloop.append(str(pv_prefix) + str("TestOpenLoopSquare") + str("-Sel"))

  print('\n')
  print('         # # # # # # # # # # # # # # # # # # # # # # # # # #')
  print('        #                                                   #')
  print('       #   Description: Script to test RTM LAMP open loop    #')
  print('       #                                                     #')
  print('       #                                                     #')
  print('       #   Created: May. 10, 2022                            #')
  print('        #                                                   #')
  print('         # # # # # # # # # # # # # # # # # # # # # # # # # #\n')

  print('\n--------------------------------------------------------------------------')
  print('------------------------------ STARTING TEST -----------------------------')
  print('--------------------------------------------------------------------------\n')

  print('------------------- Get serial number and data/time ----------------------\n')

  serial_number = "CN000" + crate_number + ":1.3"

  today = datetime.now()
  now   = today.strftime("%B %d, %Y. %H:%M:%S")

  print('Serial number: ', str(serial_number))
  print('Now: ', now)

  os.system("mkdir Inductance")

  serial_str_title = serial_number
  serial_str = list(serial_number)
  serial_str[7] = "_"
  serial_number = "".join(serial_str)

  file_name = "Inductance/" + serial_number + ".txt"

  results = open(file_name,'w')

  results.write(now)
  results.write('\n')
  results.write(serial_str_title)
  results.write('\n')

  print('>>> Set initial values for gain, offset, PI Kp and PI Ti ...')

  # initializing some PVs with default values
  PV(pv_current_setpoint_inf).put(0,             wait=True)
  for i in range(0, channels):
    PV(pv_current_gain[i]).put(current_gain,     wait=True)
    PV(pv_voltage_gain[i]).put(voltage_gain,     wait=True)
    PV(pv_voltage_offset[i]).put(voltage_offset, wait=True)
    PV(pv_pi_kp[i]).put(pi_kp,                   wait=True)
    PV(pv_pi_ti[i]).put(pi_ti,                   wait=True)
    PV(pv_amp_enable[i]).put(0,                  wait=True)
    PV(pv_pi_enable[i]).put(0,                   wait=True)
    PV(pv_square_wave_openloop[i]).put(0,        wait=True)
  time.sleep(1)
  print('>>> Set initial values for gain, offset, PI Kp and PI Ti... Done!\n')

  print('>>> Set the period for 10ms...')

  PV(pv_dac_cnt_max).put(dac_cnt_max, wait=True)

  print('>>> Set the period for 10ms... Done!\n')

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
  print('------------------------- Calculate the offset ---------------------------')
  print('--------------------------------------------------------------------------\n')


  new_offset = np.zeros(channels)
  for i in range(0, channels):
    new_offset[i] = int(np.mean(PV(pv_current_ArrayDataRAW[i]).get()))

  print('New current offset values: \n', new_offset)

  print('\n>>> Set the new current offset for all channels...')

  for i in range(0, channels):
    PV(pv_current_offset[i]).put(new_offset[i])

  print('>>> Set the new current offset for all channels... Done!')

  print('\n--------------------------------------------------------------------------')
  print('----------------------- Calculate the resistance -------------------------')
  print('--------------------------------------------------------------------------\n')


  print('>>> Set 0.5V in DAC data for all channels...')

  PV(pv_dac_data_wb).put(1, wait=True)

  for i in range(0, channels):
    PV(pv_dac_data[i]).put(dac_data, wait=True)
    PV(pv_amp_enable[i]).put(1, wait=True)
    PV(pv_dac_write[i]).put(1, wait=True)

  print('>>> Set 0.5V in DAC data for all channels... Done!')

  print('\n>>> New acquisition...')

  # do an acquisition and stop the event,
  # so the array data will be the same until we do another acquisition

  PV(pv_acq_samples_pre).put(samples, wait=True)
  PV(pv_acq_trigger_rep).put(0,       wait=True)
  PV(pv_acq_trigger_event).put(0,     wait=True)
  time.sleep(1) # just to see the waveform change in graphical interface
  PV(pv_acq_trigger_event).put(1,     wait=True)


  print('>>> New acquisition... Done!')

  print('\n>>> Calculate the resistance for each channel...')

  resistance = np.zeros(channels)
  mean       = np.zeros(channels)

  for i in range(0, channels):
    resistance[i] = 0.5/np.mean(PV(pv_current_ArrayData[i]).get())
    mean[i]       = np.mean(PV(pv_current_ArrayData[i]).get())


  print('>>> Calculate the resistance for each channel... Done!')

  print('\n>> Resistance values: \n', resistance)
  print('\n>> Mean current values: \n', mean)

  print('\n>>> Disable DAC data for all channels...')

  PV(pv_dac_data_wb).put(0,  wait=True)
  for i in range(0, channels):
    PV(pv_amp_enable[i]).put(0, wait=True)
    PV(pv_dac_data[i]).put(0, wait=True)
    PV(pv_dac_write[i]).put(0, wait=True)

  print('>>> Disable DAC data for all channels... Done!')

  print('\n--------------------------------------------------------------------------')
  print('----------------------- Calculate the indutance -------------------------')
  print('--------------------------------------------------------------------------\n')

  err = 1
  while err >= 1:

    print('>>> Configure open loop square wave for +-2.5V...')

    for i in range(0, channels):
      PV(pv_amp_enable[i]).put(0,                  wait=True)
      PV(pv_pi_enable[i]).put(0,                   wait=True)
      PV(pv_square_wave_openloop[i]).put(0,        wait=True)

    time.sleep(0.6)
    PV(pv_current_setpoint_inf).put(-sp_data, wait=True)

    for i in range(0, channels):
      PV(pv_current_setpoint[i]).put(sp_data, wait=True)
      PV(pv_square_wave_openloop[i]).put(1,   wait=True)
      PV(pv_amp_enable[i]).put(          1,   wait=True)

    print('>>> Configure open loop square wave for +-2.5V... Done!')

    min_values_1         = np.zeros(channels)
    max_values_1         = np.zeros(channels)

    min_values_2         = np.zeros(channels)
    max_values_2         = np.zeros(channels)

    min_values_indexes_1 = np.zeros(channels)
    max_values_indexes_1 = np.zeros(channels)

    min_values_indexes_2 = np.zeros(channels)
    max_values_indexes_2 = np.zeros(channels)

    min_val             = np.zeros(channels)
    max_val             = np.zeros(channels)

    min_idx             = np.zeros(channels)
    max_idx             = np.zeros(channels)

    print('\n>>> New acquisition...')

    # do an acquisition and stop the event,
    # so the array data will be the same until we do another acquisition
    time.sleep(0.3)
    PV(pv_acq_samples_pre).put(samples, wait=True)
    PV(pv_acq_trigger_rep).put(0,       wait=True)
    PV(pv_acq_trigger_event).put(0,     wait=True)
    time.sleep(1) # just to see the waveform change in graphical interface
    PV(pv_acq_trigger_event).put(1,     wait=True)

    PV(pv_current_setpoint_inf).put(0,  wait=True)

    print('>>> New acquisition... Done!')

    print('\n>>> Find the two max and min values for each channel...')

    for i in range(0, channels):
      min_values_1[i]         = np.min(PV(pv_current_ArrayData[i]).get())
      max_values_1[i]         = np.max(PV(pv_current_ArrayData[i]).get())

      min_values_2[i]         = heapq.nsmallest(2, PV(pv_current_ArrayData[i]).get())[-1]
      max_values_2[i]         = heapq.nlargest(2, PV(pv_current_ArrayData[i]).get())[-1]

      min_values_indexes_1[i] = ((PV(pv_current_ArrayData[i]).get()).tolist()).index(min_values_1[i])
      max_values_indexes_1[i] = ((PV(pv_current_ArrayData[i]).get()).tolist()).index(max_values_1[i])

      min_values_indexes_2[i] = ((PV(pv_current_ArrayData[i]).get()).tolist()).index(min_values_2[i])
      max_values_indexes_2[i] = ((PV(pv_current_ArrayData[i]).get()).tolist()).index(max_values_2[i])

      # if the second min or max is not the peak value:
      a = 1
      if abs(min_values_indexes_1[i] - min_values_indexes_2[i]) < 200:
        min_values_2[i]       = heapq.nsmallest(2+a, PV(pv_current_ArrayData[i]).get())[-1]
        min_values_indexes_2[i] = ((PV(pv_current_ArrayData[i]).get()).tolist()).index(min_values_2[i])
        a = a + 1
      a = 1
      if abs(max_values_indexes_1[i] - max_values_indexes_2[i]) < 200:
        max_values_2[i]       = heapq.nlargest(2+a, PV(pv_current_ArrayData[i]).get())[-1]
        max_values_indexes_2[i] = ((PV(pv_current_ArrayData[i]).get()).tolist()).index(max_values_2[i])
        a = a + 1

    print('>>> Find the two max and min values for each channel... Done!')

    for i in range(0, channels):
      min_val[i]    = min_values_1[i]
      max_val[i]    = max_values_1[i]

      min_idx[i]    = min_values_indexes_1[i]
      max_idx[i]    = max_values_indexes_1[i]

      aux1          = max_values_2[i]
      aux2          = max_values_indexes_2[i]

      if min_idx[i] > min_values_indexes_2[i]:
        min_val[i]  = min_values_2[i]
        min_idx[i]  = min_values_indexes_2[i]

      if max_idx[i] > max_values_indexes_2[i]:
        max_val[i]  = max_values_2[i]
        max_idx[i]  = max_values_indexes_2[i]
        aux1        = max_values_1[i]
        aux2        = max_values_indexes_1[i]

      if max_idx[i] < min_idx[i]:
        max_val[i]  = aux1
        max_idx[i]  = aux2

    cnt_err = np.zeros(channels)
    err = 0
    for i in range(0, channels):
      if abs(max_idx[i]) - abs(np.mean(max_idx)) > 10:
        cnt_err[i] = 1

      elif abs(min_idx[i]) - abs(np.mean(min_idx)) > 10:
        cnt_err[i] = 1

      else:
        cnt_err[i] = 0

      err = err + cnt_err[i]

  print('\n>> Max current values indexes: \n', max_idx)

  print('\n>> Min current values indexes: \n', min_idx)

  print('\n>>> Calculate the inductance for each channel...')

  inductance = np.zeros(channels)

  for i in range(0, channels):
    inductance[i] = (resistance[i]*(max_idx[i]-min_idx[i])*1/fs)/(np.log(1-(resistance[i]*(max_val[i]-min_val[i]))/5))

  print('>>> Calculate the inductance for each channel... Done!')

  print('\n>> Inductance values: \n', inductance)

  for i in range(0, channels):
    PV(pv_amp_enable[i]).put(0,                  wait=True)
    PV(pv_pi_enable[i]).put(0,                   wait=True)
    PV(pv_square_wave_openloop[i]).put(0,        wait=True)

  results.write('\nInductance: ')
  results.write(str(inductance))
  results.write('\n\n')

  results.write('Resistance: ')
  results.write(str(resistance))
  results.write('\n\n')

  results.write('Mean current values: ')
  results.write(str(mean))
  results.write('\n\n')

  results.close()

  print('\n--------------------------------------------------------------------------')
  print('----------------------------------- END ----------------------------------')
  print('--------------------------------------------------------------------------\n')
