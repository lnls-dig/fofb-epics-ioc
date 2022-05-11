  # # # # # # # # # # # # # # # # # # # # # # # # # #
 #                                                   #
#   Description: Script to test RTM-LAMP boards       #
#                                                     #
#   Author: Melissa Aguiar                            #
#                                                     #
#   Created: Feb. 25, 2022                            #
 #                                                   #
  # # # # # # # # # # # # # # # # # # # # # # # # # #

# importing libraries
import os
import sys
import time
import json
import warnings
import matplotlib.pyplot as plt
import numpy             as np
from   datetime      import datetime
from   scipy         import signal
from   epics         import PV

warnings.filterwarnings('ignore')                    # do not show plot/calc warnings
plt.style.use('ggplot')                              # plot style

try:
  json_file_in = int(sys.argv[1])                    # if argv = 1: read the input data from the old json file
except:                                              # if argv = None: the user need to input the new values
  json_file_in = None

# constants
fs                        = 1/940e-9                 # frequency for PSD calc
samples                   = 10000                    # number of samples for acquisition
samples_cross_talk        = 4000                     # number of samples to plot crosstalk data
channels                  = 12                       # number of channels (max 12, 8 actually in use)
pi_kp                     = 5000000                  # PI Kp parameter
pi_ti                     = 300                      # PI Ti parameter
crate_number              = "99"                     # crate number
slot_number               = "01"                     # slot number (must be equals to physical_slot*2-1)
current_gain              = 6.25e-5                  # initial value for current gain
voltage_gain              = 1.12916762036e-4         # initial value for voltage gain
current_offset            = 0                        # initial value for current offset
voltage_offset            = 0                        # initial value for voltage offset
setpoints_for_PSD         = [-1, -0.5, 0, 0.5, 1]    # current setpoints (A)
setpoints_for_cross_talk  = [1, 50e-3, 200e-3]       # current setpoints (A)
dac_data_values           = [0, 17712, -17712]       # 0V, 2V and -2V
dac_cnt_max               = 125000                   # 2ms
data                      = {}                       # json data

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
pv_current_setpoint_inf   = str(prefix_fofb) + str("TestCurrLowLim-SP")
pv_dac_cnt_max            = str(prefix_fofb) + str("TestPIDacCntMax-SP")
pv_dac_data_wb            = str(prefix_fofb) + str("DacDataSrc-Sel")

# PVs per channel
pv_current_ArrayData      = []
pv_voltage_ArrayData      = []
pv_current_ArrayDataRAW   = []
pv_current_gain           = []
pv_voltage_gain           = []
pv_current_offset         = []
pv_voltage_offset         = []
pv_current_setpoint       = []
pv_current_setpoint_raw   = []
pv_dac_data               = []
pv_amp_enable             = []
pv_pi_enable              = []
pv_dac_write              = []
pv_square_wave_enable     = []
pv_pi_kp                  = []
pv_pi_ti                  = []

# getting lists of PV names, so we can reutilize them in all tests
for i in range(0, channels):
  pv_current_ArrayData.append(     str(prefix_fofb)  + str("GENConvArrayDataCH") + str(i))
  pv_voltage_ArrayData.append(     str(prefix_fofb)  + str("GENConvArrayDataCH") + str(i+channels))
  pv_current_ArrayDataRAW.append(  str(prefix_fofb)  + str("GEN_CH")             + str(i)            + str("ArrayData"))

for pv_prefix in prefix_rtm:
  pv_current_gain.append(          str(pv_prefix)  + str("CurrGain")             + str("-SP"))
  pv_voltage_gain.append(          str(pv_prefix)  + str("VoltGain")             + str("-SP"))
  pv_current_offset.append(        str(pv_prefix)  + str("CurrOffset")           + str("-SP"))
  pv_voltage_offset.append(        str(pv_prefix)  + str("VoltOffset")           + str("-SP"))
  pv_current_setpoint.append(      str(pv_prefix)  + str("Current")              + str("-SP"))
  pv_current_setpoint_raw.append(  str(pv_prefix)  + str("CurrentRaw")           + str("-SP"))
  pv_dac_data.append(              str(pv_prefix)  + str("DacData")              + str("-SP"))
  pv_amp_enable.append(            str(pv_prefix)  + str("PwrState")             + str("-Sel"))
  pv_pi_enable.append(             str(pv_prefix)  + str("CtrlLoop")             + str("-Sel"))
  pv_dac_write.append(             str(pv_prefix)  + str("DacWr")                + str("-Cmd"))
  pv_square_wave_enable.append(    str(pv_prefix)  + str("TestClosedLoopSquare") + str("-Sel"))
  pv_pi_kp.append(                 str(pv_prefix)  + str("CtrlLoopKp")           + str("-SP"))
  pv_pi_ti.append(                 str(pv_prefix)  + str("CtrlLoopTi")           + str("-SP"))

