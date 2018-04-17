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

all: setup build install

build: setup
	@echo "Build Watson IoT Platform client based on Paho MQTT c client, for NXP i.MX Distro."
	make -C paho.mqtt.c-1.2.0 CLIENTDIR=$(CLIENTDIR)

clean: uninstall
	-rm -rf paho.mqtt.c-1.2.0 download .setup_done
setup:
	@echo "Watson IoT Platform client based on Paho MQTT c client, for NXP i.MX Distro.: Build setup"
	@chmod 755 ./setup.sh; ./setup.sh

install: build
	@echo "Install packages"
	@mkdir -p /usr/local/bin
	@mkdir -p /usr/local/lib
	@mkdir -p /usr/local/include
	@if [ -d "paho.mqtt.c-1.2.0" ]; then make -C paho.mqtt.c-1.2.0 install CLIENTDIR=$(CLIENTDIR); fi

uninstall:
	@echo "Uninstall packages"
	@if [ -d "paho.mqtt.c-1.2.0" ]; then make -i -C paho.mqtt.c-1.2.0 uninstall CLIENTDIR=$(CLIENTDIR); fi

