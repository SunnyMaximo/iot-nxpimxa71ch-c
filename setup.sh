#!/bin/bash

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

# Copy iotf client source and samples in build directory
echo "Copying the necessary source files to build"
# cp ./samples/*.c ./build/src/samples/.

# Update Paho MQTT C build tree to include WIoT C client code
echo "Updating paho mqtt c library build for IBM Watson IoT platform client ..."
cd paho.mqtt.c-1.2.0

# Update Makefile
# Add remark about the changes
sed -i '/^SHELL = /i \
# \
# The Makefile is updated to include Watson IoT Platform C client library functions. \
# \
\
current_dir := $(shell pwd)\
parent_dir := $(shell dirname ${current_dir})\
\
' Makefile

# Add WIoTP wiotp source directory defines
sed -i '/^SOURCE_FILES = /i \
ifndef wiotp_srcdir \
        wiotp_srcdir = ${parent_dir}/src\
endif \
\
ifndef wiotp_samplesdir \
        wiotp_samplesdir = ${parent_dir}/samples\
endif \
\
CFLAGS = -I${srcdir} -I${wiotp_srcdir} \
\
WIOTP_SOURCE_FILES = $(wiotp_srcdir)/deviceclient.c $(wiotp_srcdir)/gatewayclient.c $(wiotp_srcdir)/iotf_utils.c $(wiotp_srcdir)/iotfclient.c \
WIOTP_HEADERS = $(wiotp_srcdir)/deviceclient.h $(wiotp_srcdir)/gatewayclient.h $(wiotp_srcdir)/iotf_utils.h $(wiotp_srcdir)/iotfclient.h \
IOTF_SAMPLE_FILES_C = helloWorld \
IOTF_SAMPLES = ${addprefix ${blddir}/samples/,${IOTF_SAMPLE_FILES_C}} \
' Makefile

# Add wiotp sources
sed -i '/^SOURCE_FILES = / s/$/ $(WIOTP_SOURCE_FILES)/' Makefile

# Add wiotp headers
sed -i '/^HEADERS = / s/$/ $(WIOTP_HEADERS)/' Makefile

# Add OPENSSL_LOAD_CONF build directive
sed -i 's/-DOPENSSL /-DOPENSSL -DOPENSSL_LOAD_CONF/g' Makefile

# Update list of build targets
sed -i '/^build:.*/c\build: | mkdir ${MQTTLIB_CS_TARGET} ${IOTF_SAMPLES}' Makefile

# Update install rule
sed -i '/^install: buil/c\install: build dummy-unsupported-mqtt-libs install-iotf' Makefile

# Update uninstall rule
sed -i '/^uninstall:/c\uninstall: uninstall-iotf' Makefile

# Add iotf specific rules
echo >> Makefile
cat >> Makefile << EOF
\${IOTF_SAMPLES}: \${blddir}/samples/%: \${wiotp_samplesdir}/%.c \$(MQTTLIB_CS_TARGET)
	\${CC} -o \$@ \$< -I\${srcdir} -I\${wiotp_srcdir} -l\${MQTTLIB_CS} \${FLAGS_EXES}

dummy-unsupported-mqtt-libs:
	touch \${MQTTLIB_C_TARGET}
	touch \${MQTTLIB_A_TARGET}
	touch \${MQTTLIB_AS_TARGET}
	touch \${MQTTVERSION_TARGET}

install-iotf: build
	\$(INSTALL_DATA) \${wiotp_srcdir}/iotfclient.h \$(DESTDIR)\${includedir}
	\$(INSTALL_DATA) \${wiotp_srcdir}/deviceclient.h \$(DESTDIR)\${includedir}
	\$(INSTALL_DATA) \${wiotp_srcdir}/gatewayclient.h \$(DESTDIR)\${includedir}
	\$(INSTALL_DATA) \${wiotp_srcdir}/iotf_utils.h \$(DESTDIR)\${includedir}
	mkdir -p \$(CLIENTDIR)bin
	mkdir -p \$(CLIENTDIR)config
	mkdir -p \$(CLIENTDIR)certs
	\$(INSTALL_PROGRAM) \${blddir}/samples/helloWorld \$(CLIENTDIR)bin/.
	\$(INSTALL_DATA) \${wiotp_samplesdir}/*.pem \$(CLIENTDIR)certs/.
	\$(INSTALL_DATA) \${wiotp_samplesdir}/*.cfg \$(CLIENTDIR)config/.

uninstall-iotf:
	-rm \$(DESTDIR)\${includedir}/iotfclient.h
	-rm \$(DESTDIR)\${includedir}/deviceclient.h
	-rm \$(DESTDIR)\${includedir}/gatewayclient.h
	-rm \$(DESTDIR)\${includedir}/iotf_utils.h
	-rm \$(CLIENTDIR)bin/helloWorld

print-%  : ; @echo \$* = \$(\$*)

EOF


cd ..
touch .setup_done

