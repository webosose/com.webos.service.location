/********************************************************************
 * Copyright (c) 2012-2013 LG Electronics Inc. All Rights Reserved
 *
 * Project Name : Location framework
 * Group        : PD-4
 * Security     : Confidential
 ********************************************************************/

/********************************************************************
 * @File
 * Filename     : db_util.h
 * Purpose      : Utlity lib for storing/retriving key value in xml file
 * Platform     : RedRose
 * Author(s)    : rajesh gopu
 * Email ID.    : rajeshgopu.iv@lge.com
 * Creation Date: 06-03-2013
 *
 * Modifications:
 * Sl No        Modified by     Date        Version Description
 *
 *
 ********************************************************************/

#ifndef _DB_UTIL_H_
#define _DB_UTIL_H_

#include <libxml/parser.h>
#include <libxml/xmlmemory.h>
#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>
#include <glib-object.h>



#define PREF_PATH   "."             //Path of all prefence to store in system
#define MY_ENCODING "ISO-8859-1"    //Not used now
typedef struct _DBHandle DBHandle;
/*
 * DB handle has neccasry info to process request
 */
struct _DBHandle {
    xmlDocPtr doc;
  const char *fileName;
};

/*
 * Error codes
 */
typedef enum {
    SUCCESS = 0,
    IO_ERROR,
    NULL_VALUE,
    KEY_NOT_FOUND,
    PREFERENCE_NOT_FOUND,
    INVALID_KEY,
    INIT_ERROR,
    FILE_EXIST_ERROR,
    UNKNOWN_ERROR,
} DbErrorCodes;

/**
 * <Funciton>       createPreference
 * <Description>    create a new preference xml file
 * @param           <filename> <In> <name of the file to create>
 * @param           <DBHandle> <In> <Intialized DBHandle it will be populated later>
 * @param           <title> <In> <the Root elemnet in XML file>
 * @return          int
 */
int createPreference(const char *filename, DBHandle *handle, const char *title, int enablecheck);

/**
 * <Funciton>       put
 * <Description>    Get the position from GPS
 * @param           <DBHandle> <In> <DBHandled intialized in create>
 * @param           <key> <In> <key to the value will be mapped>
 * @param           <value> <In> <value will be mapped with the given key>
 * @return          int
 */
int put(DBHandle *handle, const char *key, char *value);

/**
 * <Funciton>       get
 * <Description>    Get teh Value of the given key
 * @param           <DBHandle> <In> <DBHandled intialized in create>
 * @param           <key> <In> <key to get the value>
 * @param           <result> <In> <result will be copied here>
 * @return          int
 */
int get(DBHandle *handle, const char *key, xmlChar **result);

/**
 * <Funciton>       deleteKey
 * <Description>    Delete the given key from xml
 * @param           <DBHandle> <In> <DBHandled intialized in create>
 * @param           <key> <In> <key to delete>
 * @return          int
 */
int deleteKey(DBHandle *handle, char *key);

/**
 * <Funciton>       commit
 * <Description>    Flush the modified xml to the file
 * @param           <DBHandle> <In> <DBHandled intialized in create>
 * @return          int
 */
int commit(DBHandle *handle);

/**
 * <Funciton>       isFileExists
 * <Description>    Check if the file exists
 * @param           <fname> <In> <Name of the file to check>
 * @return          int
 */
int isFileExists(const char *fname);

/**
 * <Funciton>       deletePreference
 * <Description>    Delete the gicen preference xml file
 * @param           <filename> <In> <file name to delete>
 * @return          int
 */
int deletePreference(char *filename);



#endif /* _DB_UTIL_H_ */
