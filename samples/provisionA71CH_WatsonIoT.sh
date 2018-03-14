#!/bin/bash
# Revision: 0.92
# 
# Script dealing with Watson IoT credentials
# - Conditionally create a Root CA
# - Either provide UID as argument or use config tool to retrieve it from the A71CH
# - Create a keypair, CSR & client certificate
# - Inject credentials in attached A71CH (and create a script)

# Set global variables to default values
gExecMsg=""
gRetValue=0

# Tools
A71CH_CONFIG_TOOL="./a71chConfig_i2c_imx"

# Customization variables
devType="NXP-A71CH"
ROOT_CA_CN_ECC="NXP Semiconductors DEMO rootCA v E"
CA_EXISTS="TRUE"

# CA 
rootcaKeyEcc="CACertificate_ECC.key"
rootcaCertEcc="CACertificate_ECC.crt"
ca_srl="CACertificate_ECC.srl"

# UTILITY FUNCTIONS
# -----------------
# xCmd will stop script execution when the program executed does return gRetValue (0 by default) to the shell
xCmd () {
    local command="$*"
    echo ">> ${gExecMsg}"
    echo ">> ${command}"
    ${command}
    local nRetProc="$?"
    if [ ${nRetProc} -ne ${gRetValue} ]
    then
        echo "\"${command}\" failed to run successfully, returned ${nRetProc}"
        echo "** Script execution failed **"
        exit 2
    fi
    echo ""
    # Set global variables to default values
    gExecMsg=""
    gRetValue=0
}

# Provide UID as argument, or
# attach the A71CH and have the script retrieve the UID from the IC
if [ "$#" -eq 1 ]; then
    # Check that argument (UID) is 20 ASCII characters long
    SE_UID="$1"
    if [ "${#SE_UID}" -ne 20 ]; then
        echo "  The UID (provided as argument) must be exactly 20 characters long"
        echo "The current argument is ${#SE_UID} characters long"
        echo "Exiting..."
        exit 4
    fi
else
    echo "  Get UID from attached A71CH"
    # First command executed to check whether 
    # - Config tool is available
    # - an A71CH device is attached
    xCmd "${A71CH_CONFIG_TOOL} info device"
    # ${A71CH_CONFIG_TOOL} info device | grep -e "[0-9][0-9]:[0-9][0-9]:[0-9][0-9]"
    SE_UID="$(${A71CH_CONFIG_TOOL} info device | grep -e "\([0-9][0-9]:\)\{17,\}" \
        | awk 'BEGIN {FS=":"}; {printf $3$4$9$10$11$12$13$14$15$16}')"
    echo "SE_UID: ${SE_UID}"
fi

# Sanity check on UID
if [ "${#SE_UID}" -ne 20 ]; then
    echo "  Could not retrieve UID from A71CH."
    echo "Exiting..."
    exit 5
fi

if [ "${CA_EXISTS}" != "TRUE" ]; then
    if [ ! -e ${rootcaKeyEcc} ]; then
        # Create both the CA keypair and CA certificate
        # The root CA is ECC (NIST P-256) prime256v1 in OpenSSL speak
        echo "## Create ECC Root CA Key: (${rootcaKeyEcc})"
        xCmd "openssl ecparam -genkey -name prime256v1 -out ${rootcaKeyEcc}"
        echo "## Create ECC Root CA Certificate: (${rootcaCertEcc})"
        openssl req -x509 -new -nodes -key ${rootcaKeyEcc} -sha256 -days 3650 -out ${rootcaCertEcc} -subj "/CN=${ROOT_CA_CN_ECC}"
    fi

    if [ ! -e ${rootcaCertEcc} ]; then
        echo "## Create ECC Root CA Certificate: (${rootcaCertEcc}); Root CA keypair was already present"
        openssl req -x509 -new -nodes -key ${rootcaKeyEcc} -sha256 -days 3650 -out ${rootcaCertEcc} -subj "/CN=${ROOT_CA_CN_ECC}"
    fi
