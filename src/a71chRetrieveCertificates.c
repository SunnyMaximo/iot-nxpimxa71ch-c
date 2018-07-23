/**
 * @file a71chRetrieveCertificates.c
 * @author NXP Semiconductors
 * @version 1.0
 * @par License
 * Copyright(C) NXP Semiconductors, 2018
 * All rights reserved.
 *
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * A7-series security ICs.  This software is supplied "AS IS" without any
 * warranties of any kind, and NXP Semiconductors and its licensor disclaim any and
 * all warranties, express or implied, including all implied warranties of
 * merchantability, fitness for a particular purpose and non-infringement of
 * intellectual property rights.  NXP Semiconductors assumes no responsibility
 * or liability for the use of the software, conveys no license or rights under any
 * patent, copyright, mask work right, or any other intellectual property rights in
 * or to any products. NXP Semiconductors reserves the right to make changes
 * in the software without notification. NXP Semiconductors also makes no
 * representation or warranty that such application will be suitable for the
 * specified use without further testing or modification.
 *
 * Permission to use, copy and modify this software is hereby granted,
 * under NXP Semiconductors' and its licensor's relevant copyrights in
 * the software, without fee, provided that it is used in conjunction with
 * NXP Semiconductors products. This copyright, permission, and disclaimer notice
 * must appear in all copies of this code.
 *
 * @par Description:
 *
 * Precondition:
 * - A71CH has been provisioned with appropriate credentials to support
 *   Watson IoT Platform:
 *   Device Certificate Index is 0
 *   Gateway Certificate Index is 1
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <openssl/ssl.h>
#include <openssl/engine.h>

#include "ax_api.h"
#include "HLSEAPI.h"

#define SUCCESS 0
#define FAILURE 1

#define OBJECT_CLASS(OBJECT_TYPE)     (U8)((OBJECT_TYPE & 0x00FF0000) >> 16)
#define OBJECT_INDEX(OBJECT_HANDLE)   (U8)((OBJECT_HANDLE & 0x000000FF))
#define HLSE_CERTIFICATE    0x00090000

int dbgPrint = 0;

/* Print a hex array utility */
static void printDataAsHex(U8* data, U16 dataLen)
{
    int i;

    if ( dbgPrint == 0 ) return;

    for (i=0; i<(int)dataLen; i++) {
        if ( (i%16 == 0) && (i != 0) ) { printf("\n"); }
        printf("%02x:", data[i]);
    }
    printf("\n");
}

/* Retrieve Reference key and write to file */
static int getReferenceKey(int keyIndex, int storageClass, char *keyFilepath)
{
    int i, j;
    U16 retCode = 0;
    U8 pubkey[4098];
    U16 pubkeyLen = sizeof(pubkey);
    EC_GROUP *group = NULL;
    EC_POINT *pub_key = NULL;
    int key_field_len;
    BIGNUM *X = NULL;
    BIGNUM *Y = NULL;
    BIGNUM *bn_priv = NULL;
    int nid = NID_X9_62_prime256v1;
    EC_KEY *eckey;
    BIO *out = NULL;
    U8 privKey[96];
    U16 privKeyLen;

    retCode = A71_GetPublicKeyEccKeyPair(0, pubkey, &pubkeyLen);
    if ( retCode != SW_OK ) {
        printf("Failed to get public key: %d\n", (int)retCode);
        return 1;
    } else {
        if ( dbgPrint == 1) printf("Public key length: %d\n", (int)pubkeyLen);
        printDataAsHex(pubkey, pubkeyLen);

        eckey = EC_KEY_new();
        group = EC_GROUP_new_by_curve_name(nid);
        EC_KEY_set_group(eckey, group);
        key_field_len = (EC_GROUP_get_degree(group)+7)/8;
        EC_KEY_generate_key(eckey);
        privKeyLen = (U16)BN_bn2bin( EC_KEY_get0_private_key(eckey), privKey);
        privKey[privKeyLen-1] = keyIndex;
        privKey[privKeyLen-2] = storageClass;
        for (j=0; j<2; j++) {
            for (i=3; i<7; i++) {
                privKey[privKeyLen-i-(j*4)] = (U8)(0xA5A6B5B6 >> 8*(i-3));
            }
        }
        privKey[0] = 0x10;
        for (i=11; i<(privKeyLen); i++) { privKey[privKeyLen-i] = 0x00; }
        bn_priv = BN_bin2bn(privKey, privKeyLen, NULL);
        EC_KEY_set_private_key(eckey, bn_priv);
        X = BN_bin2bn(&pubkey[1], key_field_len, NULL);
        Y = BN_bin2bn(&pubkey[1+key_field_len], key_field_len, NULL);
        pub_key = EC_POINT_new(group);
        EC_POINT_set_affine_coordinates_GFp(group, pub_key, X, Y, NULL);
        EC_KEY_set_public_key(eckey, pub_key);
        EC_KEY_set_asn1_flag(eckey, nid);
        out = BIO_new(BIO_s_file());
        BIO_write_filename(out, (void *)keyFilepath);
        if (!PEM_write_bio_ECPrivateKey(out, eckey, NULL, NULL, 0, NULL, NULL)) {
            printf("Unable to write Key\n");
        }

        EC_POINT_free(pub_key);
        EC_GROUP_free(group);
        BN_free(X);
        BN_free(Y);
        BIO_vfree(out);
        EC_KEY_free(eckey);
    }

    return 0;
}

