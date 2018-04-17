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
 * Contributors:
 *    Ranjan Dasgupta - Initial drop of gatewaySample.c
 * 
 *******************************************************************************/

/*
 * This sample gateway client code demonstrates the following gateway features:
 *     - Connect a gateway
 *     - Send gateway event
 *     - Subscribe to all gateway commands
 *     - Send event on dehalf of a device
 *     - Subscribe to a device command
 *
 * This sample reads configuration parameters from a configuration file.
 * The supported command line arguments are:
 *     -c or --config <config_file_path>  - Default is ../config/gateway.cfg
 *     -a or --action [gateway|device]    - Test action - default is gateway
 *     -t or --type <device_type>         - Default is TestDeviceType - used for device test
 *     -i or --id <device_id>             - Default is TestDevice - used for device test
 *
 * NOTE: User will have to register specified device type and id with WIoTP.
 *
 */

#include <stdio.h>
#include <signal.h>
#include <memory.h>
#include <stdlib.h>
#include <unistd.h>

#include "iotfclient.h"

char *configFilePath = NULL;
volatile int interrupt = 0;
int noEvents = 1;       /* Number of events to be sent */
int eventInterval = 10;  /* Interval in seconds         */
int testType = 0;        /* Gateway test                */

char *deviceType = "TestDeviceType"; 
char *deviceId = "TestDevice"; 

/* Usage text */
void usage(void) {
    fprintf(stderr, "Usage: gatewaySample_connect_device --config config_file_path\n");
    fprintf(stderr, "-c or --config <config_file_path>  - Default is ../config/gateway.cfg \n");
    fprintf(stderr, "-a or --action [gateway|device]    - Test action - default is gateway\n");
    fprintf(stderr, "-t or --type <device_type>         - Default is TestDeviceType - used for device test\n");
    fprintf(stderr, "-i or --id <device_id>             - Default is TestDevice - used for device test\n");
    exit(1);
}

/* Signal handler */
void sigHandler(int signo) {
    fprintf(stdout, "Received signal: %d\n", signo);
    interrupt = 1;
}

/* Get and process command line options */
void getopts(int argc, char** argv)
{
    int count = 1;

    while (count < argc) {
        if ( (strcmp(argv[count], "-c") == 0) || (strcmp(argv[count], "--config") == 0) ) {
            if (++count < argc)
                configFilePath = argv[count];
            else {
                fprintf(stderr, "ERROR: Config file is not specified\n");
                usage();
            }
        } else if ( (strcmp(argv[count], "-a") == 0) || (strcmp(argv[count], "--action") == 0) ) {
            if (++count < argc) {
                char *actionStr = argv[count];
                if (strcmp(actionStr, "gateway") == 0) {
                    testType = 0;
                } else if (strcmp(actionStr, "device") == 0) {
                    testType = 1;
                } else {
                    fprintf(stderr, "ERROR: Invalid action is specified: %s\n", actionStr?actionStr:"NULL");
                    usage();
                }
            } else {
                fprintf(stderr, "ERROR: Action is not specified\n");
                usage();
            }
        } else if ( (strcmp(argv[count], "-t") == 0) || (strcmp(argv[count], "--type") == 0) ) {
            if (++count < argc) {
                char *tmpStr = argv[count];
                if (tmpStr && *tmpStr != '\0') {
                    deviceType = tmpStr;
                } else {
                    fprintf(stderr, "ERROR: Invalid device type is specified: %s\n", tmpStr?tmpStr:"NULL");
                    usage();
                }
            } else {
                fprintf(stderr, "ERROR: Device type is not specified\n");
                usage();
            }
        } else if ( (strcmp(argv[count], "-i") == 0) || (strcmp(argv[count], "--id") == 0) ) {
            if (++count < argc) {
                char *tmpStr = argv[count];
                if (tmpStr && *tmpStr != '\0') {
                    deviceId = tmpStr;
                } else {
                    fprintf(stderr, "ERROR: Invalid device Id is specified: %s\n", tmpStr?tmpStr:"NULL");
                    usage();
                }
            } else {
                fprintf(stderr, "ERROR: Device Id is not specified\n");
                usage();
            }
        } else {
            char *tmpStr = argv[count];
            fprintf(stderr, "ERROR: Invalid option is specified: %s\n", tmpStr?tmpStr:"NULL");
            usage();
        }
        count++;
    }
}


