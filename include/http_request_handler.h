/********************************************************************
 * (c) Copyright 2012. All Rights Reserved
 * LG Electronics.
 *
 * Project Name : Location framework
 * Group   : PD-4
 * Security  : Confidential
 ********************************************************************/

/********************************************************************
 * @File
 * Filename   :http_request_handler.c
 * Purpose   : Utlity lib for http requests
 * Platform   : RedRose
 * Author(s)  : sandeep kumar h
 * Email ID.  : sandeep.kumar@lge.com
 * Creation Date : 18-09-2013
 *
 * Modifications :
 * Sl No  Modified by  Date  Version Description
 *
 *
 ********************************************************************/
#define BUFFER_SIZE 1024
#define CONNECTION_TIMEOUT 60*1000
#define HTTP_REQUEST 1
#define NO_CERTIFICATE_VERIFY 0L
#define CERTIFICATE_VERIFY 1L
#define NO_SSL_VERIFYHOST 1L
#define SSL_VERIFYHOST 0L
struct MemoryStruct {
    char *memory;
    size_t size;
};
struct MemoryStruct chunk;
static size_t WriteMemoryCallback(void *, size_t , size_t , void *);
void setupConnection(char *proxyServer , long proxyPort, char *userid, char *passwd);
struct MemoryStruct CurlConnection_getData(char *URL , char *accept, char *contentType , char *xwapprofile,
        char *charset);
struct MemoryStruct  CurlConnection_postData(char *URL , char *postdata , char *accept, char *contentType ,
        char *xwapprofile, char *charset);