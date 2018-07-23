
# 
# The Makefile is updated to include Watson IoT Platform C client library functions.
# 

current_dir := $(shell pwd)
parent_dir := $(shell dirname ${current_dir})

AXHOSTSW_DIR := /home/root/axHostSw

SRCDIR = ${parent_dir}/src
OBJDIR = ${parent_dir}/build

ifndef prefix
        prefix = /usr/local
endif

includedir = $(prefix)/include
libdir = $(prefix)/lib

CC ?= gcc
LDCONFIG ?= /sbin/ldconfig
RM = rm -f

ifndef INSTALL
INSTALL = install
endif
INSTALL_PROGRAM = $(INSTALL)
INSTALL_DATA =  $(INSTALL) -m 644

MAJOR_VERSION = 1
MINOR_VERSION = 0
VERSION = ${MAJOR_VERSION}.${MINOR_VERSION}

CINCS = -I$(AXHOSTSW_DIR)/linux/A71CH/inc \
        -I$(AXHOSTSW_DIR)/hostLib/a71ch/inc \
        -I$(AXHOSTSW_DIR)/hostLib/api/inc \
        -I/usr/local/include \
        -I$(SRCDIR)

CFLAGS = $(CINCS) -fPIC -Wall -Wextra -O2 -g -DLINUX -DTGT_A71CH -DOPENSSL -DI2C
LDFLAGS = -shared -lssl -lcrypto -lpaho-mqtt3cs -le2a71chi2c -lA71CH_i2c_imx

WIOTPLIB = libwiotpnxpimxa71ch.so
TARGET_LIB = $(OBJDIR)/${WIOTPLIB}.${VERSION}

# SOURCES  := $(wildcard $(SRCDIR)/*.c)
# OBJECTS  := $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

SOURCES := config.c a71chRetrieveCertificates.c gatewayclient.c iotfclient.c deviceclient.c iotf_utils.c cJSON.c manageddevice.c
OBJECTS := $(SOURCES:%.c=$(OBJDIR)/%.o)

SOURCES_ASYNC := a71chRetrieveCertificates.c gatewayclient.c iotfclient_async.c deviceclient.c iotf_utils.c
OBJECTS_ASYNC := $(SOURCES_ASYNC:%.c=$(OBJDIR)/%.o)

.PHONY: all clean $(OBJECTS) $(TARGET_LIB)

all: mkdir ${TARGET_LIB}

$(OBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.c
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo "Compiled "$<" successfully!"

mkdir:
	-mkdir -p ${OBJDIR}

$(TARGET_LIB): $(OBJECTS)
	$(CC) ${LDFLAGS} -o $@ $^

install:
	$(INSTALL_DATA) ${SRCDIR}/iotfclient.h ${includedir}
	$(INSTALL_DATA) ${SRCDIR}/iotf_utils.h ${includedir}
	$(INSTALL_DATA) ${TARGET_LIB} ${libdir}
	$(LDCONFIG) ${libdir}
	ln -s ${WIOTPLIB}.${VERSION} ${libdir}/${WIOTPLIB}

uninstall:
	-${RM} ${includedir}/iotfclient.h
	-${RM} ${includedir}/iotf_utils.h
	-${RM} ${libdir}/${WIOTPLIB}.${VERSION}
	-${RM} ${libdir}/${WIOTPLIB}

clean:
	-${RM} build/*


