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
 * Filename   : http_request_handler.h
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
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <http_request_handler.h>
#include <LocationService_Log.h>
#include <curl/curl.h>
CURL *curl_handle = NULL;


void setupConnection(char *proxyServer , long proxyPort, char *userid, char *passwd)
{
    char buf [BUFFER_SIZE];
    curl_handle = curl_easy_init();

    if (proxyServer != NULL)
        curl_easy_setopt(curl_handle, CURLOPT_PROXY, proxyServer);

    if (proxyPort != 0)
        curl_easy_setopt(curl_handle, CURLOPT_PROXYPORT, proxyPort);

    if ((userid != NULL)  && (passwd != NULL)) {
        g_snprintf(buf, BUFFER_SIZE, "%s:%s", userid, passwd);
        curl_easy_setopt(curl_handle, CURLOPT_PROXYUSERPWD, buf);
    }

    curl_easy_setopt(curl_handle, CURLOPT_CONNECTTIMEOUT, CONNECTION_TIMEOUT);
    curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, CONNECTION_TIMEOUT);
}

struct MemoryStruct CurlConnection_getData(char *URL , char *accept, char *contentType , char *xwapprofile,
        char *charset)
{
    struct curl_slist *m_httpHeadersList = NULL; // init to NULL is important

    if (curl_handle) {
        chunk.memory = malloc(1);  /* will be grown as needed by the realloc above */
        chunk.size = 0;    /* no data at this point */

        if (accept != NULL)
            m_httpHeadersList = curl_slist_append(m_httpHeadersList, accept);

        if (contentType != NULL)
            m_httpHeadersList = curl_slist_append(m_httpHeadersList, contentType);

        if (xwapprofile != NULL)
            m_httpHeadersList = curl_slist_append(m_httpHeadersList, xwapprofile);

        if (charset != NULL)
            m_httpHeadersList = curl_slist_append(m_httpHeadersList, charset);

        if(URL != NULL)
        curl_easy_setopt(curl_handle, CURLOPT_URL, URL); //XTRA_BIN
        curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
        curl_easy_setopt(curl_handle, CURLOPT_HTTPGET, HTTP_REQUEST);
        /* Perform the request, res will get the return code */
        curl_easy_perform(curl_handle);
        curl_slist_free_all(m_httpHeadersList);                         // headers also need to be freed
        curl_easy_cleanup(curl_handle);
        m_httpHeadersList = NULL;
    }

    return chunk;
}

struct MemoryStruct  CurlConnection_postData(char *URL , char *postdata , char *accept, char *contentType ,
        char *xwapprofile, char *charset)
{
    struct curl_slist *m_httpHeadersList = NULL; // init to NULL is important

    if (curl_handle) {
        chunk.memory = malloc(1);  /* will be grown as needed by the realloc above */
        chunk.size = 0;    /* no data at this point */

        if (accept != NULL)
            m_httpHeadersList = curl_slist_append(m_httpHeadersList, accept);

        if (contentType != NULL)
            m_httpHeadersList = curl_slist_append(m_httpHeadersList, contentType);

        if (xwapprofile != NULL)
            m_httpHeadersList = curl_slist_append(m_httpHeadersList, xwapprofile);

        if (charset != NULL)
            m_httpHeadersList = curl_slist_append(m_httpHeadersList, charset);

        if(URL != NULL)
        curl_easy_setopt(curl_handle, CURLOPT_URL, URL);
        curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, NO_CERTIFICATE_VERIFY);
        curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, NO_SSL_VERIFYHOST);
        curl_easy_setopt(curl_handle, CURLOPT_POST, HTTP_REQUEST);
        curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE, (long)strlen(postdata));
        if(postdata != NULL)
        curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, postdata);
        curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, m_httpHeadersList);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
        curl_easy_perform(curl_handle);
        curl_slist_free_all(m_httpHeadersList);                         // headers also need to be freed
        curl_easy_cleanup(curl_handle);
    }

    return chunk;
}

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;
    mem->memory = realloc(mem->memory, mem->size + realsize + 1);

    if (mem->memory == NULL) {
        /* out of memory! */
        LS_LOG_DEBUG("not enough memory (realloc returned NULL)\n");
        return 0;
    }

    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
    return realsize;
}
