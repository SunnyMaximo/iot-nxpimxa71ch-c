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
 * Contrinutors for NXP Engine changes:
 *    Ranjan Dasgupta         - Initial changes to support NXP Engine
 *
 *******************************************************************************/

#if !defined(MANAGEDDEVICE_H_)
#define MANAGEDDEVICE_H_

#define DLLImport extern
#define DLLExport __attribute__ ((visibility ("default")))

#include <stdio.h>

#include "iotfclient.h"
#include "iotf_utils.h"


/* Device Management requests */
#define MANAGE               "iotdevice-1/mgmt/manage"
#define UNMANAGE             "iotdevice-1/mgmt/unmanage"
#define NOTIFY               "iotdevice-1/notify"
#define RESPONSE             "iotdevice-1/response"
#define UPDATE_LOCATION      "iotdevice-1/device/update/location"
#define CREATE_DIAG_ERRCODES "iotdevice-1/add/diag/errorCodes"
#define CLEAR_DIAG_ERRCODES  "iotdevice-1/clear/diag/errorCodes"
#define ADD_DIAG_LOG         "iotdevice-1/add/diag/log"
#define CLEAR_DIAG_LOG       "iotdevice-1/clear/diag/log"

#define DMRESPONSE           "iotdm-1/response"
#define DMUPDATE             "iotdm-1/device/update"
#define DMOBSERVE            "iotdm-1/observe"
#define DMCANCEL             "iotdm-1/cancel"
#define DMREBOOT             "iotdm-1/mgmt/initiate/device/reboot"
#define DMFACTORYRESET       "iotdm-1/mgmt/initiate/device/factory_reset"
#define DMFIRMWAREDOWNLOAD   "iotdm-1/mgmt/initiate/firmware/download"
#define DMFIRMWAREUPDATE     "iotdm-1/mgmt/initiate/firmware/update"

/* Device Action return response */
#define REBOOT_INITIATED            202
#define REBOOT_FAILED               500
#define REBOOT_NOTSUPPORTED         501

#define FACTORYRESET_INITIATED      202
#define FACTORYRESET_FAILED         500
#define FACTORYRESET_NOTSUPPORTED   501

#define UPDATE_SUCCESS              204
#define RESPONSE_SUCCESS            200

#define FIRMWARESTATE_IDLE          0
#define FIRMWARESTATE_DOWNLOADING   1
#define FIRMWARESTATE_DOWNLOADED    2

#define FIRMWAREUPDATE_SUCCESS             0
#define FIRMWAREUPDATE_INPROGRESS          1
#define FIRMWAREUPDATE_OUTOFMEMORY         2
#define FIRMWAREUPDATE_CONNECTIONLOST      3
#define FIRMWAREUPDATE_VERIFICATIONFAILED  4
#define FIRMWAREUPDATE_UNSUPPORTEDIMAGE    5
#define FIRMWAREUPDATE_INVALIDURL          6

#define RESPONSE_ACCEPTED                  202
#define BAD_REQUEST                        400

/* Callbacks */
dmCommandCallback   dmcb;
dmCommandCallback   dmcbReboot;
dmCommandCallback   dmcbFactoryReset;
dmActionCallback    dmcbFirmwareDownload;
dmActionCallback    dmcbFirmwareUpdate;

/* Device management topics */
/*
const char* dmUpdate = "iotdm-1/device/update";
const char* dmObserve = "iotdm-1/observe";
const char* dmCancel = "iotdm-1/cancel";
const char* dmReboot = "iotdm-1/mgmt/initiate/device/reboot";
const char* dmFactoryReset = "iotdm-1/mgmt/initiate/device/factory_reset";
const char* dmFirmwareDownload = "iotdm-1/mgmt/initiate/firmware/download";
const char* dmFirmwareUpdate = "iotdm-1/mgmt/initiate/firmware/update";
*/

/* structure for device information */
struct DeviceInfo{
    char serialNumber[20];
    char manufacturer[20];
    char model[20];
    char deviceClass[20];
    char description[30];
    char fwVersion[10];
    char hwVersion[10];
    char descriptiveLocation[20];
};

/* structure for device location */
struct DeviceLocation{
    double latitude;
    double longitude;
    double elevation;
    time_t measuredDateTime;
    double accuracy;
};

/* structure for device actions */
struct DeviceAction{
    int status;
    char message[50];
    char typeId[10];
    char deviceId[10];
};

/* structure for device firmware attributes */
struct DeviceFirmware{
    char version[10];
    char name[20];
    char url[100];
    char verifier[20];
    int state;
    int updateStatus;
    char deviceId[40];
    char typeId[20];
    char updatedDateTime[20];
};

/* structure for metadata of device */
struct DeviceMetadata{
    char metadata[10];
};

/* structure for device management */
struct DeviceMgmt{
    struct DeviceFirmware firmware;
};

/* structure for device data */
struct deviceData{
    struct DeviceInfo deviceInfo;
    struct DeviceLocation deviceLocation;
    struct DeviceMgmt mgmt;
    struct DeviceMetadata metadata;
    struct DeviceAction deviceAction;
};

struct managedDevice{
    int supportDeviceActions ;
    int supportFirmwareActions ;
    int bManaged ;
    int bObserve;
    char responseSubscription[50];
    struct deviceData DeviceData;
    iotfclient *client;
};

typedef struct managedDevice ManagedDevice;

ManagedDevice dmClient;


#endif