print('\n')
print('         # # # # # # # # # # # # # # # # # # # # # # # # # #')
print('        #                                                   #')
print('       #   Description: Script to test RTM-LAMP boards       #')
print('       #                                                     #')
print('       #                                                     #')
print('       #   Created: Feb. 25, 2022                            #')
print('        #                                                   #')
print('         # # # # # # # # # # # # # # # # # # # # # # # # # #\n')

print('\n--------------------------------------------------------------------------')
print('------------------------------ STARTING TEST -----------------------------')
print('--------------------------------------------------------------------------\n')

print('------------------- Get serial number and data/time ----------------------\n')

print('Insert the serial number:')
serial_number = input()

today = datetime.now()
now   = today.strftime("%B %d, %Y. %H:%M:%S")

print('Serial number: ', str(serial_number))
print('Now: ', now)

data['Serial number']         = serial_number
data['Date and time of test'] = now

serial_str_title = serial_number
serial_str = list(serial_number)
serial_str[7] = "_"
serial_number = "".join(serial_str)

print('\n--------------------------------------------------------------------------')
print('------------------------------ ACQUIRE DATA ------------------------------')
print('--------------------------------------------------------------------------\n')

print('>>> Set initial values for gain, offset, PI Kp and PI Ti ...')

# initializing some PVs with default values
for i in range(0, channels):
  PV(pv_current_gain[i]).put(current_gain,     wait=True)
  PV(pv_current_offset[i]).put(current_offset, wait=True)
  PV(pv_voltage_gain[i]).put(voltage_gain,     wait=True)
  PV(pv_voltage_offset[i]).put(voltage_offset, wait=True)
  PV(pv_pi_kp[i]).put(pi_kp,                   wait=True)
  PV(pv_pi_ti[i]).put(pi_ti,                   wait=True)
  PV(pv_amp_enable[i]).put(0, wait=True)

PV(pv_dac_data_wb).put(0,                      wait=True)

print('>>> Set initial values for gain, offset, PI Kp and PI Ti... Done!\n')

print('>>> Set the period for 10ms...')

PV(pv_dac_cnt_max).put(dac_cnt_max, wait=True)

print('>>> Set the period for 10ms... Done!\n')

print('>>> Set zero for the current setpoints...')

for i in range(0, channels):
    PV(pv_current_setpoint[i]).put(0, wait=True)
PV(pv_current_setpoint_inf).put(0, wait=True)

print('>>> Set zero for the current setpoints... Done!')

print('\n>>> New acquisition...')

# do an acquisition and stop the event,
# so the array data will be the same until we do another acquisition
PV(pv_acq_samples_pre).put(samples, wait=True)
PV(pv_acq_trigger_rep).put(0,       wait=True)
PV(pv_acq_trigger_event).put(0,     wait=True)
time.sleep(0.5) # just to see the waveform change in graphical interface
PV(pv_acq_trigger_event).put(1,     wait=True)

print('>>> New acquisition... Done!\n')

print('\n--------------------------------------------------------------------------')
print('----------------------------- OPEN LOOP TEST -----------------------------')
print('--------------------------------------------------------------------------\n')

print('--------------------- Calculate the current offset -----------------------\n')

new_offset = np.zeros(channels)
for i in range(0, channels):
  new_offset[i] = int(np.mean(PV(pv_current_ArrayDataRAW[i]).get()))

print('New current offset values: \n', new_offset)

data['New current offsets'] = new_offset.tolist()

data_open_loop = [[], [], [], [], [], [], [], [], [], [], [], []]

for i in range(0, channels):
  data_open_loop[i] = (PV(pv_current_ArrayData[i]).get()).tolist()

data['ACQ for Open Loop Test'] = data_open_loop

print('\n------------------------- Calculate the PSD ------------------------------\n')

os.system("mkdir Results")
os.system("mkdir Results/%s"%(serial_number))
os.system("mkdir Results/%s/psd_open_loop"%(serial_number))

print('\n>>> Plot PSD for all channels and save figures...')

