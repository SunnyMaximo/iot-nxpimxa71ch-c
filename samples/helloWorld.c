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
 *    Ranjan Dasgupta - Initial drop of helloWorld sample
 * 
 *******************************************************************************/

/*
 * This sample read a device.cfg file passed as a command line parameter 
 * using option --config. Connects Watson IoT Platform and sends an event.
 *
 * Following client configuration can be set in device configuration file:
 *
 */

#include <stdio.h>
#include <signal.h>
#include <memory.h>
#include <stdlib.h>

#include "deviceclient.h"

char *configFilePath = NULL;
volatile int interrupt = 0;

/* Usage text */
void usage(void) {
    fprintf(stderr, "Usage: helloWorld --config config_file_path\n");
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

/* Main program */
int main(int argc, char *argv[])
{
    int rc = 0;
    iotfclient client;

    /* check for args */
    if ( argc < 2 )
        usage();

    /* get argument options */
    getopts(argc, argv);

    /* check for valid configuration file path */
    if ( !configFilePath || *configFilePath == '\0') {
        fprintf(stderr, "ERROR: Invalid Configuration file path is specified: NULL or empty path\n"); 
        exit(1);
    }

    /* Set signal handlers */
    signal(SIGINT, sigHandler);
    signal(SIGTERM, sigHandler);

    /* Initialize iotf configuration */
    int isGatewayClient = 0;
    rc = initialize_configfile(&client, configFilePath, isGatewayClient);
    if ( rc != 0 ) {
        fprintf(stderr, "ERROR: Failed to initialize configuration: rc=%d\n", rc);
        exit(1);
    }

    /* Invoke connection API */
    rc = connectiotf(&client);
    if ( rc != 0 ) {
        fprintf(stderr, "ERROR: Failed to connect to Watson IoT Platform: rc=%d\n", rc);
        exit(1);
    }

    /* Send some test event */
    char *data = "{\"d\" : {\"SensorID\": \"Test\", \"Reading\": 7 }}";

    while(!interrupt)
    {
        fprintf(stdout, "Send status event\n");
        rc = publishEvent(&client,"status","json", data , QOS0);
        fprintf(stdout, "RC from publishEvent(): %d\n", rc);
        sleep(2);
    }

    fprintf(stdout, "Received a signal - exiting publish event cycle.\n");

    /* Disconnect client */
    disconnect(&client);

    return 0;



/*
        int useCerts = argv[5][0] - '0';
	int rc = -1;
	if(!useCerts)
		rc = initialize(&client,argv[1],"internetofthings.ibmcloud.com",argv[2],
                		argv[3],"token",argv[4],NULL,useCerts,NULL,NULL,NULL,0);
	else
		rc = initialize(&client,argv[1],"internetofthings.ibmcloud.com",argv[2],
				argv[3],"token",argv[4],NULL,useCerts,argv[6],argv[7],argv[8],0);

	if(rc != SUCCESS){
		printf("Initialize returned rc = %d.\n Quitting..\n", rc);
		return 0;
	}

	rc = connectiotf(&client);

	if(rc != SUCCESS){
		printf("Connection returned rc = %d.\n Quitting..\n", rc);
		return 0;
	}
*/


}
