#!/bin/bash
#
# Name: provisionA71CH_WatsonIoT.sh
# Revision: 0.xx
# Date: May 3, 2018
# 
#########################################################################################################
# Script to create and configure NXP A71CH Secure Element to work with Watson IoT platform
#########################################################################################################
#
# Prereq:
# - Device/Gateway with NXP A71CH Secure Element
# - OS: NXP i.MX Release Distro 4.9.11-1.0.0 (morty)
# - Openssl version OpenSSL 1.0.2j
# - NXP A71CH Secure Element configuration tool
#########################################################################################################
#
# Change History:
#
# Revision: 0.92    - Original script created by Marc Masschelein (NXP)
#                   - Conditionally create a Root CA
#                   - Either provide UID as argument or use config tool to retrieve it from the A71CH
#                   - Create a keypair, CSR & client certificate
#                   - Inject credentials in attached A71CH (and create a script)
#
# Revision: 0.xx    - Updated by Ranjan Dasgupta (IBM) - April 11, 2018
#                   - Option to create an intermediate certificate
#                     - If intermediate certificate is created, script will also create 
#                       CACertificateChain.crt file - i.e. chain of intermediate and root CA.
#                   - Creates a generic device key and refernce key. Name is changed from
#                     <uuid>_device.key to <uuid>key and from <uuid>_device.ref_key to <uuid>.ref_key
#                   - Creates two client certificates as Secure Element can be used in  a device
#                     that can function as a gateway or a device:
#                     For device:  <uuid>_device_ec_pem.crt
#                     For gateway:  <uuid>_gateway_ec_pem.crt
#                   - Added variable for CA and Client ceritificate validity period
#                   - Adds <uuid>_device_ec_pem.crt in SE slot 0 i.e. -x 0
#                   - Adds <uuid>_gateway_ec_pem.crt in SE slot 1 i.e. -x 1
#                   - Sets Device and Gateway certificate serial number is in the following format:
#                     For device  0x01<Secure_Element_UID>
#                     For gateway 0x02<Secure_Element_UID>
#
#                   - Ensuring Intermediate CA is used when requested.
#
#########################################################################################################
##

# Set global variables to default values
gExecMsg=""
gRetValue=0

# Tools
A71CH_CONFIG_TOOL="./a71chConfig_i2c_imx"

# Customization variables
# For Watson IoT Platform Device and Gateway types
#
devType="NXP-A71CH-D"
gwType="NXP-A71CH-G"
#
# For root CA
#
ROOT_CA_CN="NXP Semiconductors rootCA v E"
CA_EXISTS="FALSE"
CA_ENC_TYPE="ECC"             # Options are RSA or ECC
CA_ECC_CURVE="prime256v1"     # Only used if CA_ENC_TYPE is ECC
# CA_ECC_CURVE="secp384r1"    # Only used if CA_ENC_TYPE is ECC
#
# For intermediate CA - Intermediate CA will be signed by root CA
#
CREATE_INTERMEDIATE_CA="TRUE"
INTERMEDIATE_CA_CN="NXP Semiconductors interCA v E"
INTERMEDIATE_CA_EXISTS="FALSE"
INTERMEDIATE_CA_ENC_TYPE="ECC"            # Supported type for intermedite CA is ECC
INTERMEDIATE_CA_ECC_CURVE="prime256v1"    # Only used if INTERMEDIATE_CA_ENC_TYPE is ECC
#
# Certificate validity
#
CA_CERT_VALIDITY=7300                     # 20 years
CLIENT_CERT_VALIDITY=7300                 # 20 years

# Root CA Files
if [ "${CA_ENC_TYPE}" == "RSA" ]; then
    rootcaKey="CACertificate_RSA.key"
    rootcaCert="CACertificate_RSA.crt"
    ca_srl="CACertificate_RSA.srl"
else
    rootcaKey="CACertificate_ECC.key"
    rootcaCert="CACertificate_ECC.crt"
    ca_srl="CACertificate_ECC.srl"
fi
# Intermediate CA files - RSA is not supported yet
intercaKey="interCACertificate_ECC.key"
intercaCsr="interCACertificate_ECC.csr"
intercaCert="interCACertificate_ECC.crt"
inter_srl="interCACertificate_ECC.srl"
# Chain of CA root and intermediate certificate
chaincaCert="CACertificateChain.crt"

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


###########################################
# Create root CA key and certificate.
###########################################

