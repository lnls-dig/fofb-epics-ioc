#!/usr/bin/env python3

#
# Script to test RTM-LAMP
#
# Authors: Melissa Aguiar
#
# Created Feb. 25, 2022
#


# Importing libraries
import numpy as np
import matplotlib.pyplot as plt
from scipy import signal
from datetime import datetime
from epics import PV
import os
import warnings
import time
import json

warnings.filterwarnings('ignore')
plt.style.use("ggplot")

# Defining constants and PVs
fs                        = 1/940e-9
samples                   = 10000
channels                  = 12
current_gain              = 0.00006250   # 6.25e-5
voltage_gain              = 0.00011292   # 1.12916762036e-4
current_offset            = 0
voltage_offset            = 0
dac_data                  = 2
pv_prefix                 = "SI-22:DI-FOFBCtrl:"
setpoints_for_PSD         = [-1, -0.5, 0, 0.5, 1]
setpoints_for_cross_talk  = [1, 50e-3, 200e-3]
dac_data_values           = [3]          # decide the values and update here later
dac_cnt_max               = 625000       # 10ms
data                      = {}           # json data

# global PVs
pv_acq_trigger_rep        = str(pv_prefix) + str("ACQTriggerRep-Sel")
pv_acq_trigger_event      = str(pv_prefix) + str("ACQTriggerEvent-Sel")
pv_acq_samples_pre        = str(pv_prefix) + str("ACQSamplesPre-SP")
pv_current_setpoint_inf   = str(pv_prefix) + str("PSCurrLimInf-SP")
pv_dac_cnt_max            = str(pv_prefix) + str("PSPIDacCntMax-SP")
pv_dac_data_wb            = str(pv_prefix) + str("PSDacDataWb-SP")

# PVs per channel
pv_current_ArrayData      = []
pv_voltage_ArrayData      = []
pv_current_gain           = []
pv_voltage_gain           = []
pv_current_offset         = []
pv_voltage_offset         = []
pv_current_setpoint       = []
pv_current_setpoint_raw   = []
pv_dac_data               = []
pv_amp_enable             = []
pv_dac_write              = []

# getting arrays of PV names
for i in range(0, channels):
	pv_current_ArrayData.append(     str(pv_prefix)  + str("GEN_CH")             + str(i)          + str("ArrayData"))
	pv_voltage_ArrayData.append(     str(pv_prefix)  + str("GEN_CH")             + str(i+channels) + str("ArrayData"))
	pv_current_gain.append(          str(pv_prefix)  + str("PSGainWavCH")        + str(i)          + str("-SP.VAL"))
	pv_voltage_gain.append(          str(pv_prefix)  + str("PSGainWavCH")        + str(i+channels) + str("-SP.VAL"))
	pv_current_offset.append(        str(pv_prefix)  + str("PSOffsetWavCH")      + str(i)          + str("-SP.VAL"))
	pv_voltage_offset.append(        str(pv_prefix)  + str("PSOffsetWavCH")      + str(i+channels) + str("-SP.VAL"))
	pv_current_setpoint.append(      str(pv_prefix)  + str("PSCurrCH")           + str(i)          + str("-SP.VAL"))
	pv_current_setpoint_raw.append(  str(pv_prefix)  + str("PSCurrRawCH")        + str(i)          + str("-SP.VAL"))
	pv_dac_data.append(              str(pv_prefix)  + str("PSDacDataCH")        + str(i)          + str("-SP.VAL"))
	pv_amp_enable.append(            str(pv_prefix)  + str("PSAmpEnCH")          + str(i)          + str("-Sel.VAL"))
	pv_dac_write.append(             str(pv_prefix)  + str("PSDacWrCH")          + str(i)          + str("-SP.VAL"))

print('\n--------------------------------------------------------------------------')
print('------------------------------ STARTING TEST -----------------------------')
print('--------------------------------------------------------------------------\n')

print('------------ Get serial number and data/time ------------\n')

print('Insert the serial number:')
serial_number = input()

today = datetime.now()
now = today.strftime("%B %d, %Y. %H:%M:%S")

print('Serial number: ', str(serial_number))
print('Now: ', now)

data['Serial number'] = serial_number
data['Date and time of test'] = now

print('\n--------------------------------------------------------------------------')
print('------------------------------ ACQUIRE DATA ------------------------------')
print('--------------------------------------------------------------------------\n')

print('>>> Set current: gain = %f offset = %f \n Set voltage: gain = %f offset = %f ...'
      %(current_gain, current_offset, voltage_gain, voltage_offset))

# initializing some PVs
for i in range(0, channels):
	PV(pv_current_gain[i]).put(current_gain, wait=True)
	PV(pv_current_offset[i]).put(current_offset, wait=True)
	PV(pv_voltage_gain[i]).put(voltage_gain, wait=True)
	PV(pv_voltage_offset[i]).put(voltage_offset, wait=True)
#PV(pv_dac_data_wb[i]).put(0, wait=True)

