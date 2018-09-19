/*******************************************************************************
 * Copyright (c) 2018 IBM Corp.
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
 * NOTE: Some of the code is taken from iot-embeddedc project and customized to
 *       support NXP SSL Engine.
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
void freeConfig(Config *cfg);

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

    Config configstr = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 1883, 0, 0, 0};

    int rc = get_config(configFilePath, &configstr);
    if (rc != 0) {
        goto exit;
    }

    LOG(INFO, "org:%s , type: %s , useCerts: %d", configstr.org, configstr.type, configstr.useClientCertificates);

    /* check for quickstart domain */
    if ((strcasecmp(configstr.org,"quickstart") == 0)) {
        client->isQuickstart = 1;
        LOG(INFO, "Connection to WIoTP Quickstart org");
    } else {
        client->isQuickstart = 0;
    }

    /* Validate input - for missing configuration data */
    if ((configstr.org == NULL || configstr.type == NULL ) ||
        (client->isQuickstart == 0 && configstr.useClientCertificates && configstr.useCertsFromSE == 0 &&
         (configstr.rootCACertPath == NULL || configstr.clientCertPath == NULL || configstr.clientKeyPath == NULL))) 
    {
        rc = MISSING_INPUT_PARAM;
        freeConfig(&configstr);
        goto exit;
    }

    LOG(INFO, "useCertificates=%d", configstr.useClientCertificates);
    LOG(INFO, "useCertsFromSE=%d", configstr.useCertsFromSE);

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
       LOG(INFO, "clientCertPath=%s", configstr.clientCertPath?configstr.clientCertPath:"");
       LOG(INFO, "clientKeyPath=%s", configstr.clientKeyPath?configstr.clientKeyPath:"");
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
 * @Param useCertsFromSE - If set, retrieve client certificate and key from Secure Element
 *
 * @return int return code
 */
int initialize(iotfclient  *client, char *orgId, char* domainName, char *deviceType,
    char *deviceId, char *authmethod, char *authToken, char *serverCertPath, int useCerts,
    char *rootCACertPath, char *clientCertPath,char *clientKeyPath, int isGatewayClient, 
    int useNXPEngine, int useCertsFromSE)
{
    LOG(TRACE, "entry::");

    Config configstr = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 1883, 0, 0, 0};
    int rc = 0;

    LOG(DEBUG, "org=%s, domain=%s, type=%s, id=%s, token= s, useCerts=%d, serverCertPath=%s useNXPEngine=%d useCertsFromSE=%d",
               orgId,domainName,deviceType,deviceId,authToken,useCerts,serverCertPath,useNXPEngine,useCertsFromSE);

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

    configstr.useNXPEngine = useNXPEngine;
    configstr.useCertsFromSE = useCertsFromSE;

    if ((strcasecmp(orgId,"quickstart") != 0)) {
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

        if (strcasecmp(prop, "org") == 0) {
            if (strlen(value) > 1)
                strCopy(&(configstr->org), value);

            if (strcasecmp(configstr->org,"quickstart") !=0)
                configstr->port = 8883;

            LOG(INFO, "org=%s", configstr->org);
            LOG(INFO, "port=%d", configstr->port);

        } else if (strcasecmp(prop, "domain") == 0) {
            if (strlen(value) <= 1)
                strCopy(&configstr->domain,"internetofthings.ibmcloud.com");
            else
                strCopy(&configstr->domain, value);

           LOG(INFO, "domain=%s", configstr->domain);

        } else if (strcasecmp(prop, "type") == 0) {
            if (strlen(value) > 1) {
                strCopy(&configstr->type, value);
                LOG(INFO, "type=%s", configstr->domain);
            }

        } else if (strcasecmp(prop, "id") == 0) {
            if (strlen(value) > 1) {
                strCopy(&configstr->id, value);
                LOG(INFO, "id=%s", configstr->id);
            }

        } else if (strcasecmp(prop, "auth-token") == 0) {
            if (strlen(value) > 1) {
                strCopy(&configstr->authtoken, value);
                LOG(INFO, "auth-token=XXXXXX");
            }

        } else if (strcasecmp(prop, "auth-method") == 0) {
            if(strlen(value) > 1) {
                strCopy(&configstr->authmethod, value);
                LOG(INFO, "auth_token=%s", configstr->authmethod);
            }

        } else if (strcasecmp(prop, "serverCertPath") == 0){
            if(strlen(value) <= 1)
                getServerCertPath(&configstr->serverCertPath);
           else
              strCopy(&configstr->serverCertPath, value);

           LOG(TRACE, "serverCertPath=%s", configstr->serverCertPath);

        } else if (strcasecmp(prop, "rootCACertPath") == 0){
            if(strlen(value) > 1) {
                strCopy(&configstr->rootCACertPath, value);
                LOG(INFO, "rootCACertPath=%s", configstr->rootCACertPath);
            }

        } else if (strcasecmp(prop, "clientCertPath") == 0){
            if(strlen(value) > 1) {
                strCopy(&configstr->clientCertPath, value);
                LOG(INFO, "clientCertPath=%s", configstr->clientCertPath);
            }

        } else if (strcasecmp(prop, "clientKeyPath") == 0){
            if(strlen(value) > 1) {
                strCopy(&configstr->clientKeyPath, value);
                LOG(INFO, "clientKeyPath=%s", configstr->clientKeyPath);
            }

        } else if (strcasecmp(prop,"useClientCertificates") == 0){
            configstr->useClientCertificates = value[0] - '0';
            LOG(INFO, "useClientCertificates=%d", configstr->useClientCertificates);

        } else if (strcasecmp(prop,"useNXPEngine") == 0){
            configstr->useNXPEngine = value[0] - '0';
            LOG(INFO, "Config: useNXPEngine=%d ",configstr->useNXPEngine);

        } else if (strcasecmp(prop,"useCertsFromSE") == 0){
            configstr->useCertsFromSE = value[0] - '0';
            LOG(INFO, "Config: useCertsFromSE=%d ",configstr->useCertsFromSE);

        }
    }

exit:
    LOG(TRACE,"exit:: rc=%d", rc);
    return rc;
}


/*
 * Free configuration structure
 */
void freeConfig(Config *cfg)
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


