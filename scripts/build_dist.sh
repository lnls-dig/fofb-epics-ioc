#!/usr/bin/env bash

USER=$(whoami)
IP="localhost"
PREFIX=$(pwd)
DIR="fofb-epics-ioc-deploy"
DIST_NAME="fofb-epics-ioc"

./deploy.sh ${USER} ${IP} ${PREFIX}/${DIR}
makeself --bzip2  --notemp ${PREFIX}/${DIR} ${DIST_NAME}.bz2.run "LNLS FOFB EPICS IOC Package" \
    make