for i in range(0, channels):
  plt.figure(figsize=[14, 8])

  f, psd = signal.periodogram(data_open_loop[i], fs)

  plt.loglog(f, np.sqrt(psd), 'b',
             linewidth=1, marker='h', markerfacecolor='lightgreen',
             markeredgewidth=1, markersize=4)

  plt.ylim([1e-10, 1e-3])
  plt.xlim([0, 1e5])
  plt.xlabel('Frequency [Hz]')
  plt.ylabel('PSD [A/$\sqrt{Hz}$]')
  plt.title('Open Loop | Power Spectral Density CH' + str(i) + ' [Serial Number: ' + str(serial_str_title) + ']')
  plt.grid(True, which="both")

  plt.savefig('Results/%s/psd_open_loop/psd_ch%02d.png' %(serial_number, i))

print('>>> Plot PSD for all channels and save figures... Done!')

if json_file_in == 1:
  # reading data from json file
  with open("Results/old/%s/data_%s.json"%(serial_str_title, serial_str_title), "r") as read_file:
    data_json_file_in = json.load(read_file)
    data['DAC voltage read'] = data_json_file_in['DAC voltage read']

if json_file_in == None:
  print('\n------------------ Read DAC values for each channel ----------------------\n')

  dac_voltage = np.zeros(np.size(dac_data_values))
  data_dac_voltage = [[], [], [], [], [], [], [], [], [], [], [], []]

  print('>>> Enable DAC data from Wb')

  PV(pv_dac_data_wb).put(1, wait=True)

  a = 0
  for i in range(0, channels):
    n = 0
    for val in dac_data_values:
      print('\n------------------------------- CHANNEL %02d --------------------------------\n'%(i))

      print('>>> Set %d in DAC data for channel %02d'%(val, i))

      PV(pv_dac_data[i]).put(val, wait=True)

      print('\n>>> Enable AmpEn for channel %02d'%(i))

      PV(pv_amp_enable[i]).put(1, wait=True)

      print('\n>>> Enable DAC write for channel %02d'%(i))

      PV(pv_dac_write[i]).put(1, wait=True)

      print('\n>>> Insert DAC voltage read from multimeter for CH%02d: '%(i))

      dac_voltage[n] = float(input())

      print('\n dac_voltage = %fV'%(dac_voltage[n]))

      print('\n>>> Disable DAC write for channel %02d'%(i))

      PV(pv_dac_write[i]).put(0, wait=True)

      print('\n>>> Disable AmpEn for channel %02d'%(i))

      PV(pv_amp_enable[i]).put(0, wait=True)

      print('\n>>> Set 0 in DAC data for channel %02d'%(i))

      PV(pv_dac_data[i]).put(0, wait=True)

      n = n + 1

    data_dac_voltage[a] = dac_voltage.tolist()
    a = a + 1

  print('\n>>> Disable DAC data from Wb')

  PV(pv_dac_data_wb).put(0,  wait=True)
  PV(pv_dac_write[i]).put(0, wait=True)

  data['DAC voltage read'] = data_dac_voltage

print('\n--------------------------------------------------------------------------')
print('---------------------------- CLOSED LOOP TEST ----------------------------')
print('--------------------------------------------------------------------------\n')

channels0_3  = [0, 1,  2,  3]
channels4_7  = [4, 5,  6,  7]
channels8_11 = [8, 9, 10, 11]

print('>>> Set the new current offset for all channels...')

for i in range(0, channels):
  PV(pv_current_offset[i]).put(new_offset[i])

print('>>> Set the new current offset for all channels... Done!')

data_closed_loop       = [[], [], [], [], [], [], [], [], [], [], [], []]
data_closed_loop_full  = [[], [], [], [], []]

data_cross_talk        = [[], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], []]
data_cross_talk_full   = [[], [], [], [], [], [], [], [], [], [], [], []]
data_cross_talk_per_sp = [[], [], []]

val_full0_3            = [[[], [], [], []], [[], [], [], []], [[], [], [], []], [[], [], [], []], [[], [], [], []]]
val_full4_7            = [[[], [], [], []], [[], [], [], []], [[], [], [], []], [[], [], [], []], [[], [], [], []]]
val_full8_11           = [[[], [], [], []], [[], [], [], []], [[], [], [], []], [[], [], [], []], [[], [], [], []]]

print('\n--------------------------------------------------------------------------')
print('-------------------- CLOSED LOOP TEST - CHANNELS 0-3 ---------------------')
print('--------------------------------------------------------------------------\n')

print('\n------------------ Connect the loads and press ENTER -------------------\n')
input()