print('>>> Set current/voltage gain and offset... Done!')

print('>>> Set the period for 10ms...')

PV(pv_dac_cnt_max).put(dac_cnt_max, wait=True)

print('>>> Set the period for 10ms... Done!')

print('>>> Set zero for the current setpoints...')
for i in range(0, channels):
		PV(pv_current_setpoint[i]).put(0, wait=True)
PV(pv_current_setpoint_inf).put(0, wait=True)
print('>>> Set zero for the current setpoints... Done!')

print('\n>>> New acquisition...')

# do an acquisition and stop the event,
#so the array data will be the same until we do another acquisition
PV(pv_acq_samples_pre).put(samples, wait=True)
PV(pv_acq_trigger_rep).put(0, wait=True)
PV(pv_acq_trigger_event).put(0, wait=True)
time.sleep(0.1)
PV(pv_acq_trigger_event).put(1, wait=True)

print('>>> New acquisition... Done!\n')

print('\n--------------------------------------------------------------------------')
print('----------------------------- OPEN LOOP TEST -----------------------------')
print('--------------------------------------------------------------------------\n')

print('\n------------ Calculate the current offset ------------\n')

new_offset = np.zeros(channels)
for i in range(0, channels):
	new_offset[i] = np.mean(PV(pv_current_ArrayData[i]).get()*current_gain)
print('New current offset values: ', new_offset)

data['New current offsets'] = new_offset.tolist()

data_open_loop = [[], [], [], [], [], [], [], [], [], [], [], []]

for i in range(0, channels):
  data_open_loop[i] = (PV(pv_current_ArrayData[i]).get()*current_gain).tolist()

data['ACQ for Open Loop Test'] = data_open_loop

print('\n------------ Calculate the PSD ------------\n')

os.system("mkdir Results")
os.system("mkdir Results/Serial_Number_%s"%(serial_number))
os.system("mkdir Results/Serial_Number_%s/ACQ_Data_OpenLoop"%(serial_number))

print('\n>>> Plot PSD for all channels and save figures...')

for i in range(0, channels):
  plt.figure(figsize=[14, 8])

  f, psd = signal.periodogram(data_open_loop[i], fs)

  plt.loglog(f, np.sqrt(psd), 'b',
             linewidth=1, marker='h', markerfacecolor='lightgreen',
             markeredgewidth=1,markersize=4)

  plt.ylim([1e-10, 1e-3])
  plt.xlim([0, 1e5])
  plt.xlabel('Frequency [Hz]')
  plt.ylabel('PSD [A/$\sqrt{Hz}$]')
  plt.title('Open Loop | Power Spectral Density CH' + str(i) + ' [Serial Number: ' + str(serial_number) + ']')
  plt.grid(True, which="both")

  plt.savefig('Results/Serial_Number_%s/ACQ_Data_OpenLoop/%s_PSD_OpenLoop_CH%d.png' %(serial_number, serial_number, i))

print('>>> Plot PSD for all channels and save figures... Done!')

print('\n------------ Read DAC values for each channel ------------\n')

dac_voltage = np.zeros(channels)

print('\n>>> Enable DAC data from Wb')
PV(pv_dac_data_wb).put(1, wait=True)

for val in dac_data_values:
	for i in range(0, channels):
		print('\n------------ CHANNEL %d ------------\n'%(i))

		print('\n>>> Set %f in DAC data for channel %d'%(val, i))

		PV(pv_dac_data[i]).put(val, wait=True)

		print('\n>>> Enable AmpEn for channel %d'%(i))

		PV(pv_amp_enable[i]).put(1, wait=True)

		print('\n>>> Enable DAC write for channel %d'%(i))

		PV(pv_dac_write[i]).put(1, wait=True)

		print('\n>>> Insert DAC voltage readed from multimeter for CH%d: '%(i))

		dac_voltage[i] = float(input())

		print('dac_voltage[%d] = %f'%(i, dac_voltage[i]))

		print('\n>>> Disable DAC write for channel %d'%(i))

		PV(pv_dac_write[i]).put(0, wait=True)

		print('\n>>> Disable AmpEn for channel %d'%(i))

		PV(pv_amp_enable[i]).put(0, wait=True)

		print('\n>>> Set 0 in DAC data for channel %d'%(i))

		PV(pv_dac_data[i]).put(0, wait=True)

print('\n>>> Disable DAC data from Wb')

PV(pv_dac_data_wb).put(0, wait=True)
PV(pv_dac_write[i]).put(0, wait=True)

data['DAC voltage readed'] = dac_voltage.tolist()

print('\n--------------------------------------------------------------------------')
print('---------------------------- CLOSED LOOP TEST ----------------------------')
print('--------------------------------------------------------------------------\n')

print('>>> Set the new current offset for all channels...')

for i in range(0, channels):
	PV(pv_current_offset[i]).put(new_offset[i])

print('>>> Set the new current offset for all channels... Done!')