/* Get certificate file size */
static U16 GetFileSize(U8* data)
{
    U16 dataLen = 6;

    U8 TagLen = 1;
    if ((data[1] & 0x1F) == 0x1F) {
        TagLen = 2;
        if (data[2] & 0x80) {
            TagLen = 3;
        }
    }
    if (data[TagLen] & 0x80){
        if (data[TagLen] == 0x81)
            dataLen = data[TagLen + 1] + 2 + TagLen;
        else {
            dataLen = data[TagLen + 1] * 256 + data[TagLen + 2] + 3 + TagLen;
        }
    }
    else
        dataLen = data[TagLen] + 1 + TagLen;

    return dataLen;
}

/* Get Unique ID */
static char * getDeviceId(void) {
    U8 uid[8096];
    U16 uidLen = 8096;
    U16 sw;
    U8 certUid[10];
    char deviceId[21];

    sw = A71_GetUniqueID(uid, &uidLen);
    if ( sw == SW_OK ) {
        int idx = 0;
        char *ptr = &deviceId[0];
        int i = 0;

        printDataAsHex(uid, uidLen);

        certUid[idx++] = uid[2];
        certUid[idx++] = uid[3];
        certUid[idx++] = uid[8];
        certUid[idx++] = uid[9];
        certUid[idx++] = uid[10];
        certUid[idx++] = uid[11];
        certUid[idx++] = uid[12];
        certUid[idx++] = uid[13];
        certUid[idx++] = uid[14];
        certUid[idx++] = uid[15];

        for (i=0; i<idx; i++) {
            ptr += sprintf(ptr, "%02X", certUid[i]);
        }        
        
        return (strdup(deviceId)); 
    } else {
        printf("A71_GetUniqueID returned error: %d\n", (int)sw);
    }

    return NULL;
}

/* Get device and gateway certificates, and refernce key from SE and write to a file 
 * On success returns unique device ID
 */
