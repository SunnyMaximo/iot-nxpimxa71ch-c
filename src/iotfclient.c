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
 *    Jeffrey Dare            - initial implementation and API implementation
 *    Sathiskumar Palaniappan - Added support to create multiple Iotfclient
 *                              instances within a single process
 *    Lokesh Haralakatta      - Added SSL/TLS support
 *    Lokesh Haralakatta      - Added Client Side Certificates support
 *    Lokesh Haralakatta      - Separated out device client and gateway client specific code.
 *                            - Retained back common code here.
 *    Lokesh Haralakatta      - Added Logging Feature
 *
 * ----------------------------------------------------------------------------
 * Contrinutors for NXP Engine changes:
 *    Ranjan Dasgupta         - Initial changes to support NXP Engine
 *                            - Code cleanup/refactor and logging support
 *
 *******************************************************************************/

#include "iotfclient.h"
#include "iotf_utils.h"

static int get_config(char * filename, Config * configstr);
static void freeConfig(Config *cfg);
static int messageArrived(void *context, char *topicName, int topicLen, MQTTClient_message * message);
static void messageDelivered(void *context, MQTTClient_deliveryToken dt);
extern void freeGatewaySubscriptionList(iotfclient *client);

/* Command Callback */
commandCallback cb;
 
unsigned short keepAliveInterval = 60;

/**
 * Function used to initialize the IBM Watson IoT client using the config file which is
 * generated when you register your device.
 * @param configFilePath - File path to the configuration file
 * @Param isGatewayClient - 0 for device client or 1 for gateway client
 *
 * @return int return code
 * error codes
 * CONFIG_FILE_ERROR -3 - Config file not present or not in right format
 */
int initialize_configfile(iotfclient  *client, char *configFilePath, int isGatewayClient)
{
    LOG(TRACE, "entry::");

    Config configstr = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 1883, 0, 0};

    int rc = get_config(configFilePath, &configstr);
    if (rc != MQTTCLIENT_SUCCESS) {
        goto exit;
    }

    LOG(DEBUG, "org:%s , domain:%s , type: %s , id:%s , token: %s , useCerts: %d , serverCertPath: %s",
        configstr.org,configstr.domain,configstr.type,configstr.id,configstr.authtoken,configstr.useClientCertificates,
        configstr.serverCertPath);

    /* check for quickstart domain */
    if ((strcmp(configstr.org,"quickstart") == 0)) {
        client->isQuickstart = 1;
        LOG(INFO, "Connection to WIoTP Quickstart org");
    } else {
        client->isQuickstart = 0;
    }

    /* Validate input - for missing configuration data */
    if ((configstr.org == NULL || configstr.type == NULL || configstr.id == NULL) ||
        (client->isQuickstart == 0 && configstr.useClientCertificates && 
         (configstr.rootCACertPath == NULL || configstr.clientCertPath == NULL || configstr.clientKeyPath == NULL))) 
    {
        rc = MISSING_INPUT_PARAM;
        freeConfig(&configstr);
        goto exit;
    }

    LOG(INFO, "useCertificates=%d", configstr.useClientCertificates);

    /* Check if NXP Engine is enabled - NXP Engine is not allowed in quickstart domain or useClientCertificate
     * is not enabled
     */
    if ( configstr.useNXPEngine ) {
        if ( ! configstr.useClientCertificates ) {
            freeConfig(&configstr);
            rc = CONFIG_FILE_ERROR;
            goto exit;
        }

        if ( client->isQuickstart == 1 ) {
           freeConfig(&configstr);
           rc = QUICKSTART_NOT_SUPPORTED;
           goto exit;
        }
    }

    if (configstr.useClientCertificates){
       LOG(INFO, "CACertPath=%s", configstr.rootCACertPath);
       LOG(INFO, "clientCertPath=%s", configstr.clientCertPath);
       LOG(INFO, "clientKeyPath=%s", configstr.clientKeyPath);
    }

    if (isGatewayClient){
        if (client->isQuickstart) {
            LOG(ERROR, "Quickstart mode is not supported in Gateway Client\n");
            freeConfig(&configstr);
            return QUICKSTART_NOT_SUPPORTED;
        }
        client->isGateway = 1;
        LOG(TRACE, "Connecting as a gateway client");
    } else {
        client->isGateway = 0;
        LOG(TRACE, "Connecting as a device client");
    }

    client->cfg = configstr;