if [ "${CA_EXISTS}" != "TRUE" ]; then

  if [ "${CA_ENC_TYPE}" == "RSA" ]; then

    # RSA Root Certificates
    if [ ! -e ${rootcaKey} ]; then
        # Create both the CA keypair and CA certificate
        # The root CA is RSA 4096
        echo "## Create RSA Root CA Key: (${rootcaKey})"
        xCmd "openssl genrsa -out ${rootcaKey} 4096"
        echo "## Create RSA Root CA Certificate: (${rootcaCert})"
        openssl req -config ./openssl.cnf -x509 -new -nodes -key ${rootcaKey} -sha256 -days ${CA_CERT_VALIDITY} -out ${rootcaCert} -subj "/CN=${ROOT_CA_CN}"
    fi

    if [ ! -e ${rootcaCert} ]; then
        echo "## Create ECC Root CA Certificate: (${rootcaCert}); Root CA keypair was already present"
        openssl req -config ./openssl.cnf -x509 -new -nodes -key ${rootcaKey} -sha256 -days ${CA_CERT_VALIDITY} -out ${rootcaCert} -subj "/CN=${ROOT_CA_CN}"
    fi

  else

    # ECC Root Certificates
    if [ ! -e ${rootcaKey} ]; then
        # Create both the CA keypair and CA certificate
        # The root CA is ECC using curve defined in CA_ECC_CURVE variable
        echo "## Create ECC Root CA Key: (${rootcaKey}) Curve: (${CA_ECC_CURVE})"
        xCmd "openssl ecparam -genkey -name ${CA_ECC_CURVE} -out ${rootcaKey}"
        echo "## Create ECC Root CA Certificate: (${rootcaCert})"
        openssl req -config ./openssl.cnf -x509 -new -nodes -key ${rootcaKey} -sha256 -days ${CA_CERT_VALIDITY} -out ${rootcaCert} -subj "/CN=${ROOT_CA_CN}"
    fi

    if [ ! -e ${rootcaCert} ]; then
        echo "## Create ECC Root CA Certificate: (${rootcaCert}); Root CA keypair was already present"
        openssl req -config ./openssl.cnf -x509 -new -nodes -key ${rootcaKey} -sha256 -days ${CA_CERT_VALIDITY} -out ${rootcaCert} -subj "/CN=${ROOT_CA_CN}"
    fi

  fi

else
    # Check whether RootCA key and certificate are present.
    if [ ! -e ${rootcaKey} ]; then
        if [ "${CA_ENC_TYPE}" == "RSA" ]; then
            echo "## No RSA Root CA Key available (${rootcaKey})"
        else
            echo "## No ECC Root CA Key available (${rootcaKey})"
        fi
        exit 8
    fi

    if [ ! -e ${rootcaCert} ]; then
        if [ "${CA_ENC_TYPE}" == "RSA" ]; then
            echo "## No RSA Root CA Certificate available (${rootcaCert})"
        else
            echo "## No ECC Root CA Certificate available (${rootcaCert})"
        fi
        exit 9
    fi
fi    


##############################################
# Create intermediate CA key and certificate.
##############################################

# echo "Check whether a .srl file of Root CA is present"
if [ -e ${ca_srl} ]; then
    echo "## ${ca_srl} already exists, use it"
    x509_serial="-CAserial ${ca_srl}"
else
    echo "## no ${ca_srl} found, create it"
    x509_serial="-CAcreateserial"
fi

SIGN_BY_INTERMEDIATE_CA="FALSE"

echo "## CREATE_INTERMEDIATE_CA = ${CREATE_INTERMEDIATE_CA}"

