#include <3ds.h>
#include <curl/curl.h>

bool initSocket();
void initcurl();
void initform();
void discordhandleentry(const char* handle);
void essentialdataentry();
void fileentry(const char* filepath);
void serialentry(const char* name, char* serial);
CURLcode submittourl(const char* url, std::string* response_string);
void exiteverything();
long gethttpcode();