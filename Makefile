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

current_dir := $(shell pwd)
parent_dir := $(shell dirname ${current_dir})

SHELL = /bin/sh
.PHONY: setup, build, clean, install, uninstall

all: setup build install

build: setup
	@echo "Watson IoT Platform client based on Paho MQTT c client, for NXP i.MX Distro."
	cd paho.mqtt.c-1.2.0; make

clean:
	test -d paho.mqtt.c-1.2.0 && (cd paho.mqtt.c-1.2.0; make -i uninstall)
	-rm -rf paho.mqtt.c-1.2.0
	-rm -rf download
	-rm -f .setup_done

setup:
	@echo "Watson IoT Platform client based on Paho MQTT c client, for NXP i.MX Distro.: Build setup"
	@[ -f ./.setup_done ] ||  ./setup.sh

install: build
	cd paho.mqtt.c-1.2.0; make install

uninstall:
	cd paho.mqtt.c-1.2.0; make uninstall

