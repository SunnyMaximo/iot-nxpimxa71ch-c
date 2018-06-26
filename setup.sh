#!/bin/bash

# Do not run the script if setup is already done
if [ -f .setup_done ]
then
    echo "Setup is already done."
    exit 0
fi

# TEST: If the project is built on a system installed with "NXP i.MX Release Distro"
grep "NXP i.MX Release" /etc/os-release > /dev/null 2>&1
if [ $? -ne 0 ]
then
    echo
    echo "ERROR: Unsupported Platform Type. OS type is not NXP i.MX Release Distro"
    exit 1
fi

echo 
echo "Platform Details:"
cat /etc/os-release
echo

# TEST: If the setup script is invoked in subdirectory iot-nxpimxac71ch-c 
CURDIR=`pwd`
export CURDIR

echo ${CURDIR} | grep "/iot-nxpimxa71ch-c" > /dev/null 2>&1
if [ $? -ne 0 ]
then
    echo
    echo "ERROR: Invoked from wrong directory ${CURDIR}."
    echo "This script should be invoked from iot-nxpimxa71ch-c subdirectory"
    exit 1
fi

echo "Working directory: ${CURDIR}"
echo
    
# Create backup files and directories
if [ -f .build_done ]
then
    rm -rf backup > /dev/null 2>&1
    mkdir -p backup
    mv run backup/. > /dev/null 2>&1
    mv build backup/. > /dev/null 2>&1
    rm -f .build_done
fi

# Download dependent MQTT Paho C source
mkdir -p download 
cd download
if [ ! -f paho.mqtt.c-1.2.0.tar.gz ]
then
    echo "Downloading paho mqtt c v1.2.0 source ..."
    curl -LJO https://github.com/eclipse/paho.mqtt.c/archive/v1.2.0.tar.gz
fi

cd ..
if [ ! -d paho.mqtt.c-1.2.0 ]
then
    echo "Unpacking paho mqtt c v1.2.0 source ..."
    tar xzf download/paho.mqtt.c-1.2.0.tar.gz
fi

touch .setup_done