/* Callback function to process Gateway Commands */
void gatewayCommandCallback (char* type, char* id, char* commandName, char *format, void* payload, size_t payloadSize)
{
    fprintf(stdout, "Gateway Command Received -----\n");
    fprintf(stdout, "Type=%s ID=%s CommandName=%s Format=%s Len=%d\n", type, id, commandName, format, (int)payloadSize);
    fprintf(stdout, "Payload: %s\n", (char *)payload);
}


/* Main program */
int main(int argc, char *argv[])
{
    int rc = 0;
    iotfclient client;

    /* get argument options */
    getopts(argc, argv);

    /* check for configuration file path */
    if ( !configFilePath || *configFilePath == '\0') {
        /* set to default */
        configFilePath = "../config/gateway.cfg";
    }

    /* Set signal handlers */
    signal(SIGINT, sigHandler);
    signal(SIGTERM, sigHandler);

    /* Initialize iotf configuration */
    int isGatewayClient = 1;
    rc = initialize_configfile(&client, configFilePath, isGatewayClient);
    if ( rc != 0 ) {
        fprintf(stderr, "ERROR: Failed to initialize gateway configuration: rc=%d\n", rc);
        exit(1);
    }

    /* Invoke connection API */
    rc = connectiotf(&client);
    if ( rc != 0 ) {
        fprintf(stderr, "ERROR: Failed to connect to Watson IoT Platform: rc=%d\n", rc);
        exit(1);
    }

    fprintf(stdout, "Gateway Type: %s\n", client.cfg.type);
    fprintf(stdout, "Gateway Id: %s\n", client.cfg.id);

    /* Sample event */
    char *datafmt = "{\"d\":{\"TestCycle\":%d}}";
    int cycle = 0;
    char data[32];

    /* Gateway Action */
    if ( testType == 0 ) {

        fprintf(stdout, "Performing Gateway Actions\n");

        /* Set gateway command callback */
        setCommandHandler(&client, gatewayCommandCallback);

        /* Subscribe to gateway commands */
        rc = subscribeToGatewayCommands(&client);
        fprintf(stdout, "RC from subscribeToGatewayCommands(): %d\n", rc);

        fprintf(stdout, "Send events once in every %d seconds. Stop after sending %d events.\n", eventInterval, noEvents);
        while (!interrupt) {
            if ( cycle >= noEvents ) break;
            sprintf(data, datafmt, cycle);
            rc = publishGatewayEvent(&client, "status", "json", data , QoS1);
            fprintf(stdout, "RC from publishGatewayEvent(): %d\n", rc);
            cycle += 1;
            sleep(eventInterval);
        }

    } else {     /* Device Action */

        fprintf(stdout, "Performing Device Actions\n");
        fprintf(stdout, "Device Type: %s\n", deviceType);
        fprintf(stdout, "Device Id: %s\n", deviceId);

        /* Set gateway command callback */
        setCommandHandler(&client, gatewayCommandCallback);

        /* Subscribe to gateway notifications */
        rc = subscribeToGatewayNotification(&client);
        fprintf(stdout, "RC from subscribeToGatewayNotification(): %d\n", rc);

        /* Subscribe to gateway commands */
        // rc = subscribeToGatewayCommands(&client);
        // fprintf(stdout, "RC from subscribeToGatewayCommands(): %d\n", rc);

        /* Subscribe to device commands */
        // rc = subscribeToDeviceCommands(&client, deviceType, deviceId, "+", "+", QoS2);
        // fprintf(stdout, "RC from subscribeToDeviceCommands(): %d\n", rc);

        fprintf(stdout, "Send events once in every %d seconds. Stop after sending %d events.\n", eventInterval, noEvents);
        while (!interrupt) {
            if ( cycle >= noEvents ) break;
            sprintf(data, datafmt, cycle);
            rc = publishDeviceEvent(&client, deviceType, deviceId, "status", "json", data, QoS1);
            fprintf(stdout, "RC from publishDeviceEvent(): %d\n", rc);
            cycle += 1;
            sleep(eventInterval);
        }

    }

    fprintf(stdout, "EXIT: Received a signal or test cycle complete.\n");

    /* Disconnect client */
    disconnect(&client);

    return 0;

}

