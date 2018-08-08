#include "manageddevice.h"
#include "cJSON.h"

static char currentRequestID[40];
volatile int interrupt = 0;

/* Device Management Command Callback */
dmCommandCallback dmcb;

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
void manage(iotfclient *client, long lifetime, int supportDeviceActions, int supportFirmwareActions, char* reqId)
{
    LOG(DEBUG, "entry::");

    int rc = -1;
    char uuid_str[40];
    char payload[1500];

    if ( client->managed == 1 ) {
        LOG(ERROR, "Managed device is already initialized.");
        return;
    }

//     char *plFormat = "{\"d\": {\"metadata\":%s ,\"lifetime\":%ld ,\"supports\": {\"deviceActions\":%d,\"firmwareActions\":%d},\"deviceInfo\": {\"serialNumber\":\"%s\",\"manufacturer\":\"%s\",\"model\":\"%s\",\"deviceClass\":\"%s\",\"description\":\"%s\",\"fwVersion\":\"%s\",\"hwVersion\":\"%s\",\"descriptiveLocation\":\"%s\"}},\"reqId\": \"%s\"}";
    char *plFormat = "{\"d\": {\"lifetime\":%ld ,\"supports\": {\"deviceActions\":%d,\"firmwareActions\":%d},\"deviceInfo\": {}},\"reqId\": \"%s\"}";

    generateUUID(uuid_str);
    strcpy(currentRequestID,uuid_str);

//     sprintf(payload, plFormat, dmClient.DeviceData.metadata.metadata, 
    sprintf(payload, plFormat, 
        lifetime, supportDeviceActions, supportFirmwareActions, 
        // dmClient.DeviceData.deviceInfo.serialNumber,
        // dmClient.DeviceData.deviceInfo.manufacturer, 
        // dmClient.DeviceData.deviceInfo.model,
        // dmClient.DeviceData.deviceInfo.deviceClass,
        // dmClient.DeviceData.deviceInfo.description,
        // dmClient.DeviceData.deviceInfo.fwVersion,
        // dmClient.DeviceData.deviceInfo.hwVersion,
        //dmClient.DeviceData.deviceInfo.descriptiveLocation,
        uuid_str);

    LOG(DEBUG, "Send MANAGE request: %s", payload);
    rc = subscribeTopic(client, "iotdm-1/#", QoS0);

    rc = publishData(client, MANAGE, payload, QoS1);
    if (rc == 0) {
        strcpy(reqId, uuid_str);
        LOG(INFO, "Managed Device: reqId = %s", reqId);

        /* Subscribe to all DM commands */
        // rc = subscribeTopic(client, "iotdm-1/#", QoS0);

        client->managed = 1;
        memset((void *)&dmClient, 0, sizeof(dmClient));
        dmClient.client = client;
    } else {
        LOG(INFO, "Failed to send Managed Device request: rc=%d", rc);
    }

    LOG(DEBUG, "exit::");
}

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
void unmanage(iotfclient *client, char* reqId)
{
    LOG(DEBUG, "entry::");

    char uuid_str[40];
    int rc = -1;
    char data[70];

    if ( client->managed == 0 ) {
        LOG(ERROR, "Managed device is not initialized.");
        return;
    }

    generateUUID(uuid_str);
    strcpy(currentRequestID,uuid_str);

    sprintf(data,"{\"reqId\":\"%s\"}",uuid_str);

    rc = publishData(client, UNMANAGE, data, QoS0);
    if(rc == 0){
        strcpy(reqId, uuid_str);
        LOG(DEBUG, "reqId = %s",reqId);
        memset((void *)&dmClient, 0, sizeof(dmClient));
        client->managed = 0;
    }

    LOG(DEBUG, "exit:: rc = %d",rc);
}

//Publish actions response to IoTF platform
int publishActionResponse(char* publishTopic, char* data)
{
    LOG(DEBUG, "entry::");

    int rc = -1;
    
    LOG(DEBUG, "Topic - %s Payload - %s", publishTopic, data);
    rc = publishData(dmClient.client, publishTopic, data, QoS1);
    LOG(DEBUG, "RC from MQTTPublish = %d",rc);

    if(rc == 0) {
        rc = yield(100);
    }

    LOG(DEBUG, "exit:: rc = %d",rc);
    return rc;
}