exit:
    LOG(TRACE,"exit:: rc=%d", rc);

   return rc;
}

/**
 * Function used to initialize the Watson IoT client
 *
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
 * @Param useNXPEngine - if useCerts is 1 and domain is not quickstart, Use NXP SSL Engine
 *
 * @return int return code
 */
int initialize(iotfclient  *client, char *orgId, char* domainName, char *deviceType,
    char *deviceId, char *authmethod, char *authToken, char *serverCertPath, int useCerts,
    char *rootCACertPath, char *clientCertPath,char *clientKeyPath, int isGatewayClient, int useNXPEngine)
{
    LOG(TRACE, "entry::");

    Config configstr = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 1883, 0, 0};
    int rc = 0;

    LOG(DEBUG, "org=%s, domain=%s, type=%s, id=%s, token= s, useCerts=%d, serverCertPath=%s",
               orgId,domainName,deviceType,deviceId,authToken,useCerts,serverCertPath);

    if (orgId == NULL || deviceType == NULL || deviceId == NULL) {
        rc = MISSING_INPUT_PARAM;
        goto exit;
    }

    if (useCerts) {
        if (rootCACertPath == NULL || clientCertPath == NULL || clientKeyPath == NULL){
            rc = MISSING_INPUT_PARAM;
            goto exit;
        }
    }

    strCopy(&configstr.org, orgId);
    strCopy(&configstr.domain,"internetofthings.ibmcloud.com");
    if(domainName != NULL)
        strCopy(&configstr.domain, domainName);
    strCopy(&configstr.type, deviceType);
    strCopy(&configstr.id, deviceId);

    if ((strcmp(orgId,"quickstart") != 0)) {
        if (authmethod == NULL || authToken == NULL) {
            freeConfig(&configstr);
            rc = MISSING_INPUT_PARAM;
            goto exit;
        }
        strCopy(&configstr.authmethod, authmethod);
        strCopy(&configstr.authtoken, authToken);

        LOG(DEBUG, "auth-method=%s, token=%s", configstr.authmethod, configstr.authtoken);

        if (serverCertPath == NULL)
            strCopy(&configstr.serverCertPath,"./IoTFoundation.pem");
        else
            strCopy(&configstr.serverCertPath,serverCertPath);

        LOG(DEBUG, "serverCertPath=%s", configstr.serverCertPath);

        if (useCerts) {
            strCopy(&configstr.rootCACertPath,rootCACertPath);
            strCopy(&configstr.clientCertPath,clientCertPath);
            strCopy(&configstr.clientKeyPath,clientKeyPath);
            configstr.useClientCertificates = 1;
            LOG(DEBUG, "Use client certificate");
            LOG(DEBUG, "CACertPath=%s", configstr.rootCACertPath);
            LOG(DEBUG, "clientCertPath=%s", configstr.clientCertPath);
            LOG(DEBUG, "clientKeyPath=%s", configstr.clientKeyPath);
        }

        client->isQuickstart = 0;
        configstr.port = 8883;
    } else {
        client->isQuickstart = 1;
        LOG(DEBUG, "Connecting to WIoTP quickstart org: port=%d", configstr.port);
    }

    if (isGatewayClient){
        if (client->isQuickstart) {
            LOG(ERROR, "Quickstart mode is not supported in Gateway Client\n");
            freeConfig(&configstr);
            return QUICKSTART_NOT_SUPPORTED;
        }
        client->isGateway = 1;
        LOG(DEBUG, "Connecting as a gateway client");
    } else {
        client->isGateway = 0;
        LOG(DEBUG, "Connecting as a device client");
    }


    /* Check if NXP Engine is enabled - NXP Engine is not allowed in quickstart domain or useClientCertificate
     * is not enabled
     */
    if ( configstr.useNXPEngine ) {
        if ( ! configstr.useClientCertificates ) {
           freeConfig(&configstr);
           rc = CONFIG_FILE_ERROR;
           goto exit;
        }

        if ( client->isQuickstart == 1 ) {
           freeConfig(&configstr);
           rc = QUICKSTART_NOT_SUPPORTED;
           goto exit;
        }
    }

    client->cfg = configstr;