a = 0
for sp in setpoints_for_PSD:
  os.system("mkdir Results/%s/psd_closed_loop_sp%sA"%(serial_number, str(sp)))

  for i in range(0, channels):
    PV(pv_current_setpoint[i]).put(sp, wait=True)
    PV(pv_pi_enable[i]).put(1, wait=True)
    PV(pv_amp_enable[i]).put(1, wait=True)

  print('\n----------------- Calculate the PSD for SP = %fA ------------------\n'%(sp))

  print('>>> New acquisition...')

  # do an acquisition and stop the event,
  #so the array data will be the same until we do another acquisition
  PV(pv_acq_samples_pre).put(samples, wait=True)
  PV(pv_acq_trigger_rep).put(0,       wait=True)
  PV(pv_acq_trigger_event).put(0,     wait=True)
  time.sleep(0.5)  # just to see the waveform change in graphical interface
  PV(pv_acq_trigger_event).put(1,     wait=True)

  print('>>> New acquisition... Done!\n')

  print('>>> Plot PSD and save figures...')

  for j in channels0_3:
    plt.figure(figsize=[14, 8])

    f, psd = signal.periodogram(PV(pv_current_ArrayData[j]).get(), fs)

    plt.loglog(f, np.sqrt(psd), 'b',
               linewidth=1, marker='h', markerfacecolor='lightgreen',
               markeredgewidth=1, markersize=4)

    plt.ylim([1e-10, 1e-3])
    plt.xlim([0, 1e5])
    plt.xlabel('Frequency [Hz]')
    plt.ylabel('PSD [A/$\sqrt{Hz}$]')
    plt.title('Closed Loop | Power Spectral Density CH' + str(j) + ' Setpoint = ' + str(sp) + 'A [Serial Number: ' + str(serial_str_title) + ']')
    plt.grid(True, which="both")

    plt.savefig('Results/%s/psd_closed_loop_sp%sA/psd_ch%02d.png' %(serial_number, str(sp), j))

    data_closed_loop[j] = (PV(pv_current_ArrayData[j]).get()).tolist()

    val_full0_3[a][j] = np.mean(data_closed_loop[j])

  data_closed_loop_full[a] = data_closed_loop

  print('>>> Plot PSD and save figures... Done!')

  a = a + 1

print('\n>>> Set the current setpoint limits in zero for all channels...')

for i in range(0, channels):
  PV(pv_current_setpoint[i]).put(0,  wait=True)
  PV(pv_current_setpoint_inf).put(0, wait=True)
  PV(pv_pi_enable[i]).put(0,         wait=True)
  PV(pv_amp_enable[i]).put(0,        wait=True)

print('>>> Set the current setpoint limits in zero for all channels... Done!')

