# Provision A71CH 
## For IBM Watson IoT Platform Demo

This document describes *NXP A71CH* provisioning process required for connecting to
IBM Watson IoT Platform, for demo or prototype development.

## 1 Prepare your board

Follow the instructions in the [NXP A71CH Product Support Package (PSP)](https://www.nxp.com/A71CH)
to connect the boards and, prepare and load the software.


## 2 Credential preparation and injection

To prepare and inject the credentials for the A71CH, ensure you have a working combination of *MCIMX6UL-EVKB* 
and *A71CH* boards. The following tools are used in this step:

* OpenSSL (openssl) is used to create the credentials.
* The A71CH Configure tool (a71chConfig_i2c_imx) is used to retrieve the UID from the A71CH and 
to inject key pair and client certificate. 
* The unix command line utilities grep and awk are used to extract bytes forming the UID part 
of the client certificate's Common Name.

All these tools come pre-installed on the SD card image made available for 
the MCIMX6UL-EVKB board on the [A71CH website](https://www.nxp.com/A71CH).

To provision an A71CH miniPCB with Watson IoT credentials, use the bash shell script '[provisionA71CH_WatsonIoT.sh](https://github.com/ibm-watson-iot/iot-nxpimxa71ch-c/blob/master/samples/provisionA71CH_WatsonIoT.sh)'. Copy the script onto the embedded file system of the MCIMX6UL-EVKB board in directory /home/root/tools. Use the following commands to execute the script:

```
cd /home/root/tools
chmod +x provisionA71CH_WatsonIoT.sh
./provisionA71CH_WatsonIoT.sh
```

Executing the script results in the following actions:

* It creates a Root CA key pair, an Intermediate CA key pair and associated certificates (an existing CA / Intermediate CA will be re-used).
* It creates a device key pair on the NIST-P256 ECC Curve.
* It issues two certificates - signed by the Intermediate CA - including the UID of the attached A71CH and the public key of the device key pair. The two certificates are a device and a gateway specific certificate. 
* It injects these credentials (i.e. device key pair, device certificate and gateway certificate) into the attached A71CH

Store your *CACertificate_ECC.crt* and *interCACertificate_ECC.crt* in order to upload them later into 
the IBM Watson IoT Platform in step [Register Certificate Authority](https://github.com/ibm-watson-iot/iot-nxpimxa71ch-c/blob/master/README.md#user-content-register-certificate-authority).

Note down your device UID in order to register the device and gateway later into the IBM Watson IoT Platform 
in step [Register Device Type and Device](https://github.com/ibm-watson-iot/iot-nxpimxa71ch-c/blob/master/README.md#user-content-register-device-type-and-device).
 
Note that the CN_val used for creating the device or gateway certificate is in a specific format. Any device
connecting to IBM Watson IoT Platform using a certificate, must have the device ID in the 
Common Name (CN) or SubjectAltName in the certificate. For details, refer to the documentation on
[Configuring certificates](https://console.bluemix.net/docs/services/IoT/reference/security/set_up_certificates.html#set_up_certificates).

