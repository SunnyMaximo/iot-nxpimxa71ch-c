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
 *
 *******************************************************************************/

 #include "deviceclient.h"

 //Command Callback
 commandCallback cb;

//Character strings to hold log header and log message to be dumped.
 char logHdr[LOG_BUF];
 char logStr[LOG_BUF];

 /**
 * Function used to Publish events from the device to the IBM Watson IoT service
 * @param eventType - Type of event to be published e.g status, gps
 * @param eventFormat - Format of the event e.g json
 * @param data - Payload of the event
 * @param QoS - qos for the publish event. Supported values : QOS0, QOS1, QOS2
 *
 * @return int return code from the publish
 */

 int publishEvent(iotfclient  *client, char *eventType, char *eventFormat, char* data, QoS qos)
 {
        sprintf(logHdr,"%s:%d:%s:",__FILE__,__LINE__,__func__);
        LOG(logHdr,"entry::");

        int rc = -1;

 	char publishTopic[strlen(eventType) + strlen(eventFormat) + 16];

 	sprintf(publishTopic, "iot-2/evt/%s/fmt/%s", eventType, eventFormat);

        sprintf(logHdr,"%s:%d:%s",__FILE__,__LINE__,__func__);
        sprintf(logStr,"Calling publishData to publish to topic - %s",publishTopic);
        LOG(logHdr,logStr);

 	rc = publishData(client,publishTopic,data,qos);

 	if(rc != SUCCESS) {
 		printf("\nConnection lost, retry the connection \n");
 		retry_connection(client);
 		rc = publishData(client,publishTopic,data,qos);
 	}

        sprintf(logHdr,"%s:%d:%s",__FILE__,__LINE__,__func__);
        sprintf(logStr,"rc = %d",rc);
        LOG(logHdr,logStr);
        LOG(logHdr,"exit::");

 	return rc;

 }

 /**
 * Function used to subscribe to all device commands.
 *
 * @return int return code
 */
 int subscribeCommands(iotfclient  *client)
 {
        sprintf(logHdr,"%s:%d:%s:",__FILE__,__LINE__,__func__);
        LOG(logHdr,"entry::");

        int rc = -1;

        sprintf(logHdr,"%s:%d:%s",__FILE__,__LINE__,__func__);
        sprintf(logStr,"Calling MQTTClient_subscribe for subscribing to device commands");
        LOG(logHdr,logStr);

        rc = MQTTClient_subscribe(&client->c, "iot-2/cmd/+/fmt/+", QOS0);

        sprintf(logHdr,"%s:%d:%s",__FILE__,__LINE__,__func__);
        sprintf(logStr,"RC from MQTTClient_subscribe - %d",rc);
        LOG(logHdr,logStr);
        LOG(logHdr,"exit::");

        return rc;
 }

 //Handler for all commands. Invoke the callback.
void messageArrived(void *context, char *topicName, int topicLen, MQTTClient_message * message)
{
        sprintf(logHdr,"%s:%d:%s:",__FILE__,__LINE__,__func__);
        LOG(logHdr,"entry::");

 	if(cb != 0) {
 		char *topic = malloc(topicLen+1);

 		sprintf(topic,"%.*s", topicLen, topicName);

 		void *payload = message->payload;

 		strtok(topic, "/");
 		strtok(NULL, "/");

 		char *commandName = strtok(NULL, "/");
 		strtok(NULL, "/");
 		char *format = strtok(NULL, "/");

                sprintf(logHdr,"%s:%d:%s",__FILE__,__LINE__,__func__);
                sprintf(logStr,"Calling registered callabck to process the arrived message");
                LOG(logHdr,logStr);

 		(*cb)(commandName, format, payload);

                MQTTClient_freeMessage(&message);
                MQTTClient_free(topicName);

 		free(topic);

 	}
        else{
                sprintf(logHdr,"%s:%d:%s",__FILE__,__LINE__,__func__);
                sprintf(logStr,"No registered callback function to process the arrived message");
                LOG(logHdr,logStr);
        }

        sprintf(logHdr,"%s:%d:%s",__FILE__,__LINE__,__func__);
        sprintf(logStr,"Returning from %s",__func__);
        LOG(logHdr,logStr);
        LOG(logHdr,"exit::");
 }

 /**
 * Function used to set the Command Callback function. This must be set if you to recieve commands.
 *
 * @param cb - A Function pointer to the commandCallback. Its signature - void (*commandCallback)(char* commandName, char* payload)
 * @return int return code
 */
 void setCommandHandler(iotfclient  *client, commandCallback handler)
 {
        sprintf(logHdr,"%s:%d:%s:",__FILE__,__LINE__,__func__);
        LOG(logHdr,"entry::");

        cb = handler;

        if(cb != NULL){
                sprintf(logHdr,"%s:%d:%s",__FILE__,__LINE__,__func__);
                sprintf(logStr,"Registered callabck to process the arrived message");
                LOG(logHdr,logStr);
        }
        else{
                sprintf(logHdr,"%s:%d:%s",__FILE__,__LINE__,__func__);
                sprintf(logStr,"Callabck not registered to process the arrived message");
                LOG(logHdr,logStr);
        }

        sprintf(logHdr,"%s:%d:%s",__FILE__,__LINE__,__func__);
        sprintf(logStr,"Returning from %s",__func__);
        LOG(logHdr,logStr);
        LOG(logHdr,"exit::");
 }