n = 0
for sp in setpoints_for_cross_talk:
  os.system("mkdir Results/%s/crosstalk_sp%sA"%(serial_number, str(sp)))

  for i in channels0_3:
    print('\n------------- Crosstalk CHANNEL %02d with SP = %fA --------------------\n'%(i, sp))

    print('\n>>> Set the limits for the square wave...')

    PV(pv_current_setpoint[i]).put(sp, wait=True)
    PV(pv_current_setpoint_inf).put(-1*PV(pv_current_setpoint_raw[i]).get(), wait=True)

    print('>>> Set the limits for the square wave... Done!')

    print('\n>>> Enable the square wave for channel %02d...'%(i))

    time.sleep(0.2)
    PV(pv_square_wave_enable[i]).put(1, wait=True)
    time.sleep(0.2)
    PV(pv_pi_enable[i]).put(1,          wait=True)
    time.sleep(0.2)
    PV(pv_amp_enable[i]).put(1,         wait=True)
    time.sleep(0.5)

    print('>>> Enable the square wave for channel %02d... Done!'%(i))

    print('\n>>> New acquisition...')

    # do an acquisition and stop the event,
    #so the array data will be the same until we do another acquisition
    PV(pv_acq_samples_pre).put(samples, wait=True)
    PV(pv_acq_trigger_rep).put(0,       wait=True)
    PV(pv_acq_trigger_event).put(0,     wait=True)
    time.sleep(0.5)  # just to see the waveform change in graphical interface
    PV(pv_acq_trigger_event).put(1,     wait=True)

    print('>>> New acquisition... Done!\n')

    print('>>> Save the acq data from all channels...')

    for j in range(0, channels):
      data_cross_talk[j]          = (PV(pv_current_ArrayData[j]).get()).tolist()
      data_cross_talk[j+channels] = (PV(pv_voltage_ArrayData[j]).get()
                                     ).tolist()

    data_cross_talk_full[i] = data_cross_talk

    print('>>> Save the acq data from all channels... Done!\n')

    print('>>> Disable the square wave and set SP = 0 for channel %02d...'%(i))

    PV(pv_square_wave_enable[i]).put(0, wait=True)
    PV(pv_current_setpoint[i]).put(0,   wait=True)
    PV(pv_pi_enable[i]).put(0,          wait=True)
    PV(pv_amp_enable[i]).put(0,         wait=True)
    time.sleep(0.5)

    print('>>> Disable the square wave and set SP = 0 for channel %02d... Done!\n'%(i))

    print('>>> Plot crosstalk data for all channels and save figures...')

    plt.figure(figsize=[40, 30])
    for j in range(0, channels):
      plt.subplot(4,3,j+1)
      y = data_cross_talk[j]
      plt.plot(y[0:samples_cross_talk])

      if j != i:
        plt.ylim([-3e-3, 3e-3])
      plt.xlabel('Sample')
      plt.ylabel('Current [A]')
      plt.title('Crosstalk SP = ' + str(sp) + 'A in Channel ' + str(i) + ' | ACQ from Channel ' + str(j))
      time.sleep(0.5)

    plt.savefig('Results/%s/crosstalk_sp%sA/current_ch%02d.png' %(serial_number, str(sp), i))

    plt.figure(figsize=[14, 8])
    y = data_cross_talk[i+channels]
    plt.plot(y[0:samples_cross_talk],'b')
    plt.xlabel('Sample')
    plt.ylabel('Voltage [V]')
    plt.title('Crosstalk SP = ' + str(sp) + 'A in Channel ' + str(i) + ' | ACQ from Channel ' + str(i))
    time.sleep(0.5)
    plt.savefig('Results/%s/crosstalk_sp%sA/voltage_ch%02d.png' %(serial_number, str(sp), i))

    print('>>> Plot crosstalk data for all channels and save figures... Done!\n')

  data_cross_talk_per_sp[n] = data_cross_talk_full

  n = n + 1

print('>>> Set zero for the current setpoint inferior limit...')

PV(pv_current_setpoint_inf).put(0, wait=True)

print('>>> Set zero for the current setpoint inferior limit... Done!')

print('\n--------------------------------------------------------------------------')
print('-------------------- CLOSED LOOP TEST - CHANNELS 4-7 ---------------------')
print('--------------------------------------------------------------------------\n')

print('\n------------------ Connect the loads and press ENTER -------------------\n')
input()

a = 0
for sp in setpoints_for_PSD:

  for i in range(0, channels):
    PV(pv_current_setpoint[i]).put(sp, wait=True)
    PV(pv_pi_enable[i]).put(1,         wait=True)
    PV(pv_amp_enable[i]).put(1,        wait=True)

  print('\n----------------- Calculate the PSD for SP = %fA ------------------\n'%(sp))

  print('>>> New acquisition...')

  # do an acquisition and stop the event,
  #so the array data will be the same until we do another acquisition
  PV(pv_acq_samples_pre).put(samples, wait=True)
  PV(pv_acq_trigger_rep).put(0,       wait=True)
  PV(pv_acq_trigger_event).put(0,     wait=True)
  time.sleep(0.5)  # just to see the waveform change in graphical interface
  PV(pv_acq_trigger_event).put(1,     wait=True)

  print('>>> New acquisition... Done!\n')

  print('>>> Plot PSD and save figures...')

  for j in channels4_7:
    plt.figure(figsize=[14, 8])

    f, psd = signal.periodogram(PV(pv_current_ArrayData[j]).get(), fs)

    plt.loglog(f, np.sqrt(psd), 'b',
               linewidth=1, marker='h', markerfacecolor='lightgreen',
               markeredgewidth=1, markersize=4)

    plt.ylim([1e-10, 1e-3])
    plt.xlim([0, 1e5])
    plt.xlabel('Frequency [Hz]')
    plt.ylabel('PSD [A/$\sqrt{Hz}$]')
    plt.title('Closed Loop | Power Spectral Density CH' + str(j) + ' Setpoint = ' + str(sp) + 'A [Serial Number: ' + str(serial_str_title) + ']')
    plt.grid(True, which="both")

    plt.savefig('Results/%s/psd_closed_loop_sp%sA/psd_ch%02d.png' %(serial_number, str(sp), j))

    data_closed_loop[j] = (PV(pv_current_ArrayData[j]).get()).tolist()

    val_full4_7[a][j-4] = np.mean(data_closed_loop[j])

  data_closed_loop_full[a] = data_closed_loop

  print('>>> Plot PSD and save figures... Done!')

  a = a + 1

