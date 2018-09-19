# IBM Watson™ IoT Platform C Client Developer's Guide
## For _NXP i_._MX_ Platform with IC A71CH Secure Element

This document describes the C interface for communicating with the IBM Watson Internet of Things Platform
from NXP IC A71CH Secure Element enabled devices and gateways.

## C API for Devices

The `iotfclient` is client for the IBM Watson Internet of Things Platform service can be connected as a `device client`.
At the time of initialization, we need to specify the client type - `device client` or `gateway client`. We can use `device client` to connect to the Watson IoT platform, publish events and subscribe to commands.

### Initialize

Initialize the `iotfclient` as Device Client using a configuration file.

The function `initialize_configfile` takes pointer to `iotfclient`, the configuration file path and 0 for `clientType` as parameters:

``` {.sourceCode .c}
#include "deviceclient.h"
   ....
   ....
   char* configFilePath="./device.cfg";
   iotfclient client;

   rc = initialize_configfile(&client, configFilePath,0);
   ....
```

The configuration file must be in the below given format:

``` {.sourceCode .}
org=$orgId
type=NXP-A71CH-D
useClientCertificates=1
rootCACertPath=/opt/iotnxpimxclient/certs/IoTFoundation.pem
useNXPEngine=1
useCertsFromSE=1
```
Note that the iotfclient will retrieve device id from NXP A71CH secure element.

##### Return codes

Following are the return codes in the `initialize` function:

* CONFIG_FILE_ERROR   -3
* MISSING_INPUT_PARAM   -4


Connect
-------

After initializing the `iotfclient`, we can connect to the IBM Watson Internet of Things Platform by calling the `connectiotf` function

``` {.sourceCode .c}
#include "deviceclient.h"
   ....
   ....
   char* configFilePath="./device.cfg";
   iotfclient client;

   rc = initialize_configfile(&client, configFilePath,0);

   if(rc != SUCCESS){
       printf("initialize failed and returned rc = %d.\n Quitting..", rc);
       return 0;
   }

   rc = connectiotf(&client);

   if(rc != SUCCESS){
       printf("Connection failed and returned rc = %d.\n Quitting..", rc);
       return 0;
   }
   ....
```

##### Return Codes

The IoTF `connectiotf` function return codes are as shown below:

* MQTTCLIENT_SUCCESS   0
* MQTTCLIENT_FAILURE   -1
* MQTTCLIENT_DISCONNECTED   -3
* MQTTCLIENT_MAX_MESSAGES_INFLIGHT   -4
* MQTTCLIENT_BAD_UTF8_STRING   -5
* MQTTCLIENT_BAD_QOS   -9
* MQTTCLIENT_NOT_AUTHORIZED   5

Handling commands
-----------------

When the device client connects in registered mode, to process specific commands, we need to subscribe to commands by calling the function `subscribeCommands` and then register a command callback function by calling the function `setCommandHandler`.

The commands are returned as:
-   type - command type
-   id - command id
-   commandName - name of the command invoked
-   format - e.g json, xml
-   payload
-   payloadSize - payload size

``` {.sourceCode .c}
#include "deviceclient.h"
....
....
void  deviceCommandCallback (char* type, char* id, char* commandName, char *format, void* payload, size_t payloadSize)
{
    if ( type == NULL && id == NULL ) {
        fprintf(stdout, "Received device management status:");

    } else {
        fprintf(stdout, "Received device command:\n");
        fprintf(stdout, "Type=%s ID=%s CommandName=%s Format=%s Len=%d\n", type, id, commandName, format, (int)payloadSize);
        fprintf(stdout, "Payload: %s\n", (char *)payload);
    }

    /*
     * DEV_NOTES:
     * Device developers - add your custom code to process device commands
     */
}

int main(int argc, char *argv[]) {
    char* configFilePath="./device.cfg";
    iotfclient client;
    ....

    rc = initialize_configfile(&client, configFilePath,0);

    if (rc != SUCCESS){
        printf("initialize failed and returned rc = %d.\n Quitting..", rc);
        return 0;
    }

    rc = connectiotf(&client);

    if (rc != SUCCESS){
        printf("Connection failed and returned rc = %d.\n Quitting..", rc);
        return 0;
    }
 
    setCommandHandler(&client, deviceCommandCallback);

    char *commandName = "+";
    char *format = "+";
    subscribeCommand(&client, commandName, format, QoS0);

    ....
}

```

Publishing events
------------------

Events can be published by using the function `publishEvent`. The parameters to the function are:

-   client - iotf client
-   eventType - Type of event to be published e.g status, gps
-   eventFormat - Format of the event e.g json
-   data - Payload of the event
-   QoS - qos for the publish event. Supported values : QOS0, QOS1, QOS2

``` {.sourceCode .c}
#include "deviceclient.h"
 ....
 rc = connectiotf (&client);
 char *payload = {\"d\" : {\"temp\" : 34 }};

 rc = publishEvent(&client,"status","json", payload , QoS0);
 ....
```

Disconnect Client
------------------

Disconnects the client, releases the connections and frees the memory.

``` {.sourceCode .c}
#include "deviceclient.h"
 ....
 rc = connectiotf (&client);
 ...
 rc = disconnect(&client);
 ....
```

