#*******************************************************************************
#  Copyright (c) 2018 IBM Corp.
# 
#  All rights reserved. This program and the accompanying materials
#  are made available under the terms of the Eclipse Public License v1.0
#  and Eclipse Distribution License v1.0 which accompany this distribution. 
# 
#  The Eclipse Public License is available at 
#     http://www.eclipse.org/legal/epl-v10.html
#  and the Eclipse Distribution License is available at 
#    http://www.eclipse.org/org/documents/edl-v10.php.
# 
#  Contributors:
#     Ranjan Dasgupta - initial drop
#
#*******************************************************************************/

# 
# The Makefile is updated to include Watson IoT Platform C client library functions. 
# 

SHELL = /bin/sh
.PHONY: setup, build, clean, install, uninstall

CLIENTDIR := /opt/iotnxpimxclient/

all: wiotp_install wiotp_samples

build: wiotp_lib 

setup:
	@echo "Setup for Watson IoT Platform client build, based on Paho MQTT c client, for NXP i.MX Distro."
	@chmod 755 ./setup.sh; ./setup.sh

paho_mqtt_libs: setup
	@echo "Build Paho MQTT c client library, for NXP i.MX Distro."
	make -C paho.mqtt.c-1.2.0

paho_mqtt_libs_install: paho_mqtt_libs
	@echo "Install Paho MQTT c client library, for NXP i.MX Distro."
	make -i -C paho.mqtt.c-1.2.0 install

wiotp_lib: paho_mqtt_libs_install
	@echo "Build Watson IoT Platform client library based on Paho MQTT c client, for NXP i.MX Distro."
	make -C src

wiotp_lib_install: wiotp_lib
	@echo "Install Watson IoT Platform client library, for NXP i.MX Distro."
	make -i -C src install

wiotp_samples: wiotp_lib_install
	@echo "Build Watson IoT Platform client samples, for NXP i.MX Distro."
	make -C samples CLIENTDIR=$(CLIENTDIR)

install: wiotp_samples
	@echo "Install Watson IoT Platform client library and samples"

uninstall:
	@echo "Uninstall Watson IoT Platform client library and samples"
	make -i -C samples uninstall
	make -i -C src uninstall
	make -i -C paho.mqtt.c-1.2.0 uninstall

clean: uninstall
	-rm -rf paho.mqtt.c-1.2.0 download build .setup_done