print('\n>>> Set the current setpoint limits in zero for all channels...')

for i in range(0, channels):
  PV(pv_current_setpoint[i]).put(0,  wait=True)
  PV(pv_current_setpoint_inf).put(0, wait=True)
  PV(pv_pi_enable[i]).put(0,         wait=True)
  PV(pv_amp_enable[i]).put(0,        wait=True)

print('>>> Set the current setpoint limits in zero for all channels... Done!')

n = 0
for sp in setpoints_for_cross_talk:
  for i in channels4_7:
    print('\n------------- Crosstalk CHANNEL %02d with SP = %fA --------------------\n'%(i, sp))

    print('\n>>> Set the limits for the square wave...')

    PV(pv_current_setpoint[i]).put(sp, wait=True)
    PV(pv_current_setpoint_inf).put(-1*PV(pv_current_setpoint_raw[i]).get(), wait=True)

    print('>>> Set the limits for the square wave... Done!')

    print('\n>>> Enable the square wave for channel %02d...'%(i))

    time.sleep(0.2)
    PV(pv_square_wave_enable[i]).put(1, wait=True)
    time.sleep(0.2)
    PV(pv_pi_enable[i]).put(1,          wait=True)
    time.sleep(0.2)
    PV(pv_amp_enable[i]).put(1,         wait=True)
    time.sleep(0.5)

    print('>>> Enable the square wave for channel %02d... Done!'%(i))

    print('\n>>> New acquisition...')

    # do an acquisition and stop the event,
    #so the array data will be the same until we do another acquisition
    PV(pv_acq_samples_pre).put(samples, wait=True)
    PV(pv_acq_trigger_rep).put(0,       wait=True)
    PV(pv_acq_trigger_event).put(0,     wait=True)
    time.sleep(0.5)  # just to see the waveform change in graphical interface
    PV(pv_acq_trigger_event).put(1,     wait=True)

    print('>>> New acquisition... Done!\n')

    print('>>> Save the acq data from all channels...')

    for j in range(0, channels):
      data_cross_talk[j]          = (PV(pv_current_ArrayData[j]).get()).tolist()
      data_cross_talk[j+channels] = (PV(pv_voltage_ArrayData[j]).get()).tolist()

    data_cross_talk_full[i] = data_cross_talk

    print('>>> Save the acq data from all channels... Done!\n')

    print('>>> Disable the square wave and set SP = 0 for channel %02d...'%(i))

    PV(pv_square_wave_enable[i]).put(0, wait=True)
    PV(pv_current_setpoint[i]).put(0,   wait=True)
    PV(pv_pi_enable[i]).put(0,          wait=True)
    PV(pv_amp_enable[i]).put(0,         wait=True)
    time.sleep(0.5)

    print('>>> Disable the square wave and set SP = 0 for channel %02d... Done!\n'%(i))

    print('>>> Plot crosstalk data for all channels and save figures...')

    plt.figure(figsize=[40, 30])
    for j in range(0, channels):
      plt.subplot(4,3,j+1)
      y = data_cross_talk[j]
      plt.plot(y[0:samples_cross_talk])

      if j != i:
        plt.ylim([-3e-3, 3e-3])
      plt.xlabel('Sample')
      plt.ylabel('Current [A]')
      plt.title('Crosstalk SP = ' + str(sp) + 'A in Channel ' + str(i) + ' | ACQ from Channel ' + str(j))
      time.sleep(0.5)

    plt.savefig('Results/%s/crosstalk_sp%sA/current_ch%02d.png' %(serial_number, str(sp), i))

    plt.figure(figsize=[14, 8])
    y = data_cross_talk[i+channels]
    plt.plot(y[0:samples_cross_talk],'b')
    plt.xlabel('Sample')
    plt.ylabel('Voltage [V]')
    plt.title('Crosstalk SP = ' + str(sp) + 'A in Channel ' + str(i) + ' | ACQ from Channel ' + str(i))
    time.sleep(0.5)
    plt.savefig('Results/%s/crosstalk_sp%sA/voltage_ch%02d.png' %(serial_number, str(sp), i))

    print('>>> Plot crosstalk data for all channels and save figures... Done!\n')

  data_cross_talk_per_sp[n] = data_cross_talk_full

  n = n + 1

