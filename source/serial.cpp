#include "serial.hpp"
#include <stdio.h>
#include <stdbool.h>
#include <3ds.h>
#include <string.h>
#include <stdlib.h>

static char secinfoserial[0x10];
static char twlnserial[0x10];
static char sdessentialserial[0x10];
static char nandessentialserial[0x10];

char* getSecinfoSerial() {
    return secinfoserial;
}

char* getTWLNSerial() {
    return twlnserial;
}

char* getSDEssentialSerial() {
    return sdessentialserial;
}

char* getNANDEssentialSerial() {
    return nandessentialserial;
}


void setSDEssentialSerial() {
    FILE *f = fopen("sdmc:/gm9/out/essential.exefs", "rb");
    if (!f) {
        sdessentialserial[0] = '\0';
        return;
    }
    fseek(f, 0x502, SEEK_SET);
    fread(sdessentialserial, 0xF, 1, f);
    for (int i = 0;i < 0xF;i++) {
        if (sdessentialserial[i] == 0) {
            sdessentialserial[i] = '\0';
            break;
        }
    }
    fclose(f);
    return;
}

void setNANDEssentialSerial() {
    FS_Archive rawnandarchive;
    Result res = FSUSER_OpenArchive(&rawnandarchive, ARCHIVE_NAND_W_FS, fsMakePath(PATH_EMPTY, ""));
    if (R_FAILED(res)) {
        nandessentialserial[0] = '\0';
        return;
    }
    Handle file;
    res = FSUSER_OpenFile(&file, rawnandarchive, fsMakePath(PATH_UTF16, u"/"), FS_OPEN_READ, 0);
    if (R_FAILED(res)) {
        nandessentialserial[0] = '\0';
        FSUSER_CloseArchive(rawnandarchive);
        return;
    }
    res = FSFILE_Read(file, NULL, 0x200 + 0x502, nandessentialserial, 0xF); //0x200=offset of essential in nand, 0x502=offset of secinfo serial in normal essential
    if (R_FAILED(res)) {
        nandessentialserial[0] = '\0';
        FSFILE_Close(file);
        FSUSER_CloseArchive(rawnandarchive);
        return;
    }
    
    for (int i = 0;i < 0xF;i++) {
        if (nandessentialserial[i] == 0) {
            nandessentialserial[i] = '\0';
            break;
        }
    }
    FSFILE_Close(file);
    FSUSER_CloseArchive(rawnandarchive);
    return;
}

void setSecinfoSerial() {
    cfguInit();
    CFGI_SecureInfoGetSerialNumber((u8*)secinfoserial);
    for (int i = 0;i < 0xF;i++) {
        if (secinfoserial[i] == 0) {
            secinfoserial[i] = '\0';
            break;
        }
    }
    cfguExit();
}

void setTWLNSerial() {
    Handle file;

    Result ret = FSUSER_OpenFileDirectly(&file, ARCHIVE_NAND_TWL_FS, fsMakePath(PATH_EMPTY, ""), fsMakePath(PATH_ASCII, "/sys/log/inspect.log"), FS_OPEN_READ, 0);

    if (R_FAILED(ret)) {
        sprintf(twlnserial, "1. ret %08lx", ret);
        return;
    }
    u64 size;
    ret = FSFILE_GetSize(file, &size);

    if (R_FAILED(ret)) {
        sprintf(twlnserial, "2. ret %08lx", ret);
        FSFILE_Close(file);
        return;
    }
    char *log = new char[size + 1];
    ret = FSFILE_Read(file, NULL, 0, log, size);

    if (R_FAILED(ret)) {
        sprintf(twlnserial, "3. ret %08lx", ret);
        FSFILE_Close(file);
        delete[] log;
        return;
    }
    ret = FSFILE_Close(file);
    if (R_FAILED(ret)) {
        sprintf(twlnserial, "4. ret %08lx", ret);
        FSFILE_Close(file);
        delete[] log;
        return;
    }

    char* serialentry = strstr(log, "SerialNumber=") + 13;
    for (int i = 0;i<0xF;i++) {
        if (serialentry[i] == '\n') {
            serialentry[i] = '\0';
            strncpy(twlnserial, serialentry, i+1);
            delete[] log;
            return;
        }
    }
    delete[] log;
    twlnserial[0] = '\0';
    return;
}