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
 *
 * NOTE: This code is taken from iot-embeddedc project and customized to
 *       support NXP SSL Engine.
 *
 * iot-embeddedc project Contributors:
 *    Lokesh Haralakatta  -  Initial implementation
 *                        -  Contains the Utility Functions declarations
 * ----------------------------------------------------------------------------
 * Contrinutors for NXP Engine changes:
 *    Ranjan Dasgupta         - Initial changes to support NXP Engine
 *
 *******************************************************************************/
 #ifndef IOTF_UTILS_H_
 #define IOTF_UTILS_H_

 #include<stdlib.h>
 #include<stdio.h>
 #include<string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <ctype.h>

 #define LOG(logHeader,msg)       if(logger != NULL) fprintf(logger,"%s:%s\n",logHeader,msg);
 #define LOG_BUF 512

 extern FILE *logger;

 typedef enum
 {
     QOS0,
     QOS1,
     QOS2
 } QoS;

#define SUCCESS 0

//Utility Functions
 void enableLogging();
 void disableLogging();
 void getServerCertPath(char** path);
 void getSamplesPath(char** path);
 void getTestCfgFilePath(char** cfgFilePath, char* fileName);
 void buildPath(char **ptr, char *path);
 char *trim(char *str);
 void strCopy(char **dest, char *src);
 int reconnect_delay(int i);
 void freePtr(char* p);

 #endif