char * a71ch_retrieveCertificatesFromSE(char *certDir) 
{
    U16 retCode = 0;
    char *deviceId = NULL;
    char filePath[2048];
    X509 *x509 = NULL;
    BIO  *bio;
    FILE *fl = NULL;
    U8 Atr[64];
    U16 AtrLen = sizeof(Atr);
    SmCommState_t commState;
    U16 selectResponse = 0;
    U8 debugOn = 0;
    U8 restrictedKpIdx = 0;
    U8 transportLockState = 0;
    U8 scpState = 0;
    U8 injectLockState = 0;
    U16 gpStorageSize = 0;
    U8 dataRead[32];
    U8 dataReadByteSize = sizeof(dataRead);
    int noEntries = 0;
    int nObj = 1;
    int i = 0;
    int devCertIndex = 0;
    int gwCertIndex = 1;

    /* Connect to Security Module */
    retCode = SM_Connect(&commState, Atr, &AtrLen);
    if (retCode == SMCOM_COM_FAILED && AtrLen <= 0) {
        printf("SM_Connect failed: %d\n", retCode);
        goto done;
    }

    retCode = A71_GetModuleInfo(&selectResponse, &debugOn, &restrictedKpIdx, &transportLockState, &scpState, &injectLockState, &gpStorageSize);
    if ( retCode != SW_OK ) {
        printf("A71_GetModuleInfo() failed: %d\n", retCode);
        goto done;
    }

    /* Get UID - device ID */
    deviceId = getDeviceId();
    if ( deviceId == NULL ) {
        retCode = -1;
        goto done;
    }

    /* Get General Purpose data */
    retCode = A71_GetGpData(gpStorageSize - dataReadByteSize, dataRead, dataReadByteSize);
    if (retCode != SW_OK) {
        printf("A71_GetGpData() failed: %d\n", retCode);
        goto done;
    }

    noEntries = dataRead[dataReadByteSize - 1];
    for ( i=0; i<noEntries; i++) {
        U8 class = dataRead[dataReadByteSize - 2 - nObj * 6 + 0];
        U8 index = dataRead[dataReadByteSize - 2 - nObj * 6 + 1];
        U16 offset = dataRead[dataReadByteSize - 2 - nObj * 6 + 4] * 256 | dataRead[dataReadByteSize - 2 - nObj * 6 + 5];

        if ( class == OBJECT_CLASS(HLSE_CERTIFICATE) ) {
            if ( (int)index == devCertIndex || (int)index == gwCertIndex ) {

                U8 header[6];
                U16 certLength = 0;
                U8 objData[8096];

                retCode = A71_GetGpData(offset, header, 6);
                if (retCode != SW_OK) {
                    printf("A71_GetGpData() failed to get header: index:%d retCode:%d\n", (int)index, retCode);
                    goto done;
                }

                certLength = GetFileSize(header);
                if ( dbgPrint == 1 )  printf("Certificate Size: %d\n", certLength);

                retCode = A71_GetGpData(offset, objData, certLength);
                if (retCode != SW_OK) {
                    printf("A71_GetGpData() failed to get object data: ndex:%d retCode:%d\n", (int)index, retCode);
                    goto done;
                }
                printDataAsHex(objData, certLength);

                /* Set certificate file path */
                if ( certDir == NULL ) certDir = ".";
                if ( (int)index == devCertIndex ) {
                    sprintf(filePath, "%s/%s_device_ec_pem.crt", certDir, deviceId);
                } else {
                    sprintf(filePath, "%s/%s_gateway_ec_pem.crt", certDir, deviceId);
                }

                fl = fopen(filePath, "w+");
                bio = BIO_new_mem_buf((void *)objData, certLength);
                if ( bio ) {
                    x509 = d2i_X509_bio(bio, NULL);
                    if ( x509 ) {
                        PEM_write_X509(fl, x509);
                    } else {
                        printf("X509 Error for filePath: %s\n", filePath);
                        retCode = 1;
                        goto done;
                    }
                }
                fclose(fl);
            }
        }
        nObj++;
    }

    /* Get reference key */
    int keyIndex = 0;
    int storageClass = A71CH_SSI_KEY_PAIR;
    sprintf(filePath, "%s/%s.ref_key", certDir, deviceId);
    retCode = getReferenceKey(keyIndex, storageClass, filePath);
    if (retCode != SUCCESS) {
        printf("Failed to retrieve reference key.\n");
        goto done;
    }

done:
    if ( AtrLen > 0 ) {
        SM_Close(SMCOM_CLOSE_MODE_STD);
    }
    if ( retCode != 0 &&  deviceId != NULL) {
        free(deviceId);
        deviceId = NULL;
    }

    return deviceId;
}

