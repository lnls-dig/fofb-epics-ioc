''' Script to check DCC FMC network integrity.
    Author: Guilherme Ricioli
'''

from epics import PV
from tabulate import tabulate

num_of_gts = 4
num_of_crates = 20
fofb_ctrl_offs = 480

pvs_suffix = ["DCCFMCLinkPartnerCH" + f"{gt}" + "-Mon" for gt in range(num_of_gts)]

integrity_res = []
for crate in range(1, num_of_crates + 1):
    bpm_id = fofb_ctrl_offs + crate - 1

    if crate == 1:      # 480
        anti_clockw_part_bpm_id = fofb_ctrl_offs + num_of_crates - 1
        clockw_part_bpm_id = bpm_id + 1
    elif crate == 20:   # 499
        anti_clockw_part_bpm_id = bpm_id - 1
        clockw_part_bpm_id = fofb_ctrl_offs
    else:               # 481 - 498
        anti_clockw_part_bpm_id = bpm_id - 1
        clockw_part_bpm_id = bpm_id + 1

    pv_prefix = "IA-" + f"{crate:02}" + "RaBPM:BS-FOFBCtrl:"

    # scanning link partners
    anti_clockw_part_found, clockw_part_found = False, False
    for pv_suffix in pvs_suffix:
        pv = PV(pv_prefix + pv_suffix)

        bpm_id_read = int(pv.get())
        if bpm_id_read == anti_clockw_part_bpm_id:
            anti_clockw_part_found = True
        elif bpm_id_read == clockw_part_bpm_id:
            clockw_part_found = True

        if anti_clockw_part_found and clockw_part_found:
            break

    # storing result
    integrity_res.append([crate, anti_clockw_part_found, clockw_part_found])

print(tabulate(integrity_res, headers=["Crate #", "Anti-clockw part.", "Clockw. part."], tablefmt="psql"))
