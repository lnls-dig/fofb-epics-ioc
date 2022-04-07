  # # # # # # # # # # # # # # # # # # # # # # # # # #
 #                                                   #
#   Description: Script to test RTM-LAMP DAC          #
#                                                     #
#   Author: Melissa Aguiar                            #
#                                                     #
#   Created: Mar. 25, 2022                            #
 #                                                   #
  # # # # # # # # # # # # # # # # # # # # # # # # # #

# importing libraries
from epics import PV

# constants
channels                  = 12                       # number of channels (max 12, 8 actually in use)
pi_kp                     = 5000000                  # PI Kp parameter
pi_ti                     = 300                      # PI Ti parameter
crate_number              = "99"                     # crate number
slot_number               = "01"                     # slot number (must be equals to physical_slot*2-1)
dac_data_values           = [0, 17712, -17712]       # 0V, 2V and -2V
dac_cnt_max               = 125000                   # 2ms

# PV prefixes
if slot_number == ("03" or "05"):                    # board connected in physical slot 2 or 3
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
pv_dac_cnt_max = str(prefix_fofb) + str("PSPIDacCntMax-SP")
pv_dac_data_wb = str(prefix_fofb) + str("PSDacDataWb-SP")

# PVs per channel
pv_dac_data    = []
pv_amp_enable  = []
pv_pi_enable   = []
pv_dac_write   = []
pv_pi_kp       = []
pv_pi_ti       = []

# getting lists of PV names, so we can reutilize them in all tests
for pv_prefix in prefix_rtm:
  pv_dac_data.append(   str(pv_prefix) + str("DacData")    + str("-SP"))
  pv_amp_enable.append( str(pv_prefix) + str("PwrState")   + str("-Sel"))
  pv_pi_enable.append(  str(pv_prefix) + str("CtrlLoop")   + str("-Sel"))
  pv_dac_write.append(  str(pv_prefix) + str("DacWr")      + str("-Cmd"))
  pv_pi_kp.append(      str(pv_prefix) + str("CtrlLoopKp") + str("-SP"))
  pv_pi_ti.append(      str(pv_prefix) + str("CtrlLoopTi") + str("-SP"))
print('\n')
print('         # # # # # # # # # # # # # # # # # # # # # # # # # #')
print('        #                                                   #')
print('       #   Description: Script to test RTM-LAMP DAC          #')
print('       #                                                     #')
print('       #                                                     #')
print('       #   Created: Mar. 25, 2022                            #')
print('        #                                                   #')
print('         # # # # # # # # # # # # # # # # # # # # # # # # # #\n')

print('\n--------------------------------------------------------------------------')
print('------------------------------ STARTING TEST -----------------------------')
print('--------------------------------------------------------------------------\n')

print('>>> Set initial values for gain, offset, PI Kp and PI Ti ...')

# initializing some PVs with default values
for i in range(0, channels):
  PV(pv_pi_kp[i]).put(pi_kp,  wait=True)
  PV(pv_pi_ti[i]).put(pi_ti,  wait=True)
  PV(pv_amp_enable[i]).put(0, wait=True)

PV(pv_dac_data_wb).put(0,     wait=True)

print('>>> Set initial values for gain, offset, PI Kp and PI Ti... Done!\n')

print('>>> Set the period for 10ms...')

PV(pv_dac_cnt_max).put(dac_cnt_max, wait=True)

print('>>> Set the period for 10ms... Done!\n')


print('\n------------------ Read DAC values for each channel ----------------------\n')

print('>>> Enable DAC data from Wb')

PV(pv_dac_data_wb).put(1, wait=True)

a = ""
for i in range(0, channels):
  for val in dac_data_values:
    print('\n------------------------------- CHANNEL %02d --------------------------------\n'%(i))

    print('>>> Set %d in DAC data for channel %02d'%(val, i))

    PV(pv_dac_data[i]).put(val, wait=True)

    print('\n>>> Enable AmpEn for channel %02d'%(i))

    PV(pv_amp_enable[i]).put(1, wait=True)

    print('\n>>> Enable DAC write for channel %02d'%(i))

    PV(pv_dac_write[i]).put(1, wait=True)

    print('\n>>> Press ENTER: ')

    a = input()

    print('\n>>> Disable DAC write for channel %02d'%(i))

    PV(pv_dac_write[i]).put(0, wait=True)

    print('\n>>> Disable AmpEn for channel %02d'%(i))

    PV(pv_amp_enable[i]).put(0, wait=True)

    print('\n>>> Set 0 in DAC data for channel %02d'%(i))

    PV(pv_dac_data[i]).put(0, wait=True)

print('\n>>> Disable DAC data from Wb')

PV(pv_dac_data_wb).put(0,  wait=True)
PV(pv_dac_write[i]).put(0, wait=True)

print('\n--------------------------------------------------------------------------')
print('----------------------------- END OF THE TEST ----------------------------')
print('--------------------------------------------------------------------------\n')
