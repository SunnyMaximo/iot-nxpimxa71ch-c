# Watson IoT Platform C Client Library 
## for *NXP i.MX* Platform with *IC A71CH* Secured Element

This project contains C client library source that can be built and installed on devices
installed with *NXP i.MX* operating system, to interact with  the IBM Watson Internet of 
Things Platform. The library is customized to support 
*[NXP A71CH](https://www.nxp.com/products/identification-and-security/authentication/plug-and-trust-the-fast-easy-way-to-deploy-secure-iot-connections:A71CH)* Secure Element.

Some of the source code in this client library is from the IBM Watson Internet of Things
[Embedded C Client library](https://github.com/ibm-watson-iot/iot-embeddedc).


## Build requirements / compilation

The client code code can be built only on *NXP i.MX* Platform. The code is dependent on
[Paho MQTT C Client](http://www.eclipse.org/paho/clients/c/) version 1.2.0. The build
script will automatically download dependent packages, so internet access is a must
during build process.

## Download C Client Library Source

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

## Build and install Steps

Use the following commands to setup build tree and build C Client library and samples:

```
cd iot-nxpimxa71ch-c
make build
make install
```

Install step will install client library, header files and sample device clients in the
following location:

```
/usr/local/lib - Client libraries
/usr/local/include - Header files for device client build
/opt/iotnxpimxclient/bin - Device client sample binaries
/opt/iotnxpimxclient/config - Configuration files for Device client samples
/opt/iotnxpimxclient/certs - Certificate used by Device client samples
/usr/local/lib and /usr/local/include directories respectively.
```

### Verify build by connecting to *Quickstart*

*Quickstart* is an open sandbox that you can use to quickly connect your device 
to the IBM Watson™ IoT Platform. 

1. Connect to *[quickstart](https://quickstart.internetofthings.ibmcloud.com/?cm_mc_uid=71367544061615028292336&cm_mc_sid_50200000=59540641520868549701#/)* organization of IBM Watson Internet of Things Platform.
..* Select "I accept IBM's Terms of Use"
..* Specify a device id to test connectivity. e.g. *testID11223344556677*. Note that this is not the
actual device ID stored in IC A71CH Secure Element.
..* Click *Go* button.

2. On Device system, run the following commands:

```
cd /opt/iotnxpimxclient/bin
./helloWorld --config /opt/iotnxpimxclient/config/device_quickstart.cfg
```
The device will get connected to *Quickstart* and the Quickstart page will start showing 
incoming sensor data from the device.


## Test client connectivity to *your own organization*

Before you can test device connectivity, you need to register for IBM Cloud account and create
an instance of the Watson Internet of Things Platform service. For registration details, 
[click here.](https://console.bluemix.net/docs/services/IoT/index.html#gettingstartedtemplate).


* Work in progress - details will be added soon *