//Handler for Firmware Download request
void messageFirmwareDownload(void *payload)
{
    LOG(DEBUG, "entry::");

    int rc = RESPONSE_ACCEPTED;
    char respmsg[300];

    cJSON * jsonPayload = cJSON_Parse(payload);
    strcpy(currentRequestID, cJSON_GetObjectItem(jsonPayload, "reqId")->valuestring);

    LOG(DEBUG,"messageFirmwareDownload with reqId:%s",currentRequestID);

    if (dmClient.DeviceData.mgmt.firmware.state != FIRMWARESTATE_IDLE)
    {
        rc = BAD_REQUEST;

        LOG(DEBUG,"Cannot download as the device is not in the idle state");
    }
    else
    {
        rc = RESPONSE_ACCEPTED;

        LOG(DEBUG,"Firmware Download Initiated");
    }

    sprintf(respmsg,"{\"rc\":%d,\"reqId\":%s}",rc,currentRequestID);

    publishData(dmClient.client, RESPONSE, respmsg, QoS1);

    if (rc == RESPONSE_ACCEPTED) {
        if ( dmcbFirmwareDownload != 0 ) {
            LOG(DEBUG,"Calling Firmware Download callback");
            (*dmcbFirmwareDownload)();
        } else {
            LOG(ERROR, "Firmware download callback is not set.");
        }
    }

    LOG(DEBUG, "exit::");
}

//Handler for Firmware update request
void messageFirmwareUpdate()
{
    LOG(DEBUG, "entry::");

    int rc;
    char respmsg[300];

    LOG(DEBUG,"Update Firmware Request, Firmware State: %d", dmClient.DeviceData.mgmt.firmware.state);

    if (dmClient.DeviceData.mgmt.firmware.state != FIRMWARESTATE_DOWNLOADED) {
        rc = BAD_REQUEST;
        LOG(DEBUG,"Firmware state is not in Downloaded state while updating");

    } else {
        rc = RESPONSE_ACCEPTED;
        LOG(DEBUG,"Firmware Update Initiated");
    }

    sprintf(respmsg, "{\"rc\":%d,\"reqId\":%s}", rc, currentRequestID);
    
    publishData(dmClient.client, RESPONSE, respmsg, QoS1);

    if (rc == RESPONSE_ACCEPTED) {
        if ( dmcbFirmwareUpdate != 0 ) {
            LOG(DEBUG,"Calling Firmware Update callback");
            (*dmcbFirmwareUpdate)();
        } else {
            LOG(ERROR, "Firmware Update callback is not set.");
        }
    }

    LOG(DEBUG, "exit::");
}

//Handler for Observe request
void messageObserve(void)
{
    LOG(DEBUG, "entry::");
    LOG(DEBUG,"Observe reqId: %s", currentRequestID);

    int rc = 200;
    char respMsg[256];
    char *plFormat = "{\"rc\":%d,\"reqId\":\"%s\",\"d\":{\"fields\":[{\"field\":\"mgmt.firmware\",\"value\":{\"state\":0,\"updateStatus\":0}}]}}";
    sprintf(respMsg, plFormat, rc, currentRequestID);

    LOG(INFO,"Response Message:%s", respMsg);

    //Publish the response to the IoTF
    publishData(dmClient.client, RESPONSE, respMsg, QoS1);

    LOG(DEBUG, "exit::");
}

