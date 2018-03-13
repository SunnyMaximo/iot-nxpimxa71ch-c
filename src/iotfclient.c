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
 *
 *******************************************************************************/

#include "iotfclient.h"

unsigned short keepAliveInterval = 60;

//Character strings to hold log header and log message to be dumped.
char logHdr[LOG_BUF];
char logStr[LOG_BUF];

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
       enableLogging();
       sprintf(logHdr,"%s:%d:%s:",__FILE__,__LINE__,__func__);
       LOG(logHdr,"entry::");

       Config configstr = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 1883, 0, 0};

       int rc = 0;

       rc = get_config(configFilePath, &configstr);

       sprintf(logHdr,"%s:%d:%s:",__FILE__,__LINE__,__func__);
       sprintf(logStr,"org:%s , domain:%s , type: %s , id:%s , token: %s , useCerts: %d , serverCertPath: %s",
		configstr.org,configstr.domain,configstr.type,configstr.id,configstr.authtoken,configstr.useClientCertificates,
		configstr.serverCertPath);
       LOG(logHdr,logStr);

       if(rc != SUCCESS) {
	       goto exit;
       }

       /* check for quickstart domain */
       if ((strcmp(configstr.org,"quickstart") == 0))
	       client->isQuickstart = 1;
       else
	       client->isQuickstart = 0;

       sprintf(logHdr,"%s:%d:%s:",__FILE__,__LINE__,__func__);
       sprintf(logStr,"isQuickStart Mode: %d",client->isQuickstart);
       LOG(logHdr,logStr);


       if ( client->isQuickstart == 0 ) {
           if (configstr.org == NULL || configstr.type == NULL || configstr.id == NULL ||
	       configstr.authmethod == NULL || configstr.authtoken == NULL) {
                   rc = MISSING_INPUT_PARAM;
           }
       } else {
           if (configstr.org == NULL || configstr.type == NULL || configstr.id == NULL ) {
                   rc = MISSING_INPUT_PARAM;
           }
       }

       if ( rc != 0 ) {
	   freeConfig(&configstr);
	   goto exit;
       }

       sprintf(logHdr,"%s:%d:%s:",__FILE__,__LINE__,__func__);
       sprintf(logStr,"useCertificates: %d",configstr.useClientCertificates);
       LOG(logHdr,logStr);

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

       if(configstr.useClientCertificates){
	       if(configstr.rootCACertPath == NULL || configstr.clientCertPath == NULL ||
		  configstr.clientKeyPath == NULL){
		       freeConfig(&configstr);
		       rc = MISSING_INPUT_PARAM;
		       goto exit;
	       }
	       sprintf(logHdr,"%s:%d:%s:",__FILE__,__LINE__,__func__);
	       sprintf(logStr,"CACertPath:%s , clientCertPath:%s , clientKeyPath: %s",
			configstr.rootCACertPath,configstr.clientCertPath,configstr.clientKeyPath);
	       LOG(logHdr,logStr);
       }

       if(isGatewayClient){
	       if(client->isQuickstart) {
		       printf("Quickstart mode is not supported in Gateway Client\n");
		       freeConfig(&configstr);
		       return QUICKSTART_NOT_SUPPORTED;
	       }
	       client->isGateway = 1;
       }
       else
	       client->isGateway = 0;

        sprintf(logHdr,"%s:%d:%s:",__FILE__,__LINE__,__func__);
	sprintf(logStr,"isGateway Client: %d",client->isGateway);
	LOG(logHdr,logStr);

       client->cfg = configstr;

 exit:
        sprintf(logHdr,"%s:%d:%s:",__FILE__,__LINE__,__func__);
	sprintf(logStr,"rc = %d",rc);
	LOG(logHdr,logStr);
	LOG(logHdr,"exit::");

       return rc;

}

