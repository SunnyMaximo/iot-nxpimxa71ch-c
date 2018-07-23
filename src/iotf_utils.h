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

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <libgen.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <ctype.h>

#define LOG(sev, fmts...) \
        logInvoke((LOGLEVEL_##sev), __FUNCTION__, __FILE__, __LINE__, fmts);

extern FILE *logger;

/* Utility functions */
void logInvoke(const LOGLEVEL level, const char * func, const char * file, int line, const char * fmts, ...);
void getServerCertPath(char** path);
void getTestCfgFilePath(char** cfgFilePath, char* fileName);
void buildPath(char **ptr, char *path);
char *trim(char *str);
void strCopy(char **dest, char *src);
int reconnect_delay(int i);
void freePtr(char* p);
void generateUUID(char* uuid_str);

 #endif
