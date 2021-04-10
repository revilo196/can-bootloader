#!/bin/bash

SCRIPTPATH="$( cd "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
KSPATH=$(sed 's/\/scripts//g' <<< $SCRIPTPATH)
KSENV="${HOME}/can-bootloader-env"

echo "Creating virtual environment"
[ ! -d ${KSENV} ] && virtualenv -p /usr/bin/python3 ${KSENV}
cd  ${KSPATH}/client
${KSENV}/bin/pip install .
