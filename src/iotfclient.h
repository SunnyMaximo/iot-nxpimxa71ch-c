/*******************************************************************************
 * Copyright (c) 2015-2018 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 *
 *
 * NOTE: This code is taken from iot-embeddedc project and customized to
 *       support NXP SSL Engine.
 *
 * iot-embeddedc project Contributors:
 *    Jeffrey Dare            - initial implementation and API implementation
 *    Sathiskumar Palaniappan - Added support to create multiple Iotfclient
 *                              instances within a single process
 *    Lokesh Haralakatta      - Added SSL/TLS support
 *    Lokesh Haralakatta      - Added Client Side Certificates support
 *    Lokesh Haralakatta      - Separated out device client and gateway client specific code.
 *                            - Retained back common code here.
 *
 * ----------------------------------------------------------------------------
 * Contrinutors for NXP Engine changes:
 *    Ranjan Dasgupta         - Initial changes to support NXP Engine
 *
 *******************************************************************************/

#if defined(__cplusplus)
 extern "C" {
#endif
#if !defined(IOTCLIENT_H_)
#define IOTCLIENT_H_

#define DLLImport extern
#define DLLExport __attribute__ ((visibility ("default")))

#include <stdio.h>

#define BUFFER_SIZE      1024
#define MAX_SUBSCRIPTION 5

/**
 * Define the log levels.
 */
typedef enum LOGLEVEL {
    LOGLEVEL_ERROR   = 1,
    LOGLEVEL_WARN    = 2,
    LOGLEVEL_INFO    = 3,
    LOGLEVEL_DEBUG   = 4,
    LOGLEVEL_TRACE   = 5
} LOGLEVEL;

enum errorCodes { CONFIG_FILE_ERROR = -3, MISSING_INPUT_PARAM = -4, QUICKSTART_NOT_SUPPORTED = -5, SE_CERT_ERROR = -6 };

typedef enum { QoS0, QoS1, QoS2 } QoS;

extern unsigned short keepAliveInterval;
extern char *sourceFile;

/* configuration file structure */
struct iotf_config
{
    char* org;
    char* domain;
    char* type;
    char* id;
    char* authmethod;
    char* authtoken;
    char* serverCertPath;
    char* rootCACertPath;
    char* clientCertPath;
    char* clientKeyPath;
    int port;
    int useClientCertificates;
    int useNXPEngine;
    int useCertsFromSE;
};

typedef struct iotf_config Config;


/* iotfclient */
typedef struct
{
    void *c;
    Config cfg;
    unsigned char buf[BUFFER_SIZE];
    unsigned char readbuf[BUFFER_SIZE];
    int isQuickstart;
    int isGateway;
    int managed;
} iotfclient;

/* Callback used to process commands */
typedef void (*commandCallback)(char* type, char* id, char* commandName, char *format, void* payload, size_t payloadlen);

/* Callback used to process device management commands */
typedef void (*dmCommandCallback)(char* status, char* requestId, void* payload, size_t payloadlen);

/* Action callback */
typedef void (*dmActionCallback)();

/**
* Function used to initialize the Watson IoT client
* @param client - Reference to the Iotfclient
* @param org - Your organization ID
* @param domain - Your domain Name
* @param type - The type of your device
* @param id - The ID of your device
* @param auth-method - Method of authentication (the only value currently supported is token)
* @param auth-token - API key token (required if auth-method is token)
* @Param serverCertPath - Custom Server Certificate Path
* @Param useCerts - Flag to indicate whether to use client side certificates for authentication
* @Param rootCAPath - if useCerts is 1, Root CA certificate Path
* @Param clientCertPath - if useCerts is 1, Client Certificate Path
* @Param clientKeyPath - if useCerts is 1, Client Key Path
* @Param isGatewayClient - 0 for device client or 1 for gateway client
* @Param useNXPEngine - Flag to indicate whether to use NXP Engine
* @Param useCertsFromSE - Flag to indicate whether to retrieve client certificates and public key from SE
*
* @return int return code
*/
DLLExport int initialize(iotfclient *client, char *orgId, char *domain, char *deviceType, char *deviceId,
              char *authmethod, char *authtoken, char *serverCertPath, int useCerts,
              char *rootCAPath, char *clientCertPath,char *clientKeyPath, int isGatewayClient, 
              int useNXPEngine, int useCertsFromSE);

/**
* Function used to initialize the IBM Watson IoT client using the config file which is generated when you register your device
* @param client - Reference to the Iotfclient
* @param configFilePath - File path to the configuration file
* @Param isGatewayClient - 0 for device client or 1 for gateway client
*
* @return int return code
* error codes
* CONFIG_FILE_ERROR -3 - Config file not present or not in right format
*/
DLLExport int initialize_configfile(iotfclient *client, char *configFilePath, int isGatewayClient);

/**
* Function used to initialize the IBM Watson IoT client
* @param client - Reference to the Iotfclient
*
* @return int return code
*/
DLLExport int connectiotf(iotfclient *client);

/**
 * Function used to set the Command Callback function. This must be set if you to recieve commands.
 * @param client - Reference to the Iotfclient
 *
 * @param cb - A Function pointer to the commandCallback. 
 *             
 * @return int return code
 */
DLLExport void setCommandHandler(iotfclient *client, commandCallback cb);

/**
* Function used to publish the given data to the topic with the given QoS
* @Param client - Address of Iotf Client
* @Param topic - Topic to publish
* @Param payload - Message payload
* @Param qos - quality of service either of 0,1,2
*
* @return int - Return code from MQTT Publish Call
**/
DLLExport int publishData(iotfclient *client, char *topic, char *payload, int qos);

/**
* Function used to subscribe to a topic with the given QoS
* @Param client - Address of Iotf Client
* @Param topic - Topic to subscribe
* @Param qos - quality of service either of 0,1,2
*
* @return int - Return code from MQTT Subscribe Call
**/
DLLExport int subscribeTopic(iotfclient *client, char *topic, int qos);

/**
* Function used to check if the client is connected
* @param client - Reference to the Iotfclient
*
* @return int return code
*/
DLLExport int isConnected(iotfclient *client);

/**
* Function used to Yield for commands.
* @param time_ms - Time in milliseconds
* @return int return code
*/
DLLExport int yield(int time_ms);

/**
* Function used to disconnect from the IBM Watson IoT service
* @param client - Reference to the Iotfclient
*
* @return int return code
*/
DLLExport int disconnect(iotfclient *client);

/**
* Function used to set the time to keep the connection alive with IBM Watson IoT service
* @param keepAlive - time in secs
*
*/
DLLExport void setKeepAliveInterval(unsigned int keepAlive);


/**
 * Function used to subscribe to device command.
 * @param client - Reference to the Iotfclient
 * @param commandName - Name of command - accepts MQTT wild card character "+"
 * @param format - Reponse format, e.g. JSON - accepts MQTT wild card character "+"
 * @param qos - Quality of service QoS0, QoS1, QoS2
 *
 * @return int return code
 */
DLLExport int subscribeCommand(iotfclient *client, char *commandName, char *format, int qos);

/**
 * Function used to subscribe to all commands.
 * @param client - Reference to the Iotfclient
 *
 * @return int return code
 */
DLLExport int subscribeCommands(iotfclient *client);

/**
 * Function used to Publish events from the device to the IBM Watson IoT service
 * @param client - Reference to the Iotfclient
 * @param eventType - Type of event to be published e.g status, gps
 * @param eventFormat - Format of the event e.g json
 * @param data - Payload of the event
 * @param QoS - qos for the publish event. Supported values : QoS0, QoS1, QoS2
 *
 * @return int return code from the publish
 */
DLLExport int publishEvent(iotfclient *client, char *eventType, char *eventFormat, char* data, QoS qos);

/**
* Function used to Publish events from the device to the Watson IoT
* @param client - Reference to the GatewayClient
* @param eventType - Type of event to be published e.g status, gps
* @param eventFormat - Format of the event e.g json
* @param data - Payload of the event
* @param QoS - qos for the publish event. Supported values : QoS0, QoS1, QoS2
*
* @return int return code from the publish
*/
DLLExport int publishGatewayEvent(iotfclient  *client, char *eventType, char *eventFormat, char* data, QoS qos);

/**
* Function used to Publish events from the device to the Watson IoT
* @param client - Reference to the GatewayClient
* @param deviceType - The type of your device
* @param deviceId - The ID of your deviceId
* @param eventType - Type of event to be published e.g status, gps
* @param eventFormat - Format of the event e.g json
* @param data - Payload of the event
* @param QoS - qos for the publish event. Supported values : QoS0, QoS1, QoS2
*
* @return int return code from the publish
*/
DLLExport int publishDeviceEvent(iotfclient  *client, char *deviceType, char *deviceId, char *eventType, char *eventFormat, char* data, QoS qos);

/**
* Function used to subscribe to all commands for the Gateway.
* @param client - Reference to the GatewayClient
*
* @return int return code
*/
DLLExport int subscribeToGatewayCommands(iotfclient  *client);

/**
* Function used to subscribe to device commands in a  gateway.
*
* @return int return code
*/
DLLExport int subscribeToDeviceCommands(iotfclient  *client, char* deviceType, char* deviceId, char* command, char* format, int qos) ;


/**
* <p>Send a device manage request to Watson IoT Platform</p>
*
* <p>A Device uses this request to become a managed device.
* It should be the first device management request sent by the
* Device after connecting to the IBM Watson IoT Platform.
* It would be usual for a device management agent to send this
* whenever is starts or restarts.</p>
*
* @param lifetime The length of time in seconds within
*        which the device must send another Manage device request.
*        if set to 0, the managed device will not become dormant.
*        When set, the minimum supported setting is 3600 (1 hour).
*
* @param supportFirmwareActions Tells whether the device supports firmware actions or not.
*        The device must add a firmware handler to handle the firmware requests.
*
* @param supportDeviceActions Tells whether the device supports Device actions or not.
*        The device must add a Device action handler to handle the reboot and factory reset requests.
*
* @param reqId Function returns the reqId if the publish Manage request is successful.
*
* @return
*/
DLLExport void manage(iotfclient *client, long lifetime, int supportDeviceActions, int supportFirmwareActions, char* reqId);


/**
 * Moves the device from managed state to unmanaged state
 *
 * A device uses this request when it no longer needs to be managed.
 * This means Watson IoT Platform will no longer send new device management requests
 * to this device and device management requests from this device will
 * be rejected apart from a Manage device request
 *
 * @param reqId Function returns the reqId if the Unmanage request is successful.
 */
DLLExport void unmanage(iotfclient *client, char* reqId);


/**
 * Register Callback function to managed device request response
 *
 * @param cb - A Function pointer to the dmCommandCallback.
 *
 */
DLLExport void setDMCommandHandler(iotfclient  *client, dmCommandCallback cb);

/**
 * Register Callback function to Factory reset request
 *
 * @param client reference to the ManagedDevice
 *
 * @param cb - A Function pointer to the dmCommandCallback.
 *
 */
DLLExport void setFactoryResetHandler(dmCommandCallback cb);

/**
 * Register Callback function to Reboot request
 *
 * @param cb - A Function pointer to the dmCommandCallback.
 *
 */
DLLExport void setRebootHandler(dmCommandCallback cb);

/**
 * Register Callback function to Firmware Download request
 *
 * @param cb - A Function pointer to the dmActionCallback.
 *
 */
DLLExport void setFirmwareDownloadHandler(dmActionCallback cb);

/**
 * Register Callback function to Firmware Update request
 *
 * @param cb - A Function pointer to the dmActionCallback.
 *
 */
DLLExport void setFirmwareUpdateHandler(dmActionCallback cb);

/**
 * Update the firmware state while downloading firmware and
 * Notifies the IBM Watson IoT Platform with the updated state
 *
 * @param state Download state update received from the device
 *
 * @return int return code
 *
 */
DLLExport int changeFirmwareDownloadState(int state);

/**
 * Update the firmware state while updating firmware and
 * Notifies the IBM Watson IoT Platform with the updated state
 *
 * @param state update state update received from the device
 *
 * @return int return code
 *
 */
DLLExport int changeFirmwareUpdateState(int state);


/**
 * Function to enable logging and set log level.
 *
 * @param - level       - Log level
 * @param - logFilePath - Log file path
 * @return - None
 **/
DLLExport void initLogging(const LOGLEVEL level, const char *logFilePath);

/** Utility Functions **/
DLLExport char *trim(char *str);
DLLExport int retry_connection(iotfclient  *client);
DLLExport int reconnect_delay(int i);

DLLExport int subscribeToGatewayNotification(iotfclient  *client);

/** Retrieve certificates and key from Secure Element using NXP A71CH APIs */
DLLExport char * a71ch_retrieveCertificatesFromSE(char * certDir);

#endif /* IOTCLIENT_H_ */
