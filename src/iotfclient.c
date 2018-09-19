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
 * NOTE: Part of the code included in this project is taken from iot-embeddedc 
 *       project and customized to support NXP SSL Engine.
 *
 * iot-embeddedc project Contributors:
 *    Jeffrey Dare, Sathiskumar Palaniappan, Lokesh Haralakatta
 *
 * ----------------------------------------------------------------------------
 * Contrinutors for NXP Engine changes:
 *    Ranjan Dasgupta         - Initial changes to support NXP Engine
 *                            - Code cleanup/refactor and logging support
 *
 *******************************************************************************/

#include <MQTTClient.h>

#include "iotfclient.h"
#include "iotf_utils.h"

static int  messageArrived(void *context, char *topicName, int topicLen, MQTTClient_message * message);
static void messageDelivered(void *context, MQTTClient_deliveryToken dt);
extern void freeGatewaySubscriptionList(iotfclient *client);
extern void freeConfig(Config *cfg);
extern int messageArrived_dm(void *context, char *topicName, int topicLen, void *payload, size_t payloadlen);

/* Command Callback */
commandCallback cb;
 
unsigned short keepAliveInterval = 60;
int a71chInited = 0;

/**
 * Callback function to handle connection lose cases
 */
void connlost(void *context, char *cause)
{
    LOG(TRACE, "entry::");
    LOG(WARN, "IoTF client connection is lost. Context=%x Cause=%s", context, cause);
    LOG(TRACE, "exit::");
}

/**
 * Function used to connect to the IBM Watson IoT client
 * @param client - Reference to the Iotfclient
 *
 * @return int return code
 */