else
    # Check whether RootCA key and certificate are present.
    if [ ! -e ${rootcaKeyEcc} ]; then
        echo "## No ECC Root CA Key available (${rootcaKeyEcc})"
        exit 8
    fi

    if [ ! -e ${rootcaCertEcc} ]; then
        echo "## No ECC Root CA Certificate available (${rootcaCertEcc})"
        exit 9
    fi
fi    

# echo "Check whether a .srl file is present"
if [ -e ${ca_srl} ]; then
    echo "## ${ca_srl} already exists, use it"
    x509_serial="-CAserial ${ca_srl}"
else
    echo "## no ${ca_srl} found, create it"
    x509_serial="-CAcreateserial"
fi

###########################################
# Create device key and device certificate.
###########################################

deviceKey="${SE_UID}_device.key"
deviceRefKey="${SE_UID}_device.ref_key"
deviceCsr="${SE_UID}_device.csr"
deviceCert="${SE_UID}_device_ec_pem.crt"
deviceCertAscii="${SE_UID}_device_ec_crt_ascii_dump.txt"

echo "## Preparing device certificate ${deviceCert}"
CN_val="d:${devType}:${SE_UID}"
echo "##    CN=${CN_val}"   

# Create a device key pair & CSR
xCmd "openssl ecparam -genkey -name prime256v1 -out ${deviceKey}"
openssl req -new -key ${deviceKey} -out ${deviceCsr} -subj "/CN=${CN_val}"
# Define the v3 extension that will be included in the certificate
echo "[ v3_req ]"                                    > v3_ext.cnf
echo ""                                             >> v3_ext.cnf
echo "# Extensions to add to a certificate request" >> v3_ext.cnf
echo ""                                             >> v3_ext.cnf
echo "basicConstraints = CA:FALSE"                  >> v3_ext.cnf
echo "# keyUsage = nonRepudiation, digitalSignature, keyEncipherment" >> v3_ext.cnf
echo "keyUsage = digitalSignature"                  >> v3_ext.cnf
# Create a Cert signed by ECC CA
xCmd "openssl x509 -req -days 3650 -in ${deviceCsr} "${x509_serial}" -CA ${rootcaCertEcc} -CAkey ${rootcaKeyEcc} \
 -extfile ./v3_ext.cnf -extensions v3_req -out ${deviceCert}"
openssl x509 -in ${deviceCert} -text -noout > ${deviceCertAscii}
echo "## Device certificate ${deviceCert} created successfully"

###########################################
# Create config tool script and execute it.
###########################################

configScript="${SE_UID}_a71chConfigScript.txt"

echo "## Create config tool script ${configScript}"

echo "# ################################################" > ${configScript}
echo "# Name: ${configScript}"                                              >> ${configScript}
echo "# Revision 0.9"                                                       >> ${configScript}
echo "# Purpose: Provision A71CH matching UID ("${SE_UID}") for Watson IoT" >> ${configScript}
echo "# Pre-condition: device has debug interface enabled"                  >> ${configScript}
echo "# Post-condition: keypair and certificate injected"                   >> ${configScript}
echo "# ################################################"                   >> ${configScript}
echo "debug reset   # Bring secure element in its original state"           >> ${configScript}
echo "info device"                                                          >> ${configScript}
echo "set pair -x 0 -k ${deviceKey} "                                       >> ${configScript}
echo "info pair     # Should display public key of keypair just written"    >> ${configScript}
echo "wcrt -x 0 -p ${deviceCert} "                                          >> ${configScript}
echo "info all      # Should show that the certificate is loaded in GP storage (DER format)"  >> ${configScript}
echo "refpem -c 10 -x 0 -r ${deviceRefKey} # Creates the reference key on the file system"    >> ${configScript}

xCmd "${A71CH_CONFIG_TOOL} script -f ${configScript}"

echo "## Successfully configured A71CH sample ${SE_UID}"

exit 0
