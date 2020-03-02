// Copyright (c) 2020 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0


#include <db_util.h>
#include <unistd.h>

int get(DBHandle *handle, const char *keyVal, xmlChar **result) {
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

  if (result == NULL)
    return KEY_NOT_FOUND;

  xmlFreeDoc(docPtr);
  xmlCleanupParser();
  return SUCCESS;
}

int put(DBHandle *handle, const char *key, char *value) {
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

int deleteKey(DBHandle *handle, char *keyVal) {
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

int commit(DBHandle *handle) {
  if (!handle || !handle->fileName || !handle->doc) {
    return INIT_ERROR;
  }

  xmlSaveFormatFileEnc(handle->fileName, handle->doc, "UTF-8", 1);
  xmlFreeDoc(handle->doc);
  xmlCleanupParser();
  return SUCCESS;
}

int isFileExists(const char *fname) {
     return (access(fname, F_OK) == 0);
}

int createPreference(const char *filename, DBHandle *handle, const char *title, int enablecheck) {
  if (!handle || !title || !filename) {
    return NULL_VALUE;
  }

  xmlDocPtr doc_ptr;
  xmlNodePtr root_node = NULL;
  doc_ptr = xmlNewDoc(BAD_CAST "1.0");
  root_node = xmlNewNode(NULL, BAD_CAST title);
  xmlDocSetRootElement(doc_ptr, root_node);
  handle->doc = doc_ptr;
  handle->fileName = filename;
  return SUCCESS;
}

int deletePreference(char *filename) {
  if (!filename) {
    return NULL_VALUE;
  }

  return remove(filename);
}

