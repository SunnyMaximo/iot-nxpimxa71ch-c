# IBM Watson™ IoT Platform C Client Library 
## For _NXP i_._MX_ Platform with IC A71CH Secure Element

This project contains C client library source that can be built and installed on devices
installed with _NXP i_._MX_ operating system, and interact with  the IBM Watson™ Internet of 
Things Platform. The library is customized to support 
*[NXP A71CH](https://www.nxp.com/products/identification-and-security/authentication/plug-and-trust-the-fast-easy-way-to-deploy-secure-iot-connections:A71CH)* Secure Element.

## Prerequisite

The client code code can be built only on _NXP i_._MX_ Platform. You need a working 
combination of *MCIMX6UL-EVKB* and *A71CH* boards. 

<img src="nxpa71ch.jpg" alt="NXP MCIMX6UL-EVKB and A71CH Board" style="display:block;margin:auto;width:400px;height:400px;"/>

You must prepare and provision the board for building client code and connecting client 
to IBM Watson IoT Platform. Refer to the following documentation for 
[Provisoing A71CH](./provision_a71ch_for_watson_iot_demo.md) process.
Note that the tools needed to build the client library and samples, 
come pre-installed on the SD card image for *MCIMX6UL-EVKB* board.

To test device connectivity to your IBM Watson™ IoT organization, you need an
IBM Cloud account and an instance of IBM Watson IoT service in your IBM Cloud organization.

If you do not have an IBM Cloud account, [sign up](https://console.bluemix.net/registration/) for an IBM Cloud account.

If you do not have an instance of IBM Watson IoT Platform service, 
[create an instance](https://console.bluemix.net/catalog/services/internet-of-things-platform/)
in your IBM Cloud organization.

## Build requirements / compilation

The client code code can be built only on _NXP i_._MX_ Platform. The code is dependent on
[Paho MQTT C Client](http://www.eclipse.org/paho/clients/c/) version 1.2.0. The build
script will automatically download dependent packages, so internet access is a must
during build process.

### Download C Client Library Source

Use the following command to download the C Client Library source:

```
git clone https://github.com/ibm-watson-iot/iot-nxpimxa71ch-c
```

If git is not available on the system, use curl command to download project zip file:

```
curl -LJO https://github.com/ibm-watson-iot/iot-nxpimxa71ch-c/archive/master.zip
unzip iot-nxpimxa71ch-c.master.zip
mv iot-nxpimxa71ch-c.master iot-nxpimxa71ch-c
```

### Build and install Steps

Use the following commands to setup build tree and build C Client library and samples:

```
cd iot-nxpimxa71ch-c
make build
make install
```

Install step will install client library, header files, sample client binaries, configuration,
and certificates in the following directory locations:

| Directory Location | Content |
| ------------------ |:------- |
| /usr/local/lib | Client libraries |
| /usr/local/include | Header files for device client build |
| /opt/iotnxpimxclient/bin | Device client sample binaries |
| /opt/iotnxpimxclient/config | Configuration files for Device client samples |
| /opt/iotnxpimxclient/certs | Certificate used by Device client samples |

### Verify build by connecting to *Quickstart*

*Quickstart* is an open sandbox that you can use to quickly connect your device 
to the IBM Watson™ IoT Platform. 

1. Connect to *[quickstart](https://quickstart.internetofthings.ibmcloud.com/?cm_mc_uid=71367544061615028292336&cm_mc_sid_50200000=59540641520868549701#/)* organization of IBM Watson™ Internet of Things Platform.

* Select "I accept IBM's Terms of Use"
* Specify a device id to test connectivity. e.g. *70028004006194989053*. Note that this device ID 
need not be the actual device ID stored in *IC A71CH* Secure Element. 
* Click *Go* button.

2. On Device system, run the following commands:

```
cd /opt/iotnxpimxclient/bin
./helloWorld --config /opt/iotnxpimxclient/config/device_quickstart.cfg
```
The test device will get connected to *Quickstart* and the Quickstart page will start showing 
incoming simulated sensor data from the test device.


## Connect A71CH to *your own organization*

Use the following steps to register device type, device ID and CA certificate with Watson IoT
Platform, configure device and connect to Watson IoT Platform:

### Register Device Type and Device:

Follow Step 1 described in the following link to register NXP-A71CH generic device type 
and your device ID (as returned by the A71CH provisioning script "provisionA71CH_WatsonIoT.sh") 
with Watson IoT Platform:

[Step 1: Registering your device with Watson IoT Platform](https://console.bluemix.net/docs/services/IoT/iotplatform_task.html#iotplatform_task)


### Register Certificate Authority

Register CA certificate created by A71CH provisioning script, with Watson IoT Platform.
Follow the instructions in the following section of Configuring certificates documentation:

[Registering Certificate Authority (CA) certificates for device authentication](https://console.bluemix.net/docs/services/IoT/reference/security/set_up_certificates.html#set_up_certificates)

### Configure device

Update device configuration file: On device system, edit *device_a71ch.cfg* file in */opt/iotnxpimxclient/config* directory:

```
org=<orgid>
type=NXP-A71CH
id=<your device id - returned by A71CH provisioning script>
useClientCertificates=1
clientCertPath=/home/root/tools/<your device id>_device_ec_pem.crt
clientKeyPath=/home/root/tools/<your device id>_device.ref_key
rootCACertPath=/opt/iotnxpimxclient/certs/IoTFoundation.pem
useNXPEngine=1
```

### Connect client 

Use the following commands to connect your sample client to Watson IoT Platform:

```
cd /opt/iotnxpimxclient/bin
./helloWorld --config /opt/iotnxpimxclient/config/device_a71ch.cfg
```