//Handler for cancel observation request
void messageCancel(void *payload)
{
    LOG(DEBUG, "entry::");

    int i = 0;
    char respMsg[100];
    cJSON * jsonPayload = cJSON_Parse(payload);
    cJSON* jreqId = cJSON_GetObjectItem(jsonPayload, "reqId");
    strcpy(currentRequestID, jreqId->valuestring);

    LOG(DEBUG,"Cancel reqId: %s", currentRequestID);

    cJSON *d = cJSON_GetObjectItem(jsonPayload, "d");
    cJSON *fields = cJSON_GetObjectItem(d, "fields");
    //cJSON *fields = cJSON_GetObjectItem(jsonPayload,"fields");
    for (i = 0; i < cJSON_GetArraySize(fields); i++) {
        cJSON * field = cJSON_GetArrayItem(fields, i);
        cJSON* fieldName = cJSON_GetObjectItem(field, "field");

        // cJSON * value = cJSON_GetArrayItem(fields, i);

        LOG(DEBUG,"Cancel called for fieldName:%s", fieldName->valuestring);

        if (!strcmp(fieldName->valuestring, "mgmt.firmware")) {
            dmClient.bObserve = 0;
            sprintf(respMsg,"{\"rc\":%d,\"reqId\":%s}",RESPONSE_SUCCESS,currentRequestID);

            LOG(DEBUG,"Response Message:%s", respMsg);

            //Publish the response to the IoTF
            publishData(dmClient.client, RESPONSE, respMsg, QoS1);
        }
    }

    LOG(DEBUG, "exit::");
}

//Utility for LocationUpdate Handler
void updateLocationHandler(double latitude, double longitude, double elevation, char* measuredDateTime, char* updatedDateTime, double accuracy)
{
    LOG(DEBUG, "entry::");

    int rc = -1;
    char data[500];
    sprintf(data,"{\"d\":{\"longitude\":%f,\"latitude\":%f,\"elevation\":%f,\"measuredDateTime\":\"%s\",\"updatedDateTime\":\"%s\",\"accuracy\":%f},\"reqId\":\"%s\"}",
        latitude, longitude, elevation, measuredDateTime, updatedDateTime, accuracy, currentRequestID);

    rc = publishData(dmClient.client, UPDATE_LOCATION, data, QoS1);

    LOG(DEBUG, "exit:: rc = %d", rc);
}


//Handler for update location request
void updateLocationRequest(cJSON* value)
{
    LOG(DEBUG, "entry::");

    double latitude, longitude, elevation,accuracy;
    char* measuredDateTime;
    char* updatedDateTime;

    latitude = cJSON_GetObjectItem(value,"latitude")->valuedouble;
    longitude = cJSON_GetObjectItem(value,"longitude")->valuedouble;
    elevation = cJSON_GetObjectItem(value,"elevation")->valuedouble;
    accuracy = cJSON_GetObjectItem(value,"accuracy")->valuedouble;
    measuredDateTime = cJSON_GetObjectItem(value,"measuredDateTime")->valuestring;
    updatedDateTime = cJSON_GetObjectItem(value,"updatedDateTime")->valuestring;

    LOG(DEBUG,"Calling updateLocationHandler");

    updateLocationHandler(latitude, longitude, elevation,measuredDateTime,updatedDateTime,accuracy);

    LOG(DEBUG, "exit::");
}

//Handler for update Firmware request
void updateFirmwareRequest(cJSON* value) 
{
    LOG(DEBUG, "entry::");

    char response[100];

    strcpy(dmClient.DeviceData.mgmt.firmware.version, cJSON_GetObjectItem(value, "version")->valuestring);
    LOG(DEBUG,"Firmware Version: %s",dmClient.DeviceData.mgmt.firmware.version);

    strcpy(dmClient.DeviceData.mgmt.firmware.name, cJSON_GetObjectItem(value, "name")->valuestring);
    LOG(DEBUG,"Name: %s",dmClient.DeviceData.mgmt.firmware.name);

    strcpy(dmClient.DeviceData.mgmt.firmware.url, cJSON_GetObjectItem(value, "uri")->valuestring);
    LOG(DEBUG,"URI: %s",dmClient.DeviceData.mgmt.firmware.url);

    strcpy(dmClient.DeviceData.mgmt.firmware.verifier, cJSON_GetObjectItem(value, "verifier")->valuestring);
    LOG(DEBUG,"Verifier: %s",dmClient.DeviceData.mgmt.firmware.verifier);

    dmClient.DeviceData.mgmt.firmware.state = cJSON_GetObjectItem(value,"state")->valueint;
    LOG(DEBUG,"State: %d",dmClient.DeviceData.mgmt.firmware.state);

    dmClient.DeviceData.mgmt.firmware.updateStatus = cJSON_GetObjectItem(value,"updateStatus")->valueint;
    LOG(DEBUG,"updateStatus: %d",dmClient.DeviceData.mgmt.firmware.updateStatus);

    strcpy(dmClient.DeviceData.mgmt.firmware.updatedDateTime, cJSON_GetObjectItem(value, "updatedDateTime")->valuestring);
    LOG(DEBUG,"updatedDateTime: %s",dmClient.DeviceData.mgmt.firmware.updatedDateTime);

    sprintf(response, "{\"rc\":%d,\"reqId\":\"%s\"}", UPDATE_SUCCESS, currentRequestID);
    LOG(DEBUG,"Response: %s",response);

    publishData(dmClient.client, RESPONSE, response, QoS1);

    LOG(DEBUG, "exit::");
}

