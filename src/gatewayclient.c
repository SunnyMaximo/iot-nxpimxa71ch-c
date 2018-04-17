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
 *                            - Added logging feature
 * ----------------------------------------------------------------------------
 * Contrinutors for NXP Engine changes:
 *    Ranjan Dasgupta         - Initial changes to support NXP Engine
 *                            - Code cleanup/refactor and logging support
 *
 *******************************************************************************/

#include "iotfclient.h"
#include "iotf_utils.h"

/* Subscription details storage */
char* subscribeTopics[MAX_SUBSCRIPTION];
int subscribeCount = 0;

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
int publishDeviceEvent(iotfclient  *client, char *deviceType, char *deviceId, char *eventType, char *eventFormat, char* data, QoS qos)
{
    LOG(TRACE, "entry::");

    int rc = -1;

    char publishTopic[strlen(eventType) + strlen(eventFormat) + strlen(deviceType) + strlen(deviceId)+25];

    sprintf(publishTopic, "iot-2/type/%s/id/%s/evt/%s/fmt/%s", deviceType, deviceId, eventType, eventFormat);

    LOG(DEBUG, "Calling publishData to publish to topic - %s",publishTopic);

    rc = publishData(client, publishTopic, data, qos);

    if (rc != MQTTCLIENT_SUCCESS) {
        LOG(WARN, "connection lost.. %d \n",rc);
        retry_connection(client);
        rc = publishData(client, publishTopic, data, qos);
    }

    LOG(TRACE, "exit:: rc=%d", rc);

    return rc;
}

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
int publishGatewayEvent(iotfclient  *client, char *eventType, char *eventFormat, char* data, QoS qos)
{
    LOG(TRACE, "entry::");

    int rc = -1;

    char publishTopic[strlen(eventType) + strlen(eventFormat) + strlen(client->cfg.id) + strlen(client->cfg.type)+25];

    sprintf(publishTopic, "iot-2/type/%s/id/%s/evt/%s/fmt/%s", client->cfg.type, client->cfg.id, eventType, eventFormat);

    LOG(DEBUG, "Calling publishData to publish to topic - %s",publishTopic);

    rc = publishData(client, publishTopic , data, qos);

    if (rc != MQTTCLIENT_SUCCESS) {
        LOG(WARN, "connection lost.. \n");
        retry_connection(client);
        rc = publishData(client, publishTopic , data, qos);
    }

    LOG(TRACE, "exit:: rc = %d",rc);

    return rc;
}

/**
* Function used to subscribe to all commands for gateway.
*
* @return int return code
*/
int subscribeToGatewayCommands(iotfclient  *client)
{
    LOG(TRACE, "entry::");

    int rc = -1;
    char* subscribeTopic = NULL;

    subscribeTopic = (char*) malloc(strlen(client->cfg.id) + strlen(client->cfg.type) + 28);

    sprintf(subscribeTopic, "iot-2/type/%s/id/%s/cmd/+/fmt/+", client->cfg.type, client->cfg.id);

    LOG(DEBUG, "Calling MQTTClient_subscribe for subscribing to gateway commands");

    rc = MQTTClient_subscribe(client->c, subscribeTopic, QoS2);

    subscribeTopics[subscribeCount++] = subscribeTopic;

    LOG(TRACE,"exit:: rc=%d", rc);

    return rc;
}

/**
* Function used to subscribe to device commands for gateway.
*
* @return int return code
*/
int subscribeToDeviceCommands(iotfclient  *client, char* deviceType, char* deviceId, char* command, char* format, int qos)
{
    LOG(TRACE, "entry::");

    int rc = -1;
    char * subscribeTopic = NULL;

    subscribeTopic = (char*)malloc(strlen(deviceType) + strlen(deviceId) + strlen(command) + strlen(format) + 26);
    sprintf(subscribeTopic, "iot-2/type/%s/id/%s/cmd/%s/fmt/%s", deviceType, deviceId, command, format);

    LOG(DEBUG, "Calling MQTTSubscribe for subscribing to device commands: %s", subscribeTopic);

    rc = MQTTClient_subscribe(client->c, subscribeTopic, qos);
    subscribeTopics[subscribeCount++] = subscribeTopic;

    LOG(TRACE,"exit:: rc=%d", rc);
    return rc;
}

/**
* Function used to disconnect from the IoTF service
*
* @return int return code
*/
void freeGatewaySubscriptionList(iotfclient  *client)
{
    LOG(TRACE, "entry::");

    int count;

    /* free memory for subscriptions - for gateways */
    if ( client->isGateway ) {
        for (count = 0; count < subscribeCount ; count++)
            free(subscribeTopics[count]);
    }

    LOG(TRACE,"exit::");
}

/**
* Function used to subscribe to gateway notifications
*
* @return int return code
*/
int subscribeToGatewayNotification(iotfclient  *client)
{
    LOG(TRACE, "entry::");

    int rc = -1;
    char * subscribeTopic = NULL;

    subscribeTopic = (char*) malloc(strlen(client->cfg.id) + strlen(client->cfg.type) + 23);
    sprintf(subscribeTopic, "iot-2/type/%s/id/%s/notify", client->cfg.type, client->cfg.id);

    LOG(DEBUG, "Calling MQTTClient_subscribe for subscribing to gateway notification");

    rc = MQTTClient_subscribe(client->c, subscribeTopic, QoS2);
    subscribeTopics[subscribeCount++] = subscribeTopic;

    LOG(TRACE,"exit:: rc=%d", rc);
    return rc;
}