exit:
    LOG(TRACE,"exit:: rc=%d", rc);

    return rc;
}

/**
 * This is the function to read the config from the device.cfg file
 */
static int get_config(char * filename, Config * configstr) 
{
    LOG(TRACE, "entry::");

    int rc = 0;
    FILE* prop;

    /* Sanity check */
    if ( !filename || *filename == '\0') {
        LOG(ERROR, "Invalid Configuration file path is specified: NULL or empty file path");
        rc = CONFIG_FILE_ERROR;
        goto exit;
    }

    /* Open property file */
    prop = fopen(filename, "r");
    if (prop == NULL) {
        LOG(ERROR, "Failed to open property file. error:%d", errno);
        rc = CONFIG_FILE_ERROR;
        goto exit;
    }
    char line[256];
    int linenum = 0;
    strCopy(&configstr->domain,"internetofthings.ibmcloud.com");

    while (fgets(line, 256, prop) != NULL) {
        char *prop;
        char *value;
        linenum++;
        if (line[0] == '#')
            continue;

        prop = strtok(line, "=");
        prop = trim(prop);
        value = strtok(NULL, "=");
        value = trim(value);

        LOG(TRACE, "Config: Property=%s Value=%s",prop,value);

        if (strcmp(prop, "org") == 0) {
            if (strlen(value) > 1)
                strCopy(&(configstr->org), value);

            if (strcmp(configstr->org,"quickstart") !=0)
                configstr->port = 8883;

            LOG(INFO, "org=%s", configstr->org);
            LOG(INFO, "port=%d", configstr->port);

        } else if (strcmp(prop, "domain") == 0) {
            if (strlen(value) <= 1)
                strCopy(&configstr->domain,"internetofthings.ibmcloud.com");
            else
                strCopy(&configstr->domain, value);

           LOG(INFO, "domain=%s", configstr->domain);

        } else if (strcmp(prop, "type") == 0) {
            if (strlen(value) > 1) {
                strCopy(&configstr->type, value);
                LOG(INFO, "type=%s", configstr->domain);
            }

        } else if (strcmp(prop, "id") == 0) {
            if (strlen(value) > 1) {
                strCopy(&configstr->id, value);
                LOG(INFO, "id=%s", configstr->id);
            }

        } else if (strcmp(prop, "auth-token") == 0) {
            if (strlen(value) > 1) {
                strCopy(&configstr->authtoken, value);
                LOG(INFO, "auth-token=XXXXXX");
            }

        } else if (strcmp(prop, "auth-method") == 0) {
            if(strlen(value) > 1) {
                strCopy(&configstr->authmethod, value);
                LOG(INFO, "auth_token=%s", configstr->authmethod);
            }

        } else if (strcmp(prop, "serverCertPath") == 0){
            if(strlen(value) <= 1)
                getServerCertPath(&configstr->serverCertPath);
           else
              strCopy(&configstr->serverCertPath, value);

           LOG(TRACE, "serverCertPath=%s", configstr->serverCertPath);

        } else if (strcmp(prop, "rootCACertPath") == 0){
            if(strlen(value) > 1) {
                strCopy(&configstr->rootCACertPath, value);
                LOG(INFO, "rootCACertPath=%s", configstr->rootCACertPath);
            }

        } else if (strcmp(prop, "clientCertPath") == 0){
            if(strlen(value) > 1) {
                strCopy(&configstr->clientCertPath, value);
                LOG(INFO, "clientCertPath=%s", configstr->clientCertPath);
            }

        } else if (strcmp(prop, "clientKeyPath") == 0){
            if(strlen(value) > 1) {
                strCopy(&configstr->clientKeyPath, value);
                LOG(INFO, "clientKeyPath=%s", configstr->clientKeyPath);
            }

        } else if (strcmp(prop,"useClientCertificates") == 0){
            configstr->useClientCertificates = value[0] - '0';
            LOG(INFO, "useClientCertificates=%d", configstr->useClientCertificates);

        } else if (strcmp(prop,"useNXPEngine") == 0){
            configstr->useNXPEngine = value[0] - '0';
            LOG(INFO, "Config: useNXPEngine=%d ",configstr->useNXPEngine);
        }
    }

exit:
    LOG(TRACE,"exit:: rc=%d", rc);
    return rc;
}