/**
* Function used to initialize the Watson IoT client
* @param client - Reference to the Iotfclient
* @param org - Your organization ID
* @param domain - Your domain Name
* @param type - The type of your device
* @param id - The ID of your device
* @param auth-method - Method of authentication (the only value currently supported is â€œtokenâ€�)
* @param auth-token - API key token (required if auth-method is â€œtokenâ€�)
* @Param serverCertPath - Custom Server Certificate Path
* @Param useCerts - Flag to indicate whether to use client side certificates for authentication
* @Param rootCAPath - if useCerts is 1, Root CA certificate Path
* @Param clientCertPath - if useCerts is 1, Client Certificate Path
* @Param clientKeyPath - if useCerts is 1, Client Key Path
* @Param useNXPEngine - if useCerts is 1 and domain is not quickstart
*
* @return int return code
*/
int initialize(iotfclient  *client, char *orgId, char* domainName, char *deviceType,
	      char *deviceId, char *authmethod, char *authToken, char *serverCertPath, int useCerts,
	       char *rootCACertPath, char *clientCertPath,char *clientKeyPath, int isGatewayClient, int useNXPEngine)
{
       enableLogging();
       sprintf(logHdr,"%s:%d:%s:",__FILE__,__LINE__,__func__);
       LOG(logHdr,"entry::");

       Config configstr = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 1883, 0, 0};
       int rc = 0;

       	sprintf(logHdr,"%s:%d:%s:",__FILE__,__LINE__,__func__);
	sprintf(logStr,"org:%s , domain:%s , type: %s , id:%s , token: %s , useCerts: %d , serverCertPath: %s",
		       orgId,domainName,deviceType,deviceId,authToken,useCerts,serverCertPath);
	LOG(logHdr,logStr);

       if(orgId == NULL || deviceType == NULL || deviceId == NULL) {
	       rc = MISSING_INPUT_PARAM;
	       goto exit;
       }

       if(useCerts){
	       if(rootCACertPath == NULL || clientCertPath == NULL || clientKeyPath == NULL){
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

        sprintf(logHdr,"%s:%d:%s:",__FILE__,__LINE__,__func__);
	sprintf(logStr,"cfgstr.domain:%s , cfgstr.type:%s , cfgstr.id: %s",configstr.domain,configstr.type,configstr.id);
	LOG(logHdr,logStr);

       if((strcmp(orgId,"quickstart") != 0)) {
	       if(authmethod == NULL || authToken == NULL) {
		       freeConfig(&configstr);
		       rc = MISSING_INPUT_PARAM;
		       goto exit;
	       }
	       strCopy(&configstr.authmethod, authmethod);
	       strCopy(&configstr.authtoken, authToken);

	       sprintf(logHdr,"%s:%d:%s:",__FILE__,__LINE__,__func__);
	       sprintf(logStr,"cfgstr.authmethod:%s , cfgstr.token:%s ",configstr.authmethod,configstr.authtoken);
	       LOG(logHdr,logStr);

	       if(serverCertPath == NULL)
		       strCopy(&configstr.serverCertPath,"./IoTFoundation.pem");
	       else
		       strCopy(&configstr.serverCertPath,serverCertPath);

	       sprintf(logHdr,"%s:%d:%s:",__FILE__,__LINE__,__func__);
	       sprintf(logStr,"cfgstr.serverCertPath:%s",configstr.serverCertPath);
	       LOG(logHdr,logStr);

	       if(useCerts){
		       strCopy(&configstr.rootCACertPath,rootCACertPath);
		       strCopy(&configstr.clientCertPath,clientCertPath);
		       strCopy(&configstr.clientKeyPath,clientKeyPath);
		       configstr.useClientCertificates = 1;

		       sprintf(logHdr,"%s:%d:%s:",__FILE__,__LINE__,__func__);
		       sprintf(logStr,"cfgstr.CACertPath:%s , cfgstr.clientCertPath:%s , cfgstr.clientKeyPath: %s",
				configstr.rootCACertPath,configstr.clientCertPath,configstr.clientKeyPath);
		       LOG(logHdr,logStr);
		       sprintf(logStr,"cfgstr.useCertificates:%d",configstr.useClientCertificates);
		       LOG(logHdr,logStr);
	       }
	       client->isQuickstart = 0;
               configstr.port = 8883;
       }
       else
	       client->isQuickstart = 1;

        sprintf(logHdr,"%s:%d:%s:",__FILE__,__LINE__,__func__);
	sprintf(logStr,"isQuickStart Mode: %d Port: %d",client->isQuickstart,configstr.port);
	LOG(logHdr,logStr);

       if(isGatewayClient){
	       if(client->isQuickstart) {
		       printf("Quickstart mode is not supported in Gateway Client\n");
		       freeConfig(&configstr);
		       return QUICKSTART_NOT_SUPPORTED;
	       }
	       client->isGateway = 1;
       }
       else
	       client->isGateway = 0;

        sprintf(logHdr,"%s:%d:%s:",__FILE__,__LINE__,__func__);
	sprintf(logStr,"isGateway Client: %d",client->isGateway);
	LOG(logHdr,logStr);

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
        sprintf(logHdr,"%s:%d:%s:",__FILE__,__LINE__,__func__);
	sprintf(logStr,"rc = %d",rc);
	LOG(logHdr,logStr);
	LOG(logHdr,"exit::");

       return rc;
}

// This is the function to read the config from the device.cfg file
int get_config(char * filename, Config * configstr) {
       sprintf(logHdr,"%s:%d:%s:",__FILE__,__LINE__,__func__);
       LOG(logHdr,"entry::");

       int rc = 0;
       FILE* prop;
       prop = fopen(filename, "r");
       if (prop == NULL) {
	      rc = CONFIG_FILE_ERROR;
	      goto exit;
       }
       char line[256];
       int linenum = 0;
       strCopy(&configstr->domain,"internetofthings.ibmcloud.com");

       sprintf(logHdr,"%s:%d:%s:",__FILE__,__LINE__,__func__);
       sprintf(logStr,"Default domainName: %s",configstr->domain);
       LOG(logHdr,logStr);

       while (fgets(line, 256, prop) != NULL) {
	      char* prop;
	      char* value;
	      linenum++;
	      if (line[0] == '#')
		      continue;

	      prop = strtok(line, "=");
	      prop = trim(prop);
	      value = strtok(NULL, "=");
	      value = trim(value);

	      sprintf(logHdr,"%s:%d:%s:",__FILE__,__LINE__,__func__);
	      sprintf(logStr,"Property: %s , Value: %s",prop,value);
	      LOG(logHdr,logStr);

	      if (strcmp(prop, "org") == 0){
		  if(strlen(value) > 1)
		    strCopy(&(configstr->org), value);

		  if(strcmp(configstr->org,"quickstart") !=0)
		     configstr->port = 8883;

		  sprintf(logHdr,"%s:%d:%s:",__FILE__,__LINE__,__func__);
		  sprintf(logStr,"cfgstr.org: %s , cfgstr.port: %d",configstr->org,configstr->port);
		  LOG(logHdr,logStr);
	       }
	      else if (strcmp(prop, "domain") == 0){
		  if(strlen(value) <= 1)
		     strCopy(&configstr->domain,"internetofthings.ibmcloud.com");
		  else
		     strCopy(&configstr->domain, value);

		  sprintf(logHdr,"%s:%d:%s:",__FILE__,__LINE__,__func__);
		  sprintf(logStr,"cfgstr.domain: %s ",configstr->domain);
		  LOG(logHdr,logStr);
	       }
	      else if (strcmp(prop, "type") == 0){
		  if(strlen(value) > 1)
		    strCopy(&configstr->type, value);
	       }
	      else if (strcmp(prop, "id") == 0){
		  if(strlen(value) > 1)
		    strCopy(&configstr->id, value);
	       }
	      else if (strcmp(prop, "auth-token") == 0){
		  if(strlen(value) > 1)
		    strCopy(&configstr->authtoken, value);
	       }
	      else if (strcmp(prop, "auth-method") == 0){
		  if(strlen(value) > 1)
		    strCopy(&configstr->authmethod, value);
	       }
	      else if (strcmp(prop, "serverCertPath") == 0){
		  if(strlen(value) <= 1)
		     getServerCertPath(&configstr->serverCertPath);
		  else
		     strCopy(&configstr->serverCertPath, value);

		  sprintf(logHdr,"%s:%d:%s:",__FILE__,__LINE__,__func__);
		  sprintf(logStr,"cfgstr.serverCertPath: %s ",configstr->serverCertPath);
		  LOG(logHdr,logStr);
	       }
	      else if (strcmp(prop, "rootCACertPath") == 0){
		  if(strlen(value) > 1)
		    strCopy(&configstr->rootCACertPath, value);
	       }
	      else if (strcmp(prop, "clientCertPath") == 0){
		  if(strlen(value) > 1)
		    strCopy(&configstr->clientCertPath, value);
	       }
	      else if (strcmp(prop, "clientKeyPath") == 0){
		  if(strlen(value) > 1)
		    strCopy(&configstr->clientKeyPath, value);
	       }
	      else if (strcmp(prop,"useClientCertificates") == 0){
		  configstr->useClientCertificates = value[0] - '0';
	       }
	      else if (strcmp(prop,"useNXPEngine") == 0){
		  configstr->useNXPEngine = value[0] - '0';
	       }
       }
 exit:
        sprintf(logHdr,"%s:%d:%s:",__FILE__,__LINE__,__func__);
	sprintf(logStr,"rc = %d",rc);
	LOG(logHdr,logStr);
	LOG(logHdr,"exit::");

       return rc;
}

/**
* Function used to connect to the IBM Watson IoT client
* @param client - Reference to the Iotfclient
*
* @return int return code
*/
int connectiotf(iotfclient  *client)
{
       sprintf(logHdr,"%s:%d:%s:",__FILE__,__LINE__,__func__);
       LOG(logHdr,"entry::");

       int rc = 0;

        MQTTClient mqttClient;
        MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
        MQTTClient_SSLOptions ssl_opts = MQTTClient_SSLOptions_initializer;

       int useCerts = client->cfg.useClientCertificates;
       int isGateway = client->isGateway;
       int qsMode = client->isQuickstart;
       int port = client->cfg.port;

       sprintf(logHdr,"%s:%d:%s:",__FILE__,__LINE__,__func__);
       sprintf(logStr,"useCerts:%d , isGateway:%d , qsMode:%d",useCerts,isGateway,qsMode);
       LOG(logHdr,logStr);

       char messagingUrl[120];
       sprintf(messagingUrl, ".messaging.%s",client->cfg.domain);
       char hostname[strlen(client->cfg.org) + strlen(messagingUrl) + 1];
       sprintf(hostname, "%s%s", client->cfg.org, messagingUrl);
       int clientIdLen =  strlen(client->cfg.org) + strlen(client->cfg.type) + strlen(client->cfg.id) + 5;
       char *clientId = malloc(clientIdLen);

       if(isGateway)
	  sprintf(clientId, "g:%s:%s:%s", client->cfg.org, client->cfg.type, client->cfg.id);
       else
	  sprintf(clientId, "d:%s:%s:%s", client->cfg.org, client->cfg.type, client->cfg.id);

       	sprintf(logHdr,"%s:%d:%s:",__FILE__,__LINE__,__func__);
	sprintf(logStr,"messagingUrl:%s , hostname:%s , port:%d , clientId:%s",messagingUrl,hostname,port,clientId);
	LOG(logHdr,logStr);

       // create MQTT Client
       char connectionUrl[strlen(hostname) + 14];
       if ( port == 1883 )
           sprintf(connectionUrl, "tcp://%s:%d", hostname, port);
       else
           sprintf(connectionUrl, "ssl://%s:%d", hostname, port);
       sprintf(logHdr,"%s:%d:%s:",__FILE__,__LINE__,__func__);
       sprintf(logStr,"connectionUrl:%s", connectionUrl);
       LOG(logHdr,logStr);

       /* If useNXPEngine is enabled set environment variable to load NXP engine */
       if ( client->cfg.useNXPEngine ) {
           char * envval;
           envval = getenv ("OPENSSL_CONF");

           if ( envval && *envval != '\0' && strstr(envval, "A71CH")) {
          	sprintf(logHdr,"%s:%d:%s:",__FILE__,__LINE__,__func__);
	        sprintf(logStr,"OPENSSL_CONF set to %s", envval);
	        LOG(logHdr,logStr);
           } else {
               setenv ("OPENSSL_CONF", "/etc/ssl/opensslA71CH_i2c.cnf", 1);
               envval = getenv ("OPENSSL_CONF");
       	       sprintf(logHdr,"%s:%d:%s:",__FILE__,__LINE__,__func__);
	       sprintf(logStr,"OPENSSL_CONF set to %s", envval);
	       LOG(logHdr,logStr);
           }
       }

       rc = MQTTClient_create(&mqttClient, connectionUrl, clientId, MQTTCLIENT_PERSISTENCE_NONE, NULL);
       if ( rc != 0 ) {
	   sprintf(logHdr,"%s:%d:%s:",__FILE__,__LINE__,__func__);
	   sprintf(logStr,"RC from MQTTClient_create:%d",rc);
           LOG(logHdr,logStr);
           goto exit;
       }
       client->c = mqttClient;

       // set callbacks and connection options
       conn_opts.keepAliveInterval = keepAliveInterval;
       conn_opts.reliable = 0;
       conn_opts.cleansession = 1;
       ssl_opts.enableServerCertAuth = 0;

       if (!qsMode) {
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

           
       if ((rc = MQTTClient_connect(client->c, &conn_opts))==0){
	   if(qsMode)
	      printf("Device Client Connected to %s Platform in QuickStart Mode\n",hostname);
	   else
	   {
	       char *clientType = (isGateway)?"Gateway Client":"Device Client";
	       char *connType = (useCerts)?"Client Side Certificates":"Secure Connection";
	       printf("%s Connected to %s in registered mode using %s\n",clientType,hostname,connType);
	   }

	   sprintf(logHdr,"%s:%d:%s:",__FILE__,__LINE__,__func__);
	   sprintf(logStr,"RC from MQTTConnect: %d",rc);
	   LOG(logHdr,logStr);
       }

exit:
        sprintf(logHdr,"%s:%d:%s:",__FILE__,__LINE__,__func__);
	sprintf(logStr,"rc = %d",rc);
	LOG(logHdr,logStr);

        if(rc != 0){
                freeConfig(&client->cfg);
        }

        sprintf(logHdr,"%s:%d:%s:",__FILE__,__LINE__,__func__);
	LOG(logHdr,"exit::");

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
int publishData(iotfclient *client, char *topic, char *payload, int qos){
       sprintf(logHdr,"%s:%d:%s:",__FILE__,__LINE__,__func__);
       LOG(logHdr,"entry::");

       int rc = -1;
       int payloadlen = strlen(payload);
       int retained = '0';

       sprintf(logHdr,"%s:%d:%s:",__FILE__,__LINE__,__func__);
       sprintf(logStr,"MQTTMessage = { qos: %d  retained: %d  payload: %s  payloadLen: %d}",
                        qos,retained,payload,payloadlen);
       LOG(logHdr,logStr);

       rc = MQTTClient_publish(client->c, topic, payloadlen, payload, qos, retained, NULL);

       sprintf(logHdr,"%s:%d:%s:",__FILE__,__LINE__,__func__);
       sprintf(logStr,"rc = %d",rc);
       LOG(logHdr,logStr);
       LOG(logHdr,"exit::");

       return(rc);
}

/**
* Function used to Yield for commands.
* @param time_ms - Time in milliseconds
* @return int return code
*/
int yield(iotfclient  *client, int time_ms)
{
       sprintf(logHdr,"%s:%d:%s:",__FILE__,__LINE__,__func__);
       LOG(logHdr,"entry::");

       int rc = 0;
       // rc = MQTTYield(&client->c, time_ms);
       rc = usleep(time_ms * 1000);

       sprintf(logHdr,"%s:%d:%s:",__FILE__,__LINE__,__func__);
       sprintf(logStr,"rc = %d",rc);
       LOG(logHdr,logStr);
       LOG(logHdr,"exit::");

       return rc;
}

/**
* Function used to check if the client is connected
*
* @return int return code
*/
int isConnected(iotfclient  *client)
{
        sprintf(logHdr,"%s:%d:%s:",__FILE__,__LINE__,__func__);
        LOG(logHdr,"entry::");

       int result = MQTTClient_isConnected(client->c);

       sprintf(logHdr,"%s:%d:%s:",__FILE__,__LINE__,__func__);
       sprintf(logStr,"isConnected = %d",result);
       LOG(logHdr,logStr);
       LOG(logHdr,"exit::");

       return result;
}

/**
* Function used to disconnect from the IBM Watson IoT service
*
* @return int return code
*/

int disconnect(iotfclient  *client)
{
        sprintf(logHdr,"%s:%d:%s:",__FILE__,__LINE__,__func__);
        LOG(logHdr,"entry::");

       int rc = 0;
       if (isConnected(client)) {
	  rc = MQTTClient_disconnect(client->c, 0);
       }
       // MQTTClient_destroy(client->c);
       freeConfig(&(client->cfg));

       sprintf(logHdr,"%s:%d:%s:",__FILE__,__LINE__,__func__);
       sprintf(logStr,"rc = %d",rc);
       LOG(logHdr,logStr);
       LOG(logHdr,"exit::");

       return rc;

}

/**
* Function used to set the time to keep the connection alive with IBM Watson IoT service
* @param keepAlive - time in secs
*
*/
void setKeepAliveInterval(unsigned int keepAlive)
{
       keepAliveInterval = keepAlive;

}

//Staggered retry
int retry_connection(iotfclient  *client)
{
       int retry = 1;
       int rc = -1;

       while((rc = connectiotf(client)) != SUCCESS)
       {
	       printf("Retry Attempt #%d ", retry);
	       int delay = reconnect_delay(retry++);
	       printf(" next attempt in %d seconds\n", delay);
	       sleep(delay);
       }
       return rc;
}

void freeConfig(Config *cfg){
       sprintf(logHdr,"%s:%d:%s:",__FILE__,__LINE__,__func__);
       LOG(logHdr,"entry::");

       freePtr(cfg->org);
       freePtr(cfg->domain);
       freePtr(cfg->type);
       freePtr(cfg->id);
       freePtr(cfg->authmethod);
       freePtr(cfg->authtoken);
       freePtr(cfg->rootCACertPath);
       freePtr(cfg->clientCertPath);
       freePtr(cfg->clientKeyPath);

       sprintf(logHdr,"%s:%d:%s:",__FILE__,__LINE__,__func__);
       LOG(logHdr,"exit::");

       disableLogging();
}
