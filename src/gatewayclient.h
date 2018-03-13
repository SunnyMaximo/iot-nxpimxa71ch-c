/*******************************************************************************
 * Copyright (c) 2016-2018 IBM Corp.
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
 *    Jeffrey Dare            - initial implementation
 *    Lokesh Haralakatta      - Added SSL/TLS support
 *    Lokesh Haralakatta      - Added Client Side Certificates support
 *    Lokesh Haralakatta      - Separated out device client and gateway client specific code.
 *                            - Retained gateway specific code here.
 * ----------------------------------------------------------------------------
 * Contrinutors for NXP Engine changes:
 *    Ranjan Dasgupta         - Initial changes to support NXP Engine
 *
 *******************************************************************************/

#ifndef GATEWAYCLIENT_H_
#define GATEWAYCLIENT_H_

#include "iotfclient.h"

//Callback used to process commands
typedef void (*commandCallback)(char* type, char* id, char* commandName, char *format,
              void* payload, size_t payloadlen);


/**
* Function used to Publish events from the device to the Watson IoT
* @param client - Reference to the GatewayClient
* @param eventType - Type of event to be published e.g status, gps
* @param eventFormat - Format of the event e.g json
* @param data - Payload of the event
* @param QoS - qos for the publish event. Supported values : QOS0, QOS1, QOS2
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
* @param QoS - qos for the publish event. Supported values : QOS0, QOS1, QOS2
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
* Function used to disconnect from the IBM Watson IoT
* @param client - Reference to the GatewayClient
*
* @return int return code
*/
DLLExport int disconnectGateway(iotfclient  *client);

/**
* Function called upon gateway receiving the message from IoT Platform.
**/
DLLExport void gatewayMessageArrived(void *context, char *topicName, int topicLen, MQTTClient_message * message);

/**
* Function used to set the Command Callback function. This must be set if you to recieve commands.
* @param client - Reference to the GatewayClient
*
* @param cb - A Function pointer to the commandCallback. Its signature - void (*commandCallback)(char* commandName, char* payload)
* @return int return code
*/
DLLExport void setGatewayCommandHandler(iotfclient *client, commandCallback cb);

/** Utility Functions **/
DLLExport char *trim(char *str);
DLLExport int retry_connection(iotfclient  *client);
DLLExport int reconnect_delay(int i);

#endif /* GATEWAYCLIENT_H_ */
