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
 *    Ranjan Dasgupta - Initial drop of deviceSample.c
 * 
 *******************************************************************************/

/*
 * This sample shows how-to develop a device code using Watson IoT Platform
 * iot-nxpa71ch-c client library, connect and interact with Watson IoT Platform
 * Service.
 * 
 * This sample includes the function/code snippets to perform the following actions:
 * - Initiliaze client library
 * - Configure device from configuration parameters specified in a configuration file
 * - Set client logging
 * - Enable error handling routines
 * - Send device events to WIoTP service
 * - Receive and process commands from WIoTP service
 *
 * NOTE: Device developers, comments tagged with DEV_NOTES describes WIoTP client API details.
 * 
 * SYNTAX:
 * deviceSample --config <config_file_path>
 *
 * Following client configuration can be set in device configuration file:
 *
 * org=<your Watson IoT Platform service organization ID>
 * type=NXP-A71CH-D                    # Pre-defined NXP A71CH Secure Element device type
 * useClientCertificates=1
 * rootCACertPath=/opt/iotnxpimxclient/certs/IoTFoundation.pem
 * useNXPEngine=1                      # Set to 1 for using NXP SSL engine
 * useCertsFromSE=1                    # Set to 1 for using certificates stored in Secured Element
 *
 * NOTE: User will have to register specified gateway type and id with WIoTP.
 */

#include <stdio.h>
#include <signal.h>
#include <memory.h>
#include <stdlib.h>
#include <unistd.h>

/* 
 * DEV_NOTES:
 * Include for this sample to use WIoTP iot-nxpa71ch-c device client APIs.
 */ 
#include "iotfclient.h"

char *configFilePath = NULL;
volatile int interrupt = 0;
char *progname = "deviceSample";

/* Usage text */
void usage(void) {
    fprintf(stderr, "Usage: %s --config config_file_path\n", progname);
    exit(1);
}

/* Signal handler - to support CTRL-C to quit */
void sigHandler(int signo) {
    fprintf(stdout, "Received signal: %d\n", signo);
    interrupt = 1;
}

/* Get and process command line options */
void getopts(int argc, char** argv)
{
    int count = 1;

    while (count < argc)
    {
        if (strcmp(argv[count], "--config") == 0)
        {
            if (++count < argc)
                configFilePath = argv[count];
            else
                usage();
        }
        count++;
    }
}

/* 
 * DEV_NOTES:
 * Device command callback function
 * Device developers can customize this function based on their use case
 * to handle device commands sent by WIoTP.
 * Set this callback function using API setCommandHandler().
 */
void  deviceCommandCallback (char* type, char* id, char* commandName, char *format, void* payload, size_t payloadSize)
{
    if ( type == NULL && id == NULL ) {
        fprintf(stdout, "Received device management status:");

    } else {
        fprintf(stdout, "Received device command:\n");
        fprintf(stdout, "Type=%s ID=%s CommandName=%s Format=%s Len=%d\n", type, id, commandName, format, (int)payloadSize);
        fprintf(stdout, "Payload: %s\n", (char *)payload);
    }

    /*
     * DEV_NOTES:
     * Device developers - add your custom code to process device commands
     */
}


/* Main program */
int main(int argc, char *argv[])
{
    int rc = 0;

    /* 
     * DEV_NOTES:
     * Specifiy variable for WIoT client object 
     */
    iotfclient client;

    /* check for args */
    if ( argc < 2 )
        usage();

    /* Set signal handlers */
    signal(SIGINT, sigHandler);
    signal(SIGTERM, sigHandler);

    /* get argument options */
    getopts(argc, argv);

    /* 
     * DEV_NOTES:
     * Initialize logging 
     * Specify logging level and log file path.
     * Supported log levels are LOGLEVEL_ERROR, LOGLEVEL_WARN, LOGLEVEL_INFO, LOGLEVEL_DEBUG, LOGLEVEL_TRACE
     * If log file path is NULL, the library use default log file path ./iotfclient.log 
     */
    initLogging(LOGLEVEL_INFO, NULL);

    /* 
     * DEV_NOTES:
     * Initialize iotf configuration using configuration options defined in the configuration file.
     * The variable configFilePath is set in getopts() function.
     * The initialize_configfile() function can be used to initialize both device and gateway client. 
     * Since this sample is for device client, set isGatewayClient to 0.
     */
    int isGatewayClient = 0;
    rc = initialize_configfile(&client, configFilePath, isGatewayClient);
    if ( rc != 0 ) {
        fprintf(stderr, "ERROR: Failed to initialize configuration: rc=%d\n", rc);
        exit(1);
    }

    /* 
     * DEV_NOTES:
     * Invoke connection API connectiotf() to connect to WIoTP.
     */
    rc = connectiotf(&client);
    if ( rc != 0 ) {
        fprintf(stderr, "ERROR: Failed to connect to Watson IoT Platform: rc=%d\n", rc);
        exit(1);
    }

    /*
     * DEV_NOTES:
     * Set device command callback using API setCommandHandler().
     * Refer to deviceCommandCallback() function DEV_NOTES for details on
     * how to process device commands received from WIoTP.
     */
    setCommandHandler(&client, deviceCommandCallback);

    /*
     * DEV_NOTES:
     * Invoke device command subscription API subscribeDeviceCommands().
     * The arguments for this API are commandName, format, QoS
     * If you want to subscribe to all commands of any format, set commandName and format to "+"
     * Note that client library also provides a convinence function subscribeCommands(iotfclient *client)
     * to subscribe to all commands. The QoS used in this convience function is QoS0. 
     */
    char *commandName = "+";
    char *format = "+";
    subscribeCommand(&client, commandName, format, QoS0);


    /*
     * DEV_NOTES:
     * Use publishEvent() API to send device events to Watson IoT Platform.
     */

    /* Sample event - this sample device will send this event once in every 10 seconds. */
    char *data = "{\"d\" : {\"SensorID\": \"Test\", \"Reading\": 7 }}";

    while(!interrupt)
    {
        fprintf(stdout, "Send status event\n");
        rc = publishEvent(&client,"status","json", data , QoS0);
        fprintf(stdout, "RC from publishEvent(): %d\n", rc);
        sleep(10);
    }

    fprintf(stdout, "Received a signal - exiting publish event cycle.\n");

    /*
     * DEV_NOTES:
     * Disconnect device
     */
    disconnect(&client);

    return 0;

}
