# Provision A71CH 
## For IBM Watson IoT Platform Demo

This document describes *NXP A71CH* provisioning process required for connecting to
IBM Watson IoT Platform, for demo or prototype developemnt.

## 1 Prepare your board

Follow the instructions in the [NXP A71CH Product Support Package (PSP)](https://www.nxp.com/A71CH)
to connect the boards and, prepare and load the software.


## 2 Credential preparation and injection

To prepare and inject the Credentials for the A71CH, ensure you have a working combination of *MCIMX6UL-EVKB* 
and *A71CH* boards. The following tools are used in this step:

* OpenSSL (openssl) is used to create the Credentials.
* The A71CH Configure tool (a71chConfig_i2c_imx) is used to retrieve the UID from the A71CH and 
to inject key pair and client certificate. 
* The unix command line utilities grep and awk are used to extract bytes forming the UID part 
of the client certificate's Common Name.

Note that all these tools come pre-installed on the SD card image made available for 
the MCIMX6UL-EVKB board on the [A71CH website](https://www.nxp.com/A71CH).

### 2.1 Introduction

The following sections explain how to create a CA and device credentials, and 
how to inject the device credentials into the A71CH. 
 
To provision an A71CH miniPCB so it contains Watson IoT credentials:
- Follow the instructions under (2.2) to create a demo CA
- Use the bash shell script '[provisionA71CH_WatsonIoT.sh](https://github.com/ibm-watson-iot/iot-nxpimxa71ch-c/blob/master/samples/provisionA71CH_WatsonIoT.sh)' to execute 
the different steps outlined here starting from section 2.3.  

To use the script copy it onto the embedded file system of the MCIMX6UL-EVKB board in directory /home/root/tools. Ensure the script has execution rights set and invoke it from the /home/root/tools directory.

### 2.2 Create a Root and Intermediate CA keypair and certificate once

For the purpose of the demo ensure you only create Root CA keypair and certificate once. In case you already have a Root CA keypair and certificate available, you can skip this step.
	
In this step we will create a Root CA keypair (CACertificate_ECC.key) and a Root CA certificate (CACertificate_ECC.crt). 
The keypair will be created on the NIST-P256 ECC Curve.
Ensure to go into directory /home/root/tools by issuing the following command

    cd /home/root/tools

To create the key pair, execute the following commmand.

    openssl ecparam -genkey -name prime256v1 -out CACertificate_ECC.key
    
To create the CA certificate, execute the following commands. Note that you can adapt the value you give to the variable ROOT_CA_CN_ECC to match your own organization.

    export ROOT_CA_CN_ECC="NXP IMX-A71CH demo rootCA v E"
    openssl req -x509 -new -nodes -key CACertificate_ECC.key -sha256 -days 3650 -out CACertificate_ECC.crt -subj "/CN=${ROOT_CA_CN_ECC}"

Store your *CACertificate_ECC.crt* in order to upload the certificate later into 
the IBM Watson IoT Platform in step [Register Certificate Authority](https://github.com/ibm-watson-iot/iot-nxpimxa71ch-c/blob/master/README.md#user-content-register-certificate-authority).

### 2.3 Retrieve UID from A71CH with A71CH configure tool

The steps detailed out in subsections 2.3 until 2.6 are covered by bash shell script 
'[provisionA71CH_WatsonIoT.sh](https://github.com/ibm-watson-iot/iot-nxpimxa71ch-c/blob/master/samples/provisionA71CH_WatsonIoT.sh)'

The following command captures the correct byte's from the UID that will become part of the certificates common name into the environment variable SE_UID

    export SE_UID="$(./a71chConfig_i2c_imx info device | grep -e "\([0-9][0-9]:\)\{17,\}" | awk 'BEGIN {FS=":"}; {printf $3$4$9$10$11$12$13$14$15$16}')"
    echo "SE_UID: ${SE_UID}"

Note down your device UID in order to register the device later into the IBM Watson IoT Platform 
in step [Register Device Type and Device](https://github.com/ibm-watson-iot/iot-nxpimxa71ch-c/blob/master/README.md#user-content-register-device-type-and-device).
 

### 2.4 Create Device key pair

    cd /home/root/tools
    export deviceKey="${SE_UID}_device.key"
    openssl ecparam -genkey -name prime256v1 -out ${deviceKey}

### 2.5 Create Device certificate

Creating a device certificate implies creating first a CSR and submitting this CSR to the CA. The following covers these steps:

    echo "[ v3_req ]" > v3_ext.cnf
    echo "basicConstraints = CA:FALSE" >> v3_ext.cnf
    echo "keyUsage = digitalSignature" >> v3_ext.cnf

    export CN_val="d:NXP-A71CH-D:${SE_UID}"
    export deviceCsr="${SE_UID}_device.csr"
    export deviceCert="${SE_UID}_device_ec_pem.crt"
    openssl req -new -key ${deviceKey} -out ${deviceCsr} -subj "/CN=${CN_val}"
    openssl x509 -req -days 3650 -in ${deviceCsr} -CAcreateserial -CA CACertificate_ECC.crt -CAkey CACertificate_ECC.key -extfile ./v3_ext.cnf -extensions v3_req -out ${deviceCert}

Note that the CN_val used for creating the device certificate is in a specific format. Any device
connecting to IBM Watson IoT Platform using a device certificate, must have the device ID in the 
Common Name (CN) or SubjectAltName in the certificate. For details, refer to the documentation on
[Configuring certificates](https://console.bluemix.net/docs/services/IoT/reference/security/set_up_certificates.html#set_up_certificates).

### 2.6 Inject credentials into A71CH with Configure Tool

    export deviceRefKey="${SE_UID}_device.ref_key"
    ./a71chConfig_i2c_imx debug reset
    ./a71chConfig_i2c_imx info device
    ./a71chConfig_i2c_imx set pair -x 0 -k ${deviceKey}
    ./a71chConfig_i2c_imx info pair
    ./a71chConfig_i2c_imx wcrt -x 0 -p ${deviceCert}
    ./a71chConfig_i2c_imx info all
    ./a71chConfig_i2c_imx refpem -c 10 -x 0 -r ${deviceRefKey}
    
