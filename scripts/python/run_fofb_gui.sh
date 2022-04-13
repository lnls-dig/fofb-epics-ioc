#!/usr/bin/env bash

pydm -m 'SECTION="XX", CRATE="99", SLOT="SL01"' --hide-nav-bar --hide-menu-bar --hide-status-bar fofb_gui.ui

# if the board is connected in physical slot 02 or 03, SECTION="IA" and SLOT=""
# else, we have SECTION="XX" and SLOT="SLn", where n=2*physical_slot-1 (n has two digits)