if [ "${CREATE_INTERMEDIATE_CA}" == "TRUE" ]
then

  echo "## Create Intermediate CA"

  if [ "${INTERMEDIATE_CA_EXISTS}" != "TRUE" ]
  then

    # Define the v3 extension that will be included in the intermediate certificate
    echo "## Create v3 extension file for Intermediate CA Certificate"
    echo "[ v3_req ]"                                          > v3_ext.cnf
    echo "basicConstraints = CA:TRUE, pathlen:0"               >> v3_ext.cnf
    echo "keyUsage = critical, digitalSignature, keyCertSign"  >> v3_ext.cnf


    if [ "${INTERMEDIATE_CA_ENC_TYPE}" == "RSA" ]
    then

        echo "## RSA Encryption type is not supported yet."
        exit 10

    else

        # ECC Intemediate Certificates
        if [ ! -e ${intercaKey} ]
        then
            # Create both the CA keypair and CA certificate
            # The interemediate CA is ECC using curve defined in INTERMEDIATE_CA_ECC_CURVE variable
            echo "## Create ECC intermediate CA Key: (${intercaKey}) Curve: (${INTERMEDIATE_CA_ECC_CURVE})"
            xCmd "openssl ecparam -genkey -name ${INTERMEDIATE_CA_ECC_CURVE} -out ${intercaKey}"
            echo "## Create RSA Intermediate CA CSR: (${intercaCsr})"
            openssl req -new -key ${intercaKey} -out ${intercaCsr} -subj "/CN=${INTERMEDIATE_CA_CN}"
            echo "## Sign Intermediate CA CSR with (${rootcaCert})"
            openssl x509 -req -days ${CA_CERT_VALIDITY} -in ${intercaCsr} "${x509_serial}" -CA ${rootcaCert} \
                -CAkey ${rootcaKey}  -extfile ./v3_ext.cnf -extensions v3_req -out ${intercaCert}
        fi

        if [ ! -e ${intercaCert} ]; then
            echo "## Create RSA Intermediate CA CSR: (${intercaCsr})"
            openssl req -new -key ${intercaKey} -out ${intercaCsr} -subj "/CN=${INTERMEDIATE_CA_CN}"
            echo "## Sign Intermediate CA CSR with (${rootcaCert})"
            openssl x509 -req -days ${CA_CERT_VALIDITY} -in ${intercaCsr} "${x509_serial}" -CA ${rootcaCert} \
                -CAkey ${rootcaKey}  -extfile ./v3_ext.cnf -extensions v3_req -out ${intercaCert}
        fi

    fi

  else

    # Check whether Intermediate CA key and certificate are present.
    if [ ! -e ${intercaKey} ]; then
        if [ "${INTERMEDIATE_CA_ENC_TYPE}" == "RSA" ]; then
            echo "## RSA Encryption type is not supported yet."
        else
            echo "## No ECC Intermediate CA Key available (${intercaKey})"
        fi
        exit 11 
    fi

    if [ ! -e ${intercaCert} ]; then
        if [ "${INTERMEDIATE_CA_ENC_TYPE}" == "RSA" ]; then
            echo "## RSA Encryption type is not supported yet."
        else
            echo "## No ECC Intermediate CA Certificate available (${intercaCert})"
        fi
        exit 12 
    fi

  fi

  SIGN_BY_INTERMEDIATE_CA="TRUE"

fi


############################################
# Create key and device/gateway certificates
############################################

if [ "${SIGN_BY_INTERMEDIATE_CA}" == "TRUE" ]; then

    # Create Chain of CA Certificates for WIoTP CA Certificate upload step
    cat ${intercaCert} ${rootcaCert} > ${chaincaCert}
fi

# Create serial number of gateway and device certificates
# for device  1${SE_UID}
# for gateway 2${SE_UID}
deviceSerial="0x01${SE_UID}"
gatewaySerial="0x02${SE_UID}"

# Unique ID Key, reference key and CSR files
uidKey="${SE_UID}.key"
uidRefKey="${SE_UID}.ref_key"
uidCsr="${SE_UID}.csr"

# Device certificate
deviceCert="${SE_UID}_device_ec_pem.crt"
deviceCertAscii="${SE_UID}_device_ec_crt_ascii_dump.txt"

# Gateway certificate
gatewayCert="${SE_UID}_gateway_ec_pem.crt"
gatewayCertAscii="${SE_UID}_gateway_ec_crt_ascii_dump.txt"

echo "## Preparing device certificate ${deviceCert}"
CN_val="${SE_UID}"
GENERIC_val="URI:NXP:${SE_UID}"
SAN_D_val="email:d:${devType}:${SE_UID}, ${GENERIC_val}"
SAN_G_val="email:g:${gwType}:${SE_UID}, ${GENERIC_val}"

echo "##    CN=${CN_val}"   
echo "##    SAN_D_VAL=${SAN_D_val}"   
echo "##    SAN_G_VAL=${SAN_G_val}"   

# Create a device key pair & CSR
xCmd "openssl ecparam -genkey -name prime256v1 -out ${uidKey}"
openssl req -new -key ${uidKey} -out ${uidCsr} -subj "/CN=${CN_val}"

