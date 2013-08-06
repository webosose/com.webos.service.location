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
 * Filename   : db_util.c
 * Purpose   : Utlity lib for storing/retriving key value in xml file
 * Platform   : RedRose
 * Author(s)  : rajesh gopu
 * Email ID.  : rajeshgopu.iv@lge.com
 * Creation Date : 06-03-2013
 *
 * Modifications :
 * Sl No  Modified by  Date  Version Description
 *
 *
 ********************************************************************/
#include <db_util.h>
#include <libxml/parser.h>
#include <libxml/xmlmemory.h>
#include <glib.h>
int get(DBHandle* handle, char* keyVal, xmlChar** result)
{
    if (!keyVal || !handle) {
        return NULL_VALUE;
    }
    xmlDocPtr docPtr = xmlParseFile(handle->fileName);
    xmlNodePtr cur = xmlDocGetRootElement(docPtr);
    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
        if ((!xmlStrcmp(cur->name, (const xmlChar *) keyVal))) {
            *result = xmlNodeListGetString(docPtr, cur->xmlChildrenNode, 1);

            break;
        }
        cur = cur->next;
    }
    if (result == NULL) return KEY_NOT_FOUND;
    xmlFreeDoc(docPtr);
    xmlCleanupParser();
    return SUCCESS;
}

int put(DBHandle* handle, char* key, char* value)
{
    //TODO: allow duplicate keys?
    if (!key) {
        return NULL_VALUE;
    }
    if (!handle || !handle->doc) {
        return INIT_ERROR;
    }
    xmlNodePtr root_node = xmlDocGetRootElement(handle->doc);
    xmlNewChild(root_node, NULL, BAD_CAST key, BAD_CAST value);
    return SUCCESS;
}
int deleteKey(DBHandle* handle, char* keyVal)
{
    if (!keyVal || !handle) {
        return NULL_VALUE;
    }
    xmlDocPtr docPtr = xmlParseFile(handle->fileName);
    xmlNodePtr cur = xmlDocGetRootElement(docPtr);
    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
        if ((!xmlStrcmp(cur->name, (const xmlChar *) keyVal))) {
            xmlUnlinkNode(cur);
            //TODO:duplicate keys , get in list delete after while
        }
        cur = cur->next;
    }

    handle->doc = docPtr;
    commit(handle);
    return SUCCESS;

}
int commit(DBHandle* handle)
{
    if (!handle || !handle->fileName || !handle->doc) {
        return INIT_ERROR;
    }
    xmlSaveFormatFileEnc(handle->fileName, handle->doc, "UTF-8", 1);
    xmlFreeDoc(handle->doc);
    xmlCleanupParser();

    return SUCCESS;
}
int isFileExists(const char *fname)
{
    FILE *file;
    if (file = fopen(fname, "r")) {
        fclose(file);
        return 1;
    }
    return 0;
}
int createPreference(char* filename, DBHandle* handle, char* title,
                     int enablecheck)
{
    if (!handle || !title || !filename) {

        return NULL_VALUE;
    }
    xmlDocPtr doc_ptr;
    xmlNodePtr root_node = NULL;

    printf("crating xml\n");
    doc_ptr = xmlNewDoc(BAD_CAST "1.0");
    root_node = xmlNewNode(NULL, BAD_CAST title);
    xmlDocSetRootElement(doc_ptr, root_node);

    handle->doc = doc_ptr;
    handle->fileName = filename;
    return SUCCESS;
}

int deletePreference(char* filename)
{
    if (!filename) {
        return NULL_VALUE;
    }
    return remove(filename);
}