print('>>> Set zero for the current setpoint inferior limit...')

PV(pv_current_setpoint_inf).put(0, wait=True)

print('>>> Set zero for the current setpoint inferior limit... Done!')

print('\n--------------------------------------------------------------------------')
print('------------------- CLOSED LOOP TEST - CHANNELS 8-11 ---------------------')
print('--------------------------------------------------------------------------\n')

print('\n------------------ Connect the loads and press ENTER -------------------\n')
input()

a = 0
for sp in setpoints_for_PSD:

  for i in range(0, channels):
    PV(pv_current_setpoint[i]).put(sp, wait=True)
    PV(pv_pi_enable[i]).put(1,         wait=True)
    PV(pv_amp_enable[i]).put(1,        wait=True)

  print('\n----------------- Calculate the PSD for SP = %fA ------------------\n'%(sp))

  print('>>> New acquisition...')

  # do an acquisition and stop the event,
  #so the array data will be the same until we do another acquisition
  PV(pv_acq_samples_pre).put(samples, wait=True)
  PV(pv_acq_trigger_rep).put(0,       wait=True)
  PV(pv_acq_trigger_event).put(0,     wait=True)
  time.sleep(0.5)  # just to see the waveform change in graphical interface
  PV(pv_acq_trigger_event).put(1,     wait=True)

  print('>>> New acquisition... Done!\n')

  print('>>> Plot PSD and save figures...')

  for j in channels8_11:
    plt.figure(figsize=[14, 8])

    f, psd = signal.periodogram(PV(pv_current_ArrayData[j]).get(), fs)

    plt.loglog(f, np.sqrt(psd), 'b',
               linewidth=1, marker='h', markerfacecolor='lightgreen',
               markeredgewidth=1, markersize=4)

    plt.ylim([1e-10, 1e-3])
    plt.xlim([0, 1e5])
    plt.xlabel('Frequency [Hz]')
    plt.ylabel('PSD [A/$\sqrt{Hz}$]')
    plt.title('Closed Loop | Power Spectral Density CH' + str(j) + ' Setpoint = ' + str(sp) + 'A [Serial Number: ' + str(serial_str_title) + ']')
    plt.grid(True, which="both")

    plt.savefig('Results/%s/psd_closed_loop_sp%sA/psd_ch%02d.png' %(serial_number, str(sp), j))

    data_closed_loop[j] = (PV(pv_current_ArrayData[j]).get()).tolist()

    val_full8_11[a][j-8] = np.mean(data_closed_loop[j])

  data_closed_loop_full[a] = data_closed_loop

  print('>>> Plot PSD and save figures... Done!')

  a = a + 1

print('\n>>> Set the current setpoint limits in zero for all channels...')

for i in range(0, channels):
  PV(pv_current_setpoint[i]).put(0,  wait=True)
  PV(pv_current_setpoint_inf).put(0, wait=True)
  PV(pv_pi_enable[i]).put(0,         wait=True)
  PV(pv_amp_enable[i]).put(0,        wait=True)

print('>>> Set the current setpoint limits in zero for all channels... Done!')