# Define the v3 extension that will be included in the device certificate
echo "[ v3_req ]"                                    > v3_ext.cnf
echo ""                                             >> v3_ext.cnf
echo "# Extensions to add to a certificate request" >> v3_ext.cnf
echo ""                                             >> v3_ext.cnf
echo "basicConstraints = CA:FALSE"                  >> v3_ext.cnf
echo "# keyUsage = nonRepudiation, digitalSignature, keyEncipherment" >> v3_ext.cnf
echo "keyUsage = digitalSignature"                  >> v3_ext.cnf
echo "subjectAltName = ${SAN_D_val}"                >> v3_ext.cnf

if [ "${SIGN_BY_INTERMEDIATE_CA}" == "TRUE" ]; then
    # Create a Cert signed by Intermediate CA
    xCmd "openssl x509 -req -days ${CLIENT_CERT_VALIDITY} -in ${uidCsr} -set_serial ${deviceSerial} -CA ${intercaCert} -CAkey ${intercaKey} \
        -extfile ./v3_ext.cnf -extensions v3_req -out ${deviceCert}"
else
    # Create a Cert signed by Root CA
    xCmd "openssl x509 -req -days ${CLIENT_CERT_VALIDITY} -in ${uidCsr} -set_serial ${deviceSerial} -CA ${rootcaCert} -CAkey ${rootcaKey} \
        -extfile ./v3_ext.cnf -extensions v3_req -out ${deviceCert}"
fi
openssl x509 -in ${deviceCert} -text -noout > ${deviceCertAscii}
echo "## Device certificate ${deviceCert} created successfully"


# Define the v3 extension that will be included in the gateway certificate
echo "[ v3_req ]"                                    > v3_ext.cnf
echo ""                                             >> v3_ext.cnf
echo "# Extensions to add to a certificate request" >> v3_ext.cnf
echo ""                                             >> v3_ext.cnf
echo "basicConstraints = CA:FALSE"                  >> v3_ext.cnf
echo "# keyUsage = nonRepudiation, digitalSignature, keyEncipherment" >> v3_ext.cnf
echo "keyUsage = digitalSignature"                  >> v3_ext.cnf
echo "subjectAltName = ${SAN_G_val}"                >> v3_ext.cnf
if [ "${SIGN_BY_INTERMEDIATE_CA}" == "TRUE" ]; then
    # Create a Cert signed by Intermediate CA
    xCmd "openssl x509 -req -days ${CLIENT_CERT_VALIDITY} -in ${uidCsr} -set_serial ${gatewaySerial} -CA ${intercaCert} -CAkey ${intercaKey} \
        -extfile ./v3_ext.cnf -extensions v3_req -out ${gatewayCert}"
else
    # Create a Cert signed by Root CA
    xCmd "openssl x509 -req -days ${CLIENT_CERT_VALIDITY} -in ${uidCsr} -set_serial ${gatewaySerial} -CA ${rootcaCert} -CAkey ${rootcaKey} \
        -extfile ./v3_ext.cnf -extensions v3_req -out ${gatewayCert}"
fi
openssl x509 -in ${gatewayCert} -text -noout > ${gatewayCertAscii}
echo "## Gateway certificate ${gatewayCert} created successfully"


###########################################
# Create config tool script and execute it.
###########################################

configScript="${SE_UID}_a71chConfigScript.txt"

echo "## Create config tool script ${configScript}"

echo "# ################################################"                                     > ${configScript}
echo "# Name: ${configScript}"                                                                >> ${configScript}
echo "# Revision 0.9"                                                                         >> ${configScript}
echo "# Purpose: Provision A71CH matching UID ("${SE_UID}") for Watson IoT"                   >> ${configScript}
echo "# Pre-condition: device has debug interface enabled"                                    >> ${configScript}
echo "# Post-condition: keypair and certificate injected"                                     >> ${configScript}
echo "# ################################################"                                     >> ${configScript}
echo "debug reset   # Bring secure element in its original state"                             >> ${configScript}
echo "info device"                                                                            >> ${configScript}
echo "set pair -x 0 -k ${uidKey} "                                                            >> ${configScript}
echo "info pair     # Should display public key of keypair just written"                      >> ${configScript}
echo "wcrt -x 0 -p ${deviceCert} "                                                            >> ${configScript}
echo "wcrt -x 1 -p ${gatewayCert} "                                                           >> ${configScript}
echo "info all      # Should show that the certificate is loaded in GP storage (DER format)"  >> ${configScript}
echo "refpem -c 10 -x 0 -r ${uidRefKey} # Creates the reference key on the file system"       >> ${configScript}

xCmd "${A71CH_CONFIG_TOOL} script -f ${configScript}"

echo "## Successfully configured A71CH sample ${SE_UID}"

exit 0

