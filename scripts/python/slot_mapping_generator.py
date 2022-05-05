f1 = open("slot-mapping.txt", "w+")

crates = 22
slots = 24
ioc_name = "FOFB"

pv_area_prefix = "IA"
pv_area_prefix_others = "XX"
pv_device_prefix = "RaBPM:BS"
pv_name_prefix = "FOFBCtrl"

for i in range(1, crates+1, 1):
    f1.write('\n' + "# --- CRATE " + str(i) + " ---" + '\n' + '\n')
    for j in range(1, slots+1, 1):
      if (j == 3 or j == 5):
        if i < 10:
          f1.write("# Crate 0" + str(i) + " - " + ioc_name + " slot " + str(j) + " - " + pv_area_prefix + "-0" + str(i) + pv_device_prefix + "-" + pv_name_prefix + '\n')
          f1.write("CRATE_0" + str(i) + "_" + ioc_name + "_" + str(j) + "_PV_AREA_PREFIX=" + pv_area_prefix + "-0" + str(i) + '\n')
          f1.write("CRATE_0" + str(i) + "_" + ioc_name + "_" + str(j) + "_PV_DEVICE_PREFIX=" + pv_device_prefix + "-" + pv_name_prefix + ":" + '\n'+ '\n')
        else:
          f1.write("# Crate " + str(i) + " - " + ioc_name + " slot " + str(j) + " - " + pv_area_prefix + "-" + str(i) + pv_device_prefix + "-" + pv_name_prefix + '\n')
          f1.write("CRATE_" + str(i) + "_" + ioc_name + "_" + str(j) + "_PV_AREA_PREFIX=" + pv_area_prefix + "-" + str(i) + '\n')
          f1.write("CRATE_" + str(i) + "_" + ioc_name + "_" + str(j) + "_PV_DEVICE_PREFIX=" + pv_device_prefix + "-" + pv_name_prefix + ":" + '\n'+ '\n')
      else:
        if i < 10:
          if j < 10:
            f1.write("# Crate 0" + str(i) + " - " + ioc_name + " slot " + str(j) + " - " + pv_area_prefix_others + "-0" + str(i) + "SL0" + str(j) + pv_device_prefix + "-" + pv_name_prefix + '\n')
            f1.write("CRATE_0" + str(i) + "_" + ioc_name + "_" + str(j) + "_PV_AREA_PREFIX=" + pv_area_prefix_others + "-0" + str(i) + "SL0" + str(j) + '\n')
            f1.write("CRATE_0" + str(i) + "_" + ioc_name + "_" + str(j) + "_PV_DEVICE_PREFIX=" + pv_device_prefix + "-" + pv_name_prefix + ":" + '\n'+ '\n')
          else:
            f1.write("# Crate 0" + str(i) + " - " + ioc_name + " slot " + str(j) + " - " + pv_area_prefix_others + "-0" + str(i) + "SL" + str(j) + pv_device_prefix + "-" + pv_name_prefix + '\n')
            f1.write("CRATE_0" + str(i) + "_" + ioc_name + "_" + str(j) + "_PV_AREA_PREFIX=" + pv_area_prefix_others + "-0" + str(i) + "SL" + str(j) + '\n')
            f1.write("CRATE_0" + str(i) + "_" + ioc_name + "_" + str(j) + "_PV_DEVICE_PREFIX=" + pv_device_prefix + "-" + pv_name_prefix + ":" + '\n'+ '\n')
        else:
          if j < 10:
            f1.write("# Crate " + str(i) + " - " + ioc_name + " slot " + str(j) + " - " + pv_area_prefix_others + "-" + str(i) + "SL0" + str(j) + pv_device_prefix + "-" + pv_name_prefix + '\n')
            f1.write("CRATE_" + str(i) + "_" + ioc_name + "_" + str(j) + "_PV_AREA_PREFIX=" + pv_area_prefix_others + "-" + str(i) + "SL0" + str(j) + '\n')
            f1.write("CRATE_" + str(i) + "_" + ioc_name + "_" + str(j) + "_PV_DEVICE_PREFIX=" + pv_device_prefix + "-" + pv_name_prefix + ":" + '\n'+ '\n')
          else:
            f1.write("# Crate " + str(i) + " - " + ioc_name + " slot " + str(j) + " - " + pv_area_prefix_others + "-" + str(i) + "SL" + str(j) + pv_device_prefix + "-" + pv_name_prefix + '\n')
            f1.write("CRATE_" + str(i) + "_" + ioc_name + "_" + str(j) + "_PV_AREA_PREFIX=" + pv_area_prefix_others + "-" + str(i) + "SL" + str(j) + '\n')
            f1.write("CRATE_" + str(i) + "_" + ioc_name + "_" + str(j) + "_PV_DEVICE_PREFIX=" + pv_device_prefix + "-" + pv_name_prefix + ":" + '\n'+ '\n')