//Handler for update request from the server.
//It receives all the update requests like location, mgmt.firmware
//Currently only location and firmware updates are supported.
void messageUpdate(void *payload)
{
    LOG(DEBUG, "entry::");

    int i = 0;
    cJSON * jsonPayload = cJSON_Parse(payload);
    if (jsonPayload) {
        cJSON* jreqId = cJSON_GetObjectItem(jsonPayload, "reqId");
        strcpy(currentRequestID, jreqId->valuestring);
        LOG(DEBUG,"Update reqId: %s",currentRequestID);
        cJSON *d = cJSON_GetObjectItem(jsonPayload, "d");
        cJSON *fields = cJSON_GetObjectItem(d, "fields");

        for (i = 0; i < cJSON_GetArraySize(fields); i++) {
            cJSON * field = cJSON_GetArrayItem(fields, i);
            cJSON* fieldName = cJSON_GetObjectItem(field, "field");
            LOG(DEBUG,"Update request received for fieldName: %s",fieldName->valuestring);
            cJSON * value = cJSON_GetObjectItem(field, "value");

            if (!strcmp(fieldName->valuestring, "location")){
                LOG(DEBUG,"Calling updateLocationRequest");
                updateLocationRequest(value);
            }
            else if (!strcmp(fieldName->valuestring, "mgmt.firmware")){
                LOG(DEBUG,"Calling updateFirmwareRequest");
                updateFirmwareRequest(value);
            }
            else if (!strcmp(fieldName->valuestring, "metadata")){
                LOG(DEBUG,"METADATA not supported");
            }
            else if (!strcmp(fieldName->valuestring, "deviceInfo")){
                LOG(DEBUG,"deviceInfo not supported");
            }
            else{
                LOG(DEBUG, "Fieldname = %s",fieldName->valuestring);
            }
        }
        cJSON_Delete(jsonPayload);//Needs to delete the parsed pointer
    } else{
        LOG(DEBUG, "Error in parsing Json");
    }

    LOG(DEBUG, "exit::");
}

//Handler for responses from the server . Invoke the callback for the response.
//Callback needs to be invoked only if the request Id is matched. While yielding we
//receives the response for old request Ids from the platform. But we are interested only
//with the request Id action was initiated.
void messageResponse(void *payload, size_t sz)
{
    LOG(DEBUG, "entry::");

    if (dmcb != 0) {
        char *pl = (char*) malloc(sizeof(char)*sz+1);
        strcpy(pl,payload);
        char *reqID;
        char *status;

        reqID = strtok(pl, ",");
        status= strtok(NULL, ",");

        reqID = strtok(reqID, ":\"");
        reqID = strtok(NULL, ":\"");
        reqID = strtok(NULL, ":\"");

        status= strtok(status, "}");
        status= strtok(status, ":");
        status= strtok(NULL, ":");

        LOG(INFO, "DMResponse: Status:%s reqID:%s payload:%s", status, reqID, pl);
        if(!strcmp(currentRequestID,reqID))
        {
            LOG(DEBUG, "%s == %s, Calling the callback",currentRequestID,reqID);
            interrupt = 1;
            (*dmcb)(status, reqID, payload, sz);
        }
        else
        {
            LOG(DEBUG, "%s != %s, Calling the callback",currentRequestID,reqID);
        }
        free(pl);
    }

    LOG(DEBUG, "exit::");
}

