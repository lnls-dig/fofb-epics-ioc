#!/usr/bin/env bash

: ${EPICS_HOST_ARCH:?"Environment variable needs to be set"}

VALID_FOFB_NUMBER_STR="Valid values are between 0 and 23."

# Select endpoint.
FOFB_ENDPOINT=$1

if [ -z "$FOFB_ENDPOINT" ]; then
    echo "\"FOFB_ENDPOINT\" variable unset."
    exit 1
fi

# Select board in which we will work.
FOFB_NUMBER=$2

if [ -z "$FOFB_NUMBER" ]; then
    echo "\"FOFB_NUMBER\" variable unset. "$VALID_FOFB_NUMBERS_STR
    exit 1
fi

if [ "$FOFB_NUMBER" -lt 1 ] || [ "$FOFB_NUMBER" -gt 24 ]; then
    echo "Unsupported FOFB number. "$VALID_FOFB_NUMBERS_STR
    exit 1
fi

export FOFB_CURRENT_PV_AREA_PREFIX=CRATE_${EPICS_PV_CRATE_NUMBER}_FOFB_${FOFB_NUMBER}_PV_AREA_PREFIX
export FOFB_CURRENT_PV_DEVICE_PREFIX=CRATE_${EPICS_PV_CRATE_NUMBER}_FOFB_${FOFB_NUMBER}_PV_DEVICE_PREFIX
export EPICS_PV_AREA_PREFIX=${!FOFB_CURRENT_PV_AREA_PREFIX}
export EPICS_PV_DEVICE_PREFIX=${!FOFB_CURRENT_PV_DEVICE_PREFIX}

export EPICS_RTM_SECTION_PREFIX=SI-${EPICS_PV_CRATE_NUMBER}

FOFB_ENDPOINT=${FOFB_ENDPOINT} FOFB_NUMBER=${FOFB_NUMBER} ../../bin/${EPICS_HOST_ARCH}/FOFB stFOFB.cmd