f1.write('\n' + "# --- CRATE 98 (devel) ---" + '\n' + '\n')
for j in range(1, slots+1, 1):
  i = 98
  if (j == 3 or j == 5):
    f1.write("# Crate " + str(i) + " - " + ioc_name + " slot " + str(j) + " - " + pv_area_prefix + "-" + str(i) + pv_device_prefix + "-" + pv_name_prefix + '\n')
    f1.write("CRATE_" + str(i) + "_" + ioc_name + "_" + str(j) + "_PV_AREA_PREFIX=" + pv_area_prefix + "-" + str(i) + '\n')
    f1.write("CRATE_" + str(i) + "_" + ioc_name + "_" + str(j) + "_PV_DEVICE_PREFIX=" + pv_device_prefix + "-" + pv_name_prefix + ":" + '\n'+ '\n')
  else:
    if j < 10:
      f1.write("# Crate " + str(i) + " - " + ioc_name + " slot " + str(j) + " - " + pv_area_prefix_others + "-" + str(i) + "SL0" + str(j) + pv_device_prefix + "-" + pv_name_prefix + '\n')
      f1.write("CRATE_" + str(i) + "_" + ioc_name + "_" + str(j) + "_PV_AREA_PREFIX=" + pv_area_prefix_others + "-" + str(i) + "SL0" + str(j) + '\n')
      f1.write("CRATE_" + str(i) + "_" + ioc_name + "_" + str(j) + "_PV_DEVICE_PREFIX=" + pv_device_prefix + "-" + pv_name_prefix + ":" + '\n'+ '\n')
    else:
      f1.write("# Crate " + str(i) + " - " + ioc_name + " slot " + str(j) + " - " + pv_area_prefix_others + "-" + str(i) + "SL" + str(j) + pv_device_prefix + "-" + pv_name_prefix + '\n')
      f1.write("CRATE_" + str(i) + "_" + ioc_name + "_" + str(j) + "_PV_AREA_PREFIX=" + pv_area_prefix_others + "-" + str(i) + "SL" + str(j) + '\n')
      f1.write("CRATE_" + str(i) + "_" + ioc_name + "_" + str(j) + "_PV_DEVICE_PREFIX=" + pv_device_prefix + "-" + pv_name_prefix + ":" + '\n'+ '\n')

f1.write('\n' + "# --- CRATE 99 (minicrate) ---" + '\n' + '\n')
for j in range(1, slots+1, 1):
  i = 99
  if (j == 3 or j == 5):
    f1.write("# Crate " + str(i) + " - " + ioc_name + " slot " + str(j) + " - " + pv_area_prefix + "-" + str(i) + pv_device_prefix + "-" + pv_name_prefix + '\n')
    f1.write("CRATE_" + str(i) + "_" + ioc_name + "_" + str(j) + "_PV_AREA_PREFIX=" + pv_area_prefix + "-" + str(i) + '\n')
    f1.write("CRATE_" + str(i) + "_" + ioc_name + "_" + str(j) + "_PV_DEVICE_PREFIX=" + pv_device_prefix + "-" + pv_name_prefix + ":" + '\n'+ '\n')
  else:
    if j < 10:
      f1.write("# Crate " + str(i) + " - " + ioc_name + " slot " + str(j) + " - " + pv_area_prefix_others + "-" + str(i) + "SL0" + str(j) + pv_device_prefix + "-" + pv_name_prefix + '\n')
      f1.write("CRATE_" + str(i) + "_" + ioc_name + "_" + str(j) + "_PV_AREA_PREFIX=" + pv_area_prefix_others + "-" + str(i) + "SL0" + str(j) + '\n')
      f1.write("CRATE_" + str(i) + "_" + ioc_name + "_" + str(j) + "_PV_DEVICE_PREFIX=" + pv_device_prefix + "-" + pv_name_prefix + ":" + '\n'+ '\n')
    else:
      f1.write("# Crate " + str(i) + " - " + ioc_name + " slot " + str(j) + " - " + pv_area_prefix_others + "-" + str(i) + "SL" + str(j) + pv_device_prefix + "-" + pv_name_prefix + '\n')
      f1.write("CRATE_" + str(i) + "_" + ioc_name + "_" + str(j) + "_PV_AREA_PREFIX=" + pv_area_prefix_others + "-" + str(i) + "SL" + str(j) + '\n')
      f1.write("CRATE_" + str(i) + "_" + ioc_name + "_" + str(j) + "_PV_DEVICE_PREFIX=" + pv_device_prefix + "-" + pv_name_prefix + ":" + '\n'+ '\n')

f1.close()