//Handler for Reboot and Factory reset action requests received from the platform.
//Invoke the respective callback for action.
void messageForAction(char *topicName, void *payload, size_t sz, int isReboot)
{
    LOG(DEBUG, "entry::");

    if (dmcbReboot != 0 || dmcbFactoryReset != 0) {

        char *topic = strdup(topicName);

        char *pl = (char*) malloc(sz+1);
        memset((void *)pl, 0, sz+1);
        memcpy((void *)pl, payload, sz);

        strtok(topic, "/");
        strtok(NULL, "/");

        strtok(NULL, "/");
        strtok(NULL, "/");
        char *action = strtok(NULL, "/");

        char *reqID;

        reqID = strtok(pl, ":\"");
        reqID = strtok(NULL, ":\"");
        reqID = strtok(NULL, ":\"");

        strcpy(currentRequestID,reqID);
        LOG(INFO, "DMAction: reqId:%s action:%s", reqID, action);

        if (isReboot && dmcbReboot != 0) {
            LOG(DEBUG, "Calling Reboot callback");
            (*dmcbReboot)(reqID, action, payload, sz);
        } else if (dmcbFactoryReset != 0) {
            LOG(DEBUG, "Calling Factory Reset callback");
            (*dmcbFactoryReset)(reqID, action, payload, sz);
        }

        free(topic);
        free(pl);
    }

    LOG(DEBUG, "exit:: ");
}



/* Process device management messages */
int messageArrived_dm(void *context, char *topic, void *payload, size_t len) 
{
    LOG(DEBUG, "entry:: ");

    LOG(INFO, "DM Message. Context=%x Topic=%s", context, topic);

    if(!strcmp(topic, DMRESPONSE)){
        messageResponse(payload, len);
    } else if (!strcmp(topic, DMUPDATE)) {
        messageUpdate(payload);
    } else if (!strcmp(topic, DMOBSERVE)) {
        messageObserve();
    } else if (!strcmp(topic, DMCANCEL)) {
        messageCancel(payload);
    } else if (!strcmp(topic, DMREBOOT)) {
        messageForAction(topic, payload, len, 1);
    } else if (!strcmp(topic, DMFACTORYRESET)) {
        messageForAction(topic, payload, len, 0);
    } else if (!strcmp(topic, DMFIRMWAREDOWNLOAD)) {
        messageFirmwareDownload(payload);
    } else if (!strcmp(topic, DMFIRMWAREUPDATE)) {
        messageFirmwareUpdate();
    }

    LOG(DEBUG, "exit:: ");

    return 1;
}

/**
 * Function used to set the Device Management Command Callback function. This must be set for Managed Device.
 *
 * @param dmcb - A Function pointer to the dmCommandCallback. Its signature - void (*dmCommandCallback)(char *status, char *requestId, void *payload)
 * @return int return code
 */
void setDMCommandHandler(iotfclient  *client, dmCommandCallback handler)
{
    LOG(TRACE, "entry::");

    dmcb = handler;

    if (dmcb != NULL){
        LOG(DEBUG, "Client ID %s : Registered DM callabck to process the arrived messages", client->cfg.id);
    } else {
        LOG(DEBUG, "Client ID %s : DM Callabck not registered to process the arrived messages", client->cfg.id);
    }

    LOG(TRACE, "exit::");
}


/**
 * Notifies the IBM Watson IoT Platform response for action
 *
 * @param reqId request Id of the request that is received from the IBM Watson IoT Platform
 *
 * @param state state of the request that is request received from the IBM Watson IoT Platform
 *
 * @return int return code
 *
 */
int changeState(int rc)
{
    LOG(TRACE, "entry::");

    char response[128];

    switch(rc)
    {
        case 202:
            sprintf(response, "{\"rc\":\"%d\",\"message\":\"Device action is initiated.\",\"reqId\":\"%s\"}", rc, currentRequestID);
            break;
        case 500:
            sprintf(response, "{\"rc\":\"%d\",\"message\":\"Device action attempt failed.\",\"reqId\":\"%s\"}", rc, currentRequestID);
            break;
        case 501:
            sprintf(response, "{\"rc\":\"%d\",\"message\":\"Device action is not supported.\",\"reqId\":\"%s\"}", rc, currentRequestID);
            break;
        default:
            sprintf(response, "{\"rc\":\"%d\",\"message\":\"\",\"reqId\":\"%s\"}", rc, currentRequestID);
            break;
    }

    int res = publishActionResponse(RESPONSE, response);
    LOG(TRACE, "exit:: publishActionResponse = %d",res);

    return res;
}

