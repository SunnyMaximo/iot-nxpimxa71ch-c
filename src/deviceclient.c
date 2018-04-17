/*******************************************************************************
 * Copyright (c) 2017-2018 IBM Corp.
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
 * NOTE: This code is taken from iot-embeddedc project and customized to
 *       support NXP SSL Engine.
 *
 * iot-embeddedc project Contributors:
 *    Lokesh Haralakatta  -  Initial implementation
 *                        -  Contains the device client specific functions
 *                        -  Added logging feature
 *
 * ----------------------------------------------------------------------------
 * Contrinutors for NXP Engine changes:
 *    Ranjan Dasgupta         - Initial changes to support NXP Engine
 *                            - Code cleanup/refactor and logging support
 *
 *******************************************************************************/

#include "iotfclient.h"
#include "iotf_utils.h"


/**
 * Function used to Publish events from the device to the IBM Watson IoT service
 * @param eventType - Type of event to be published e.g status, gps
 * @param eventFormat - Format of the event e.g json
 * @param data - Payload of the event
 * @param QoS - qos for the publish event. Supported values : QoS0, QoS1, QoS2
 *
 * @return int return code from the publish
 */
int publishEvent(iotfclient  *client, char *eventType, char *eventFormat, char* data, QoS qos)
{
    LOG(TRACE, "entry::");

    int rc = -1;

    char publishTopic[strlen(eventType) + strlen(eventFormat) + 16];
    sprintf(publishTopic, "iot-2/evt/%s/fmt/%s", eventType, eventFormat);

    LOG(DEBUG,"Calling publishData to publish to topic - %s",publishTopic);

    rc = publishData(client,publishTopic,data,qos);

    if (rc != MQTTCLIENT_SUCCESS) {
 	LOG(WARN, "Connection lost, retry the connection \n");
        retry_connection(client);
        rc = publishData(client,publishTopic,data,qos);
    }

    LOG(TRACE, "exit:: rc=%d", rc);

    return rc;
}

 /**
 * Function used to subscribe to a device command.
 *
 * @return int return code
 */
int subscribeCommand(iotfclient  *client, char *commandName, char *format, int qos)
{
    LOG(TRACE, "entry::");

    int rc = -1;

    /* Sanity check */
    if ( !commandName || *commandName == '\0' || !format || *format == '\0' ) {
        LOG(WARN, "Invalid or NULL arguments");
        return rc;
    }

    LOG(DEBUG,"Calling MQTTClient_subscribe for subscribing to device command: name=%s format=%s qos=%d", commandName, format, qos);
    rc = MQTTClient_subscribe(client->c, "iot-2/cmd/%s/fmt/%s", QoS0);

    LOG(TRACE, "exit:: rc=%d", rc);
    return rc;
}

/**
 * Function used to subscribe to all device commands.
 *
 * @return int return code
 */
int subscribeCommands(iotfclient  *client)
{
    LOG(TRACE, "entry::");

    int rc = -1;
    LOG(DEBUG,"Calling MQTTClient_subscribe for subscribing to all device commands");
    rc = MQTTClient_subscribe(client->c, "iot-2/cmd/+/fmt/+", QoS0);

    LOG(TRACE, "exit:: rc=%d", rc);
    return rc;
}

