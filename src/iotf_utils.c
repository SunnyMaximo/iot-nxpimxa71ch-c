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
 *                        -  Contains Utility Functions Definitions
 *                        -  Added logging feature
 * ----------------------------------------------------------------------------
 * Contrinutors for NXP Engine changes:
 *    Ranjan Dasgupta         - Initial changes to support NXP Engine
 *                            - Code cleanup/refactor and logging support
 *
 *******************************************************************************/

#include "iotfclient.h"
#include "iotf_utils.h"

#define MAX_LOG_BUFSIZE 8192

FILE *logger = NULL;
int logLevel = LOGLEVEL_INFO;

/**
 * LOG Level string
 */
static char * logLevelStr(int level)
{
    if (level == 1)
        return "ERROR";
    else  if (level == 2)
        return "WARN";
    else  if (level == 3)
        return "INFO";
    else  if (level == 4)
        return "DEBUG";
    else  if (level == 5)
        return "TRACE";

    return "UNKNOWN";
}


/**
 * Create log entry
 */
void logInvoke(const LOGLEVEL level, const char * func, const char * file, int line, const char * fmts, ...) 
{
    va_list args;
    char buf[MAX_LOG_BUFSIZE];

    if (logger != NULL && level <= logLevel ) 
    {
        va_start(args, fmts);
        vsnprintf(buf, MAX_LOG_BUFSIZE, fmts, args);
        va_end(args);

        fprintf(logger, "%s %s %s %d: %s: %s\n", __TIMESTAMP__, func, basename((char *)file), line, logLevelStr(level), buf);
    }
}


/** 
 * Function to enable logging and set log level.
 *
 * @param - level - set log level
 * @return - None
 **/
void initLogging(const LOGLEVEL level, const char * logFilePath) 
{
    if (logger == NULL)
    {
        char *logFile = "./iotfclient.log";   /* Default log file */
        if ( logFilePath && *logFilePath != '\0' )
            logFile = (char *)logFilePath;
        
        printf("Initialize logging. LogFile:%s LogLevel:%d\n", logFile, level);
        logLevel = level;
        if ((logger = fopen(logFile,"a")) != NULL) {
            fprintf(logger, "%s %s %s %d: INFO: %s\n", __TIMESTAMP__, __FUNCTION__, basename((char *) __FILE__ ), __LINE__, "----- INITIALIZE LOGGING -----");
            fprintf(logger, "%s %s %s %d: INFO: Log Level: %s\n", __TIMESTAMP__, __FUNCTION__, basename((char *) __FILE__ ), __LINE__, logLevelStr(level));
        }
        else
            fprintf(stderr, "ERROR: Unable to initialize logging. errno=%d\n", errno);
    }
    return;
}

/** 
* @param - Pointer to character string to hold the final path
* @param - Filepath to append to ./
*
* @returns - None
*
**/
void buildPath(char **ptr, char *filePath)
{
    LOG(DEBUG, "entry::");
    int pathLen;
    pathLen = strlen(filePath);
    *ptr = (char*)malloc(sizeof(char)*(pathLen+3));
    strcpy(*ptr,".");
    strcat(*ptr,filePath);
    (*ptr)[pathLen]='\0';
    LOG(INFO, "Built Path = %s",*ptr);
    LOG(DEBUG, "exit::");
}

/**
 * @param - Address of character pointer to store the server certificate path
 * @return - void
 **/
void getServerCertPath(char** path)
{
    LOG(DEBUG, "entry::");
    buildPath(path,"/IoTFoundation.pem");
    LOG(INFO, "Server Certificate Path = %s",*path);
    LOG(DEBUG,"exit::");
}

/**
 * @param - Character string to store the path
 *        - Config file name to be appended to path
 * @return - void
 **/
void getTestCfgFilePath(char** path, char* fileName)
{
    LOG(DEBUG, "entry::");
    buildPath(path,"/test/");
    *path = (char*)realloc(*path,strlen(*path)+strlen(fileName)+1);
    strcat(*path,fileName);
    LOG(INFO, "Test Config File Path = %s",*path);
    LOG(DEBUG,"exit::");
}

/**
 * Trim characters
 */
char *trim(char *str) 
{
    LOG(DEBUG,"entry::");
    size_t len = 0;
    char *frontp = str - 1;
    char *endp = NULL;

    if (str == NULL)
        return NULL;

    if (str[0] == '\0')
        return str;

    len = strlen(str);
    endp = str + len;

    while (isspace(*(++frontp)))
        ;

    while (isspace(*(--endp)) && endp != frontp)
        ;

    if (str + len - 1 != endp)
        *(endp + 1) = '\0';
    else if (frontp != str && endp == frontp)
        *str = '\0';

    endp = str;
    if (frontp != str) {
        while (*frontp)
            *endp++ = *frontp++;

        *endp = '\0';
    }

    LOG(DEBUG, "String After trimming = %s",str);
    LOG(DEBUG,"exit::");

    return str;
}

/** Function to copy source to destination string after allocating required memory.
 * @param - Address of character pointer as destination
 *        - Contents of source string
 * @return - void
 **/
void strCopy(char **dest, char *src)
{
    LOG(DEBUG,"entry::");

    if (strlen(src) >= 1)
    {
        *dest = (char*)malloc(sizeof(char)*(strlen(src)+1));
        strcpy(*dest,src);
        LOG(DEBUG,"Destination String = %s",*dest);
    } else {
        LOG(WARN, "Source String is empty");
    }

    LOG(DEBUG,"exit::");
}

/** Function to free the allocated memory for character string.
 * @param - Character pointer pointing to allocated memory
 * @return - void
 **/
void freePtr(char* p)
{
    LOG(DEBUG,"entry::");
    if (p != NULL)
        free(p);
    LOG(DEBUG,"exit::");
}

/* Reconnect delay time
  * depends on the number of failed attempts
  */
int reconnect_delay(int i) 
{
    if (i < 10) {
        return 3; // first 10 attempts try every 3 seconds
    }
    if (i < 20)
        return 60; // next 10 attempts retry after every 1 minute

    return 600;	// after 20 attempts, retry every 10 minutes
}