data_closed_loop      = [[], [], [], [], [], [], [], [], [], [], [], []]
data_closed_loop_full = [[], [], [], [], []]

a = 0
for sp in setpoints_for_PSD:
	os.system("mkdir Results/Serial_Number_%s/ACQ_Data_ClosedLoop_sp%a"%(serial_number, a))
	for i in range(0, channels):
		PV(pv_current_setpoint[i]).put(sp, wait=True)

	print('\n------------ Calculate the PSD for SP = %fA ------------\n'%(sp))

	print('>>> New acquisition...')

	# do an acquisition and stop the event,
	#so the array data will be the same until we do another acquisition
	PV(pv_acq_samples_pre).put(samples, wait=True)
	PV(pv_acq_trigger_rep).put(0, wait=True)
	PV(pv_acq_trigger_event).put(0, wait=True)
	time.sleep(0.1)
	PV(pv_acq_trigger_event).put(1, wait=True)

	print('>>> New acquisition... Done!\n')

	print('>>> Plot PSD for all channels and save figures...')

	for j in range(0, channels):
		plt.figure(figsize=[14, 8])

		f, psd = signal.periodogram(PV(pv_current_ArrayData[j]).get()*current_gain, fs)

		plt.loglog(f, np.sqrt(psd), 'b',
				       linewidth=1, marker='h', markerfacecolor='lightgreen',
				       markeredgewidth=1,markersize=4)

		plt.ylim([1e-10, 1e-3])
		plt.xlim([0, 1e5])
		plt.xlabel('Frequency [Hz]')
		plt.ylabel('PSD [A/$\sqrt{Hz}$]')
		plt.title('Closed Loop | Power Spectral Density CH' + str(j) + ' Setpoint = ' + str(sp) + 'A [Serial Number: ' + str(serial_number) + ']')
		plt.grid(True, which="both")

		plt.savefig('Results/Serial_Number_%s/ACQ_Data_ClosedLoop_sp%d/%s_PSD_ClosedLoop_CH%d.png' %(serial_number, a, serial_number, j))

		data_closed_loop[j] = (PV(pv_current_ArrayData[j]).get()*current_gain).tolist()

	data_closed_loop_full[a] = data_closed_loop

	print('>>> Plot PSD for all channels and save figures... Done!')
	a = a + 1

data['ACQ for Closed Loop Test'] = data_closed_loop_full

print('\n--------------------------------------------------------------------------')
print('--------------------- CLOSED LOOP TEST - SQUARE WAVE ---------------------')
print('--------------------------------------------------------------------------\n')

a = 0
data_cross_talk        = [[], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], []]
data_cross_talk_full   = [[], [], [], [], [], [], [], [], [], [], [], []]
data_cross_talk_per_sp = [[], [], []]

print('>>> Set the current setpoint limits in zero for all channels...')

for i in range(0, channels):
	PV(pv_current_setpoint[i]).put(0, wait=True)
	PV(pv_current_setpoint_inf).put(0, wait=True)

print('>>> Set the current setpoint limits in zero for all channels... Done!')

n = 0

for sp in setpoints_for_cross_talk:

	for i in range(0, channels):
		print('\n---- CHANNEL %d with SP = %fA and others with SP = 0 ----\n'%(i, sp))

		print('\n>>> Set the limits for the square wave...')

		PV(pv_current_setpoint[i]).put(sp, wait=True)
		PV(pv_current_setpoint_inf).put(-1*PV(pv_current_setpoint_raw[i]).get(), wait=True)

		print('\n>>> Set the limits for the square wave... Done!')

		print('\n>>> New acquisition...')

		# do an acquisition and stop the event,
		#so the array data will be the same until we do another acquisition
		PV(pv_acq_samples_pre).put(samples, wait=True)
		PV(pv_acq_trigger_rep).put(0, wait=True)
		PV(pv_acq_trigger_event).put(0, wait=True)
		time.sleep(0.1)
		PV(pv_acq_trigger_event).put(1, wait=True)

		print('>>> New acquisition... Done!\n')

		for j in range(0, channels):
			data_cross_talk[j] = (PV(pv_current_ArrayData[j]).get()*current_gain).tolist()
			data_cross_talk[j+channels] = (PV(pv_voltage_ArrayData[j]).get()*voltage_gain).tolist()

		data_cross_talk_full[i] = data_cross_talk

	data_cross_talk_per_sp[n] = data_cross_talk_full
	n = n+1

print('\n-------------------------------------------------------------\n')

data['ACQ for Cross Talk'] = data_cross_talk_per_sp

json_data = json.dumps(data, indent = 4)

print('>>> Save data in json file... Done!\n')

# Writing to json file
with open("Results/Serial_Number_%s/data_%s.json"%(serial_number, serial_number), "w") as outfile:
		outfile.write(json_data)

print('>>> Save data in json file... Done!\n')

#with open("Results/Serial_Number_%s/data_%s.json"%(serial_number, serial_number), "r") as read_file:
#    data = json.load(read_file)