/**
 * Callback function to handle connection lose cases
 */
void connlost(void *context, char *cause)
{
    LOG(TRACE, "entry::");
    LOG(WARN, "IoTF client connection is lost. Cause=%s", cause);
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
    }

    rc = MQTTClient_create(&mqttClient, connectionUrl, clientId, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    if ( rc != 0 ) {
        LOG(WARN, "RC from MQTTClient_create:%d",rc);
        goto exit;
    }

    client->c = mqttClient;

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
    MQTTClient_setCallbacks(client->c, NULL, connlost, messageArrived, messageDelivered);
           
    if ((rc = MQTTClient_connect(client->c, &conn_opts)) == MQTTCLIENT_SUCCESS) {
        if (qsMode) {
            LOG(INFO, "Device Client Connected to %s Platform in QuickStart Mode\n",hostname);
        } else {
            char *clientType = (isGateway)?"Gateway Client":"Device Client";
            char *connType = (useCerts)?"Client Side Certificates":"Secure Connection";
            LOG(INFO, "%s Connected to %s using %s\n", clientType, hostname, connType);
        }
    }

exit:
    if (rc != 0) {
        freeConfig(&client->cfg);
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

    rc = MQTTClient_publishMessage(client->c, topic, &pubmsg, &token);
    LOG(DEBUG, "Message with delivery token %d delivered\n", token);

    LOG(TRACE, "exit:: rc=%d", rc);
    return rc;
}

/**
* Function used to Yield for commands.
* @param time_ms - Time in milliseconds
* @return int return code
*/
int yield(iotfclient  *client, int time_ms)
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
        LOG(DEBUG, "Registered callabck to process the arrived message");
    } else {
        LOG(DEBUG, "Callabck not registered to process the arrived message");
    }

    LOG(TRACE, "exit::");
}

/* Handle Message Delivery notices */
static void messageDelivered(void *context, MQTTClient_deliveryToken dt)
{
    LOG(TRACE, "entry::");
    LOG(DEBUG, "Message delivery confirmed. token=%d", dt);
    LOG(TRACE, "exit::");
}

/* Handler for all commands. Invoke the callback. */
static int messageArrived(void *context, char *topicName, int topicLen, MQTTClient_message * message)
{
    LOG(TRACE, "entry::");

    if (cb != 0) {
        char topic[4096];
        sprintf(topic,"%s",topicName);
        void *payload = message->payload;

        LOG(TRACE, "Topic:%s  Payload:%s", topic, (char *)payload);

        size_t payloadlen = message->payloadlen;

        strtok(topic, "/");
        strtok(NULL, "/");

        char *type = strtok(NULL, "/");
        strtok(NULL, "/");
        char *id = strtok(NULL, "/");
        strtok(NULL, "/");
        char *commandName = strtok(NULL, "/");
        strtok(NULL, "/");
        char *format = strtok(NULL, "/");

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
        rc = MQTTClient_disconnect(client->c, 10000);

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

/*
 * Free configuration structure
 */
static void freeConfig(Config *cfg)
{
    LOG(TRACE, "entry::");

    freePtr(cfg->org);
    freePtr(cfg->domain);
    freePtr(cfg->type);
    freePtr(cfg->id);
    freePtr(cfg->authmethod);
    freePtr(cfg->authtoken);
    freePtr(cfg->rootCACertPath);
    freePtr(cfg->clientCertPath);
    freePtr(cfg->clientKeyPath);

    LOG(TRACE, "exit::");
}