n = 0
for sp in setpoints_for_cross_talk:
  for i in channels8_11:
    print('\n------------- Crosstalk CHANNEL %02d with SP = %fA --------------------\n'%(i, sp))

    print('\n>>> Set the limits for the square wave...')

    PV(pv_current_setpoint[i]).put(sp, wait=True)
    PV(pv_current_setpoint_inf).put(-1*PV(pv_current_setpoint_raw[i]).get(), wait=True)

    print('>>> Set the limits for the square wave... Done!')

    print('\n>>> Enable the square wave for channel %02d...'%(i))

    time.sleep(0.2)
    PV(pv_square_wave_enable[i]).put(1, wait=True)
    time.sleep(0.2)
    PV(pv_pi_enable[i]).put(1,          wait=True)
    time.sleep(0.2)
    PV(pv_amp_enable[i]).put(1,         wait=True)
    time.sleep(0.5)

    print('>>> Enable the square wave for channel %02d... Done!'%(i))

    print('\n>>> New acquisition...')

    # do an acquisition and stop the event,
    #so the array data will be the same until we do another acquisition
    PV(pv_acq_samples_pre).put(samples, wait=True)
    PV(pv_acq_trigger_rep).put(0,       wait=True)
    PV(pv_acq_trigger_event).put(0,     wait=True)
    time.sleep(0.5)  # just to see the waveform change in graphical interface
    PV(pv_acq_trigger_event).put(1,     wait=True)

    print('>>> New acquisition... Done!\n')

    print('>>> Save the acq data from all channels...')

    for j in range(0, channels):
      data_cross_talk[j]          = (PV(pv_current_ArrayData[j]).get()).tolist()
      data_cross_talk[j+channels] = (PV(pv_voltage_ArrayData[j]).get()).tolist()

    data_cross_talk_full[i] = data_cross_talk

    print('>>> Save the acq data from all channels... Done!\n')

    print('>>> Disable the square wave and set SP = 0 for channel %02d...'%(i))

    PV(pv_square_wave_enable[i]).put(0, wait=True)
    PV(pv_current_setpoint[i]).put(0,   wait=True)
    PV(pv_pi_enable[i]).put(0,          wait=True)
    PV(pv_amp_enable[i]).put(0,         wait=True)
    time.sleep(0.5)

    print('>>> Disable the square wave and set SP = 0 for channel %02d... Done!\n'%(i))

    print('>>> Plot crosstalk data for all channels and save figures...')

    plt.figure(figsize=[40, 30])
    for j in range(0, channels):
      plt.subplot(4,3,j+1)
      y = data_cross_talk[j]
      plt.plot(y[0:samples_cross_talk])

      if j != i:
        plt.ylim([-3e-3, 3e-3])
      plt.xlabel('Sample')
      plt.ylabel('Current [A]')
      plt.title('Crosstalk SP = ' + str(sp) + 'A in Channel ' + str(i) + ' | ACQ from Channel ' + str(j))
      time.sleep(0.5)

    plt.savefig('Results/%s/crosstalk_sp%sA/current_ch%02d.png' %(serial_number, str(sp), i))

    plt.figure(figsize=[14, 8])
    y = data_cross_talk[i+channels]
    plt.plot(y[0:samples_cross_talk],'b')
    plt.xlabel('Sample')
    plt.ylabel('Voltage [V]')
    plt.title('Crosstalk SP = ' + str(sp) + 'A in Channel ' + str(i) + ' | ACQ from Channel ' + str(i))
    time.sleep(0.5)
    plt.savefig('Results/%s/crosstalk_sp%sA/voltage_ch%02d.png' %(serial_number, str(sp), i))

    print('>>> Plot crosstalk data for all channels and save figures... Done!\n')

  data_cross_talk_per_sp[n] = data_cross_talk_full

  n = n + 1

print('>>> Set zero for the current setpoint inferior limit...')

PV(pv_current_setpoint_inf).put(0, wait=True)

print('>>> Set zero for the current setpoint inferior limit... Done!')

data['ACQ for Closed Loop Test'] = data_closed_loop_full

data['ACQ for Crosstalk'] = data_cross_talk_per_sp

print('\n---------------------------------------------------------------------------\n')

json_data = json.dumps(data, indent = 4)

print('>>> Save data in json file...')

# writing data to json file
with open("Results/%s/data_%s.json"%(serial_number, serial_number), "w") as outfile:
    outfile.write(json_data)

print('>>> Save data in json file... Done!\n')

a = 0
for sp in setpoints_for_PSD:
  print('\n>>> Setpoint =  %f'%(sp))

  if sp == 0:
    lim = 5e-3

  else:
    lim = 0.02*abs(sp)

  for i in channels0_3:
    if abs(abs(val_full0_3[a][i]) - abs(sp)) > lim:
      print('>>> FAIL!')
      print('>>> Mean value of channel %02d is equal to %f for setpoint %f'%(i, val_full0_3[a][i], sp))
  print(val_full0_3[a])

  for i in channels4_7:
    if abs(abs(val_full4_7[a][i-4]) - abs(sp)) > lim:
      print('>>> FAIL!')
      print('>>> Mean value of channel %02d is equal to %f for setpoint %f'%(i, val_full4_7[a][i-4], sp))
  print(val_full4_7[a])

  for i in channels8_11:
    if abs(abs(val_full8_11[a][i-8]) - abs(sp)) > lim:
      print('>>> FAIL!')
      print('>>> Mean value of channel %02d is equal to %f for setpoint %f'%(i, val_full8_11[a][i-8], sp))
  print(val_full8_11[a])

  a = a + 1

print('\n--------------------------------------------------------------------------')
print('----------------------------- END OF THE TEST ----------------------------')
print('--------------------------------------------------------------------------\n')