int connectiotf(iotfclient  *client)
{
    LOG(TRACE, "entry::");

    int rc = 0;
    char *seUID = NULL;

    MQTTClient mqttClient;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_SSLOptions ssl_opts = MQTTClient_SSLOptions_initializer;

    int useCerts = client->cfg.useClientCertificates;
    int isGateway = client->isGateway;
    int qsMode = client->isQuickstart;
    int port = client->cfg.port;

    LOG(DEBUG, "useCerts:%d, isGateway:%d, qsMode:%d", useCerts, isGateway, qsMode);

    char messagingUrl[120];
    sprintf(messagingUrl, ".messaging.%s",client->cfg.domain);
    char hostname[strlen(client->cfg.org) + strlen(messagingUrl) + 1];
    sprintf(hostname, "%s%s", client->cfg.org, messagingUrl);

    /* If useNXPEngine is enabled set environment variable to load NXP engine */
    if ( client->cfg.useNXPEngine ) {
        char * envval;
        envval = getenv ("OPENSSL_CONF");

        if ( envval && *envval != '\0' && strstr(envval, "A71CH")) {
            LOG(INFO, "Library is already set to use NXP Openssl Engine.");
        } else {
            setenv ("OPENSSL_CONF", "/etc/ssl/opensslA71CH_i2c.cnf", 1);
            LOG(INFO, "Library is set to use NXP Openssl Engine.");
        }

        /* Retrieve certificates from SE if useCertsFromSE is set */
        if ( client->cfg.useCertsFromSE && a71chInited == 0 ) {
            /* Get certificate directory from client certificate path is specified
             * else use default certificate directory is /opt/iotnxpimxclient/certs
             */
            char *certDir = NULL;
            if ( client->cfg.clientCertPath != NULL ) {
                certDir = dirname(client->cfg.clientCertPath);
            }
            if ( certDir == NULL ) certDir = "/opt/iotnxpimxclient/certs";
            seUID = a71ch_retrieveCertificatesFromSE(certDir);
            if ( seUID == NULL ) {
                LOG(ERROR, "Failed to retrieve client certificate and key from Secure Element");
                rc = SE_CERT_ERROR;
                return rc;
            }

            LOG(INFO, "Retrieved client certificate and key for UID: %s", seUID);

            /* Use retrieved certificates */
            if (useCerts) {
                char certPath[2048];
                char keyPath[2048];

                /* set client cert */
                if (isGateway) {
                    sprintf(certPath, "%s/%s_gateway_ec_pem.crt", certDir, seUID);
                } else {
                   sprintf(certPath, "%s/%s_device_ec_pem.crt", certDir, seUID);
                }
                if ( client->cfg.clientCertPath != NULL ) free(client->cfg.clientCertPath);
                client->cfg.clientCertPath = (char *)strdup(certPath);

                /* set client reference key */
                sprintf(keyPath, "%s/%s.ref_key", certDir, seUID);
                if (client->cfg.clientKeyPath != NULL) free(client->cfg.clientKeyPath);
                client->cfg.clientKeyPath = (char *)strdup(keyPath);

                LOG(INFO, "Client certificate  from SE: clientCertPath=%s", client->cfg.clientCertPath);
                LOG(INFO, "Client refernce key from SE: clientKeyPath=%s", client->cfg.clientKeyPath);
            }

            a71chInited = 1;
        }
    }

    /* Use seUID if retrieved from SE */
    if ( seUID != NULL ) {
        if ( client->cfg.id != NULL ) {
            if ( !strcmp(seUID, client->cfg.id)) {
                LOG(INFO, "Specified id in config file matches with id of SE: %s", seUID);
            } else {
                LOG(WARN, "Ignoring specified id in config file: %s", client->cfg.id); 
                LOG(WARN, "Using id of SE: %s", seUID);
                free(client->cfg.id);
            }
        }
        client->cfg.id = strdup(seUID);
    }

    /* sanity check for client id */
    if ( client->cfg.id == NULL ) {
        LOG(ERROR, "Client ID is NULL");
        rc = SE_CERT_ERROR;
        return rc;
    }

    int clientIdLen =  strlen(client->cfg.org) + strlen(client->cfg.type) + strlen(client->cfg.id) + 5;
    char *clientId = malloc(clientIdLen);

    if (isGateway)
        sprintf(clientId, "g:%s:%s:%s", client->cfg.org, client->cfg.type, client->cfg.id);
    else
        sprintf(clientId, "d:%s:%s:%s", client->cfg.org, client->cfg.type, client->cfg.id);

    LOG(INFO, "messagingUrl=%s", messagingUrl);
    LOG(INFO, "hostname=%s", hostname);
    LOG(INFO, "port=%d", port);
    LOG(INFO, "clientId=%s", clientId);

    /* create MQTT Client */
    char connectionUrl[strlen(hostname) + 14];
    if ( port == 1883 )
       sprintf(connectionUrl, "tcp://%s:%d", hostname, port);
    else
       sprintf(connectionUrl, "ssl://%s:%d", hostname, port);

    LOG(INFO, "connectionUrl=%s", connectionUrl);

    rc = MQTTClient_create(&mqttClient, connectionUrl, clientId, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    if ( rc != 0 ) {
        LOG(WARN, "RC from MQTTClient_create:%d",rc);
        return rc;
    }

    client->c = (void *)mqttClient;

    /* set connection options */
    conn_opts.keepAliveInterval = keepAliveInterval;
    conn_opts.reliable = 0;
    conn_opts.cleansession = 1;
    ssl_opts.enableServerCertAuth = 0;

    if (!qsMode && client->cfg.authtoken ) {
        conn_opts.username = "use-token-auth";
        conn_opts.password = client->cfg.authtoken;
    }

    if ( port != 1883 ) {
        conn_opts.ssl = &ssl_opts;
        if (useCerts) {
            conn_opts.ssl->enableServerCertAuth = 1;
            conn_opts.ssl->trustStore = client->cfg.rootCACertPath;
            conn_opts.ssl->keyStore = client->cfg.clientCertPath;
            conn_opts.ssl->privateKey = client->cfg.clientKeyPath;
        }
    }

    /* Set callbacks */
    MQTTClient_setCallbacks((MQTTClient *)client->c, NULL, connlost, messageArrived, messageDelivered);
           
    if ((rc = MQTTClient_connect((MQTTClient *)client->c, &conn_opts)) == MQTTCLIENT_SUCCESS) {
        if (qsMode) {
            LOG(INFO, "Device Client Connected to %s Platform in QuickStart Mode\n",hostname);
        } else {
            char *clientType = (isGateway)?"Gateway Client":"Device Client";
            char *connType = (useCerts)?"Client Side Certificates":"Secure Connection";
            LOG(INFO, "%s Connected to %s using %s\n", clientType, hostname, connType);
        }
    }

    LOG(TRACE, "exit:: rc=%d", rc);
    return rc;
}


/**
* Function used to publish the given data to the topic with the given QoS
* @Param client - Address of MQTT Client
* @Param topic - Topic to publish
* @Param payload - Message payload
* @Param qos - quality of service either of 0,1,2
*
* @return int - Return code from MQTT Publish
**/
int publishData(iotfclient *client, char *topic, char *payload, int qos)
{
    LOG(TRACE, "entry::");

    int rc = -1;
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;
    int payloadlen = strlen(payload);

    pubmsg.payload = payload;
    pubmsg.payloadlen = payloadlen;
    pubmsg.qos = qos;
    pubmsg.retained = 0;

    LOG(DEBUG, "Publish Message: qos=%d retained=%d payloadlen=%d payload: %s",
                    pubmsg.qos, pubmsg.retained, pubmsg.payloadlen, payload);

    rc = MQTTClient_publishMessage((MQTTClient *)client->c, topic, &pubmsg, &token);
    LOG(DEBUG, "Message with delivery token %d delivered\n", token);

    LOG(TRACE, "exit:: rc=%d", rc);
    return rc;
}

/**
* Function used to subscribe to a topic to get command(s)
* @Param client - Address of MQTT Client
* @Param topic - Topic to publish
* @Param qos - quality of service either of 0,1,2
*
* @return int - Return code from MQTT Publish
**/
int subscribeTopic(iotfclient *client, char *topic, int qos)
{
    LOG(TRACE, "entry::");

    int rc = -1;
    LOG(DEBUG,"Calling MQTTClient_subscribe: topic=%s qos=%d", topic, qos);
    rc = MQTTClient_subscribe((MQTTClient *)client->c, topic, qos);

    LOG(TRACE, "exit:: rc=%d", rc);
    return rc;
}

/**
* Function used to Yield for commands.
* @param time_ms - Time in milliseconds
* @return int return code
*/
int yield(int time_ms)
{
    LOG(TRACE, "entry::");

    int rc = usleep(time_ms * 1000);

    LOG(TRACE, "exit:: rc=%d", rc);
    return rc;
}

/**
* Function used to check if the client is connected
*
* @return int return code
*/
int isConnected(iotfclient  *client)
{
    LOG(TRACE, "entry::");

    int rc = MQTTClient_isConnected(client->c);

    LOG(TRACE, "exit:: rc=%d", rc);
    return rc;
}

/**
 * Function used to set the Command Callback function. This must be set if you to recieve commands.
 *
 * @param cb - A Function pointer to the commandCallback. Its signature - void (*commandCallback)(char* commandName, char* pay
 * @return int return code
 */
void setCommandHandler(iotfclient  *client, commandCallback handler)
{
    LOG(TRACE, "entry::");

    cb = handler;

    if (cb != NULL){
        LOG(INFO, "Client ID %s : Registered callabck to process the arrived message", client->cfg.id);
    } else {
        LOG(INFO, "Client ID %s : Callabck not registered to process the arrived message", client->cfg.id);
    }

    LOG(TRACE, "exit::");
}

/* Handle Message Delivery notices */
static void messageDelivered(void *context, MQTTClient_deliveryToken dt)
{
    LOG(TRACE, "entry::");
    LOG(DEBUG, "Message delivery confirmed. context=%x token=%d", context, dt);
    LOG(TRACE, "exit::");
}

/* Handler for all commands. Invoke the callback. */
static int messageArrived(void *context, char *topicName, int topicLen, MQTTClient_message * message)
{
    LOG(TRACE, "entry::");

    /* Check if the topic is device management topic */
    if ( topicName && strncmp(topicName, "iotdm-1/", 8) == 0 ) {
        void *payload = message->payload;
        size_t payloadlen = message->payloadlen;
        topicLen = strlen(topicName);
        int rc = messageArrived_dm(context, topicName, topicLen, payload, payloadlen);
        MQTTClient_freeMessage(&message);
        MQTTClient_free(topicName);
        return rc;
    }

    /* Process incoming message if callback is defined */
    if (cb != 0) {
        char topic[4096];
        void *payload = message->payload;
        size_t payloadlen = message->payloadlen;

        sprintf(topic,"%s",topicName);

        LOG(INFO, "Context:%x Topic:%s TopicLen=%d PayloadLen=%d Payload:%s", context, topic, topicLen, payloadlen, payload);

        char *type = NULL;
        char *id = NULL;
        char *commandName = NULL;
        char *format = NULL;

        if ( strncmp(topicName, "iot-2/cmd/", 10) == 0 ) {

            strtok(topic, "/");
            strtok(NULL, "/");
            commandName = strtok(NULL, "/");
            strtok(NULL, "/");
            format = strtok(NULL, "/");

        } else {

            strtok(topic, "/");
            strtok(NULL, "/");

            type = strtok(NULL, "/");
            strtok(NULL, "/");
            id = strtok(NULL, "/");
            strtok(NULL, "/");
            commandName = strtok(NULL, "/");
            strtok(NULL, "/");
            format = strtok(NULL, "/");

        }

        LOG(TRACE, "Calling registered callabck to process the arrived message");

        (*cb)(type,id,commandName, format, payload,payloadlen);

        MQTTClient_freeMessage(&message);
        MQTTClient_free(topicName);
    } else {
        LOG(TRACE, "No registered callback function to process the arrived message");
    }

    LOG(TRACE, "exit::");
    return 1;
}


/**
* Function used to disconnect from the IBM Watson IoT service
*
* @return int return code
*/
int disconnect(iotfclient  *client)
{
    LOG(TRACE, "entry::");

    int rc = 0;
    if (isConnected(client)) {
        rc = MQTTClient_disconnect((MQTTClient *)client->c, 10000);

        /* Free gateway subscription list if set */
        freeGatewaySubscriptionList(client);
    }

    // MQTTClient_destroy(client->c);
    freeConfig(&(client->cfg));

    LOG(TRACE, "exit:: %d", rc);
    return rc;
}

/**
* Function used to set the time to keep the connection alive with IBM Watson IoT service
* @param keepAlive - time in secs
*
*/
void setKeepAliveInterval(unsigned int keepAlive)
{
    LOG(TRACE, "entry::");
    keepAliveInterval = keepAlive;
    LOG(TRACE, "exit::");
}

/**
 * Retry connection
 */
int retry_connection(iotfclient  *client)
{
    LOG(TRACE, "entry::");

    int retry = 1;
    int rc = -1;

    while((rc = connectiotf(client)) != MQTTCLIENT_SUCCESS)
    {
        LOG(DEBUG, "Retry Attempt #%d ", retry);
        int delay = reconnect_delay(retry++);
        LOG(DEBUG, " next attempt in %d seconds\n", delay);
        sleep(delay);
    }

    LOG(TRACE, "exit:: %d", rc);
    return rc;
}