/**
 * Update the firmware state while downloading firmware and
 * Notifies the IBM Watson IoT Platform with the updated state
 *
 * @param state Download state update received from the device
 *
 * @return int return code
 *
 */
int changeFirmwareDownloadState(int state)
{
    LOG(TRACE, "entry::");

    char firmwareMsg[300];
    int rc = -1;
    if (dmClient.bObserve) {
        dmClient.DeviceData.mgmt.firmware.state = state;
        sprintf(firmwareMsg, "{\"d\":{\"fields\":[{\"field\" : \"mgmt.firmware\",\"value\":{\"state\":%d}}]}}", state);
        rc = publishActionResponse(NOTIFY, firmwareMsg);
        LOG(TRACE, "publishActionResponse = %d",rc);
    } else{
        LOG(TRACE, "mgmt.firmware is not in observe state");
    }

    LOG(TRACE, "exit:: rc = %d",rc);
    return rc;
}


/**
 * Update the firmware update state while updating firmware and
 * Notifies the IBM Watson IoT Platform with the updated state
 *
 * @param state Update state received from the device while updating the Firmware
 *
 * @return int return code
 *
 */
int changeFirmwareUpdateState(int state)
{
    LOG(TRACE, "entry::");

    char firmwareMsg[300];
    int rc = -1;
    if (dmClient.bObserve) {
        dmClient.DeviceData.mgmt.firmware.updateStatus = state;
        sprintf(firmwareMsg,
            "{\"d\":{\"fields\":[{\"field\" : \"mgmt.firmware\",\"value\":{\"state\":%d,\"updateStatus\":%d}}]}}",
            dmClient.DeviceData.mgmt.firmware.state,state);
        rc = publishActionResponse(NOTIFY, firmwareMsg);
        LOG(TRACE, "publishActionResponse = %d",rc);
    } else{
        LOG(TRACE, "mgmt.firmware is not in observe state");
    }

    LOG(TRACE, "exit:: rc = %d",rc);

    return rc;
}


/**
 * Register Callback function to Reboot request
 *
 * @param handler Function pointer to the commandCallback. Its signature - void (*commandCallback)(char* Status, char* requestId,            void*       payload)
 *
 */
void setRebootHandler(dmCommandCallback handler)
{
    LOG(TRACE, "entry::");
    dmcbReboot = handler;

    if (dmcbReboot != NULL){
        LOG(INFO, "Reboot callabck is registered.");
    } else {
        LOG(INFO, "Reboot callabck is not registered.");
    }

    LOG(TRACE, "exit::");
}


/**
 * Register Callback function to Factory reset request
 *
 * @param handler Function pointer to the commandCallback.
 *
 */

void setFactoryResetHandler(dmCommandCallback handler)
{
    LOG(TRACE, "entry::");
    dmcbFactoryReset = handler;

    if (dmcbFactoryReset != NULL){
        LOG(INFO, "Factory Reset callabck is registered.");
    } else {
        LOG(INFO, "Factory Reset callabck is not registered.");
    }

    LOG(TRACE, "exit::");
}


/**
 * Register Callback function to Download Firmware
 *
 * @param handler Function pointer to the actionCallback.
 *
 */
void setFirmwareDownloadHandler(dmActionCallback handler)
{
    LOG(TRACE, "entry::");
    dmcbFirmwareDownload = handler;

    if (dmcbFirmwareDownload != NULL){
        LOG(INFO, "Firmware Download callabck is registered.");
    } else {
        LOG(INFO, "Firmware Download callabck is not registered.");
    }

    LOG(TRACE, "exit::");
}

/**
 * Register Callback function to Update Firmware
 *
 * @param handler Function pointer to the actionCallback.
 *
 */
void setFirmwareUpdateHandler(dmActionCallback handler)
{
    LOG(TRACE, "entry::");
    dmcbFirmwareUpdate = handler;

    if (dmcbFirmwareUpdate != NULL){
        LOG(INFO, "Firmware Update callabck is registered.");
    } else {
        LOG(INFO, "Firmware Update callabck is not registered.");
    }

    LOG(TRACE, "exit::");
}

