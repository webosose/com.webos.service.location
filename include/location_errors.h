// @@@LICENSE
//
//      Copyright (c) 2015 LG Electronics, Inc.
//
// Confidential computer software. Valid license from LG required for
// possession, use or copying. Consistent with FAR 12.211 and 12.212,
// Commercial Computer Software, Computer Software Documentation, and
// Technical Data for Commercial Items are licensed to the U.S. Government
// under vendor's standard commercial license.
//
// LICENSE@@@


#ifndef LOCATION_ERRORS_H_
#define LOCATION_ERRORS_H_

/*
 * Common Error codes of Location Framework
 */
//get rid of  LocationError_t later on
typedef uint32_t LocationError_t;
typedef enum {
    ERROR_NONE = 0,
    ERROR_NOT_IMPLEMENTED,
    ERROR_TIMEOUT,
    ERROR_NETWORK_ERROR,
    ERROR_NOT_APPLICABLE_TO_THIS_HANDLER,
    ERROR_NOT_AVAILABLE,
    ERROR_WRONG_PARAMETER,
    ERROR_DUPLICATE_REQUEST,
    ERROR_REQUEST_INPROGRESS,
    ERROR_NOT_STARTED,
    ERROR_LICENSE_KEY_INVALID,
    ERROR_MUTLITHREAD
} ErrorCodes;

enum {
    ERROR_NETWORK_NOT_FOUND = 0,
    ERROR_NO_MEMORY,
    ERROR_NO_RESPONSE,
    ERROR_NO_DATA
};

    typedef enum {
        REQUEST_NONE = 0,
        REQUEST_PROGRESS,
        REQUEST_COMPLETED
    } request_state_cellid_type;

#endif /* LOCATION_ERRORS_H_ */
