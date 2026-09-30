#include <curl/curl.h> //curl
#include <3ds.h>       //libctru

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <malloc.h>

#define SOC_ALIGN 0x1000
#define SOC_BUFFERSIZE 0x100000
static u32 *SOC_buffer = NULL;

CURL *curl;
curl_mime *mime;
curl_mimepart *part1;
curl_mimepart *part2;

size_t writeFunction(void *ptr, size_t size, size_t nmemb, std::string* data) {
    data->append((char*) ptr, size * nmemb);
    return size * nmemb;
}

bool initSocket() {
    Result ret;
    SOC_buffer = (u32 *)memalign(SOC_ALIGN, SOC_BUFFERSIZE);
    if (SOC_buffer == NULL) {
        printf("memalign: failed to allocate\n");
        return false;
    }

    // Now intialise soc:u service
    if ((ret = socInit(SOC_buffer, SOC_BUFFERSIZE)) != 0) {
        printf("socInit: 0x%08X\n", (unsigned int)ret);
        return false;
    }
    return true;
}

bool initcurl() {
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    return !(!(curl));
}

void initform() {
    mime = curl_mime_init(curl);
}

void discordhandleentry(const char* handle) {
    part1 = curl_mime_addpart(mime);
    curl_mime_data(part1, handle, CURL_ZERO_TERMINATED);
    curl_mime_name(part1, "discordhandle");
}

void essentialdataentry() {
    u8* buf = new u8[0x2200];
    Handle file;
    FSUSER_OpenFileDirectly(&file, ARCHIVE_NAND_W_FS, fsMakePath(PATH_EMPTY, ""), fsMakePath(PATH_UTF16, u"/"), FS_OPEN_READ, 0);
    FSFILE_Read(file, NULL, 0x200, buf, 0x2200);
    FSFILE_Close(file);
    part2 = curl_mime_addpart(mime);
    curl_mime_data(part2, (char*)buf, 0x2200);
    curl_mime_filename(part2, "essential.exefs");
    curl_mime_name(part2, "file");
    delete[] buf;
}

void fileentry(const char* filepath) {
    part2 = curl_mime_addpart(mime);
    curl_mime_filedata(part2, filepath);
    curl_mime_name(part2, "file");
}

void serialentry(const char* name, char* serial) {
    curl_mimepart *serialpart = curl_mime_addpart(mime);
    curl_mime_data(serialpart, serial, CURL_ZERO_TERMINATED);
    curl_mime_name(serialpart, name);
}

CURLcode submittourl(const char* url, std::string* response_string) {
    curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 5000);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFunction);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, response_string);
    return curl_easy_perform(curl);
}

long gethttpcode() {
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    return http_code;
}

void exiteverything() {
    curl_easy_cleanup(curl);
    curl_mime_free(mime);
    curl_global_cleanup();
}