/**
 @file      LunaLocationService_Log.h
 @brief     Logging mechanism using PmLog

 @author    Sunit
 @date      2013-01-10
 @version   TDB
 @todo      TDB

 @copyright Copyright (c) 2012-2013 LG Electronics Inc. All Rights Reserved
 */

#ifndef _LOCATION_SERVICE_LOG_H_
#define _LOCATION_SERVICE_LOG_H_

#include <glib.h>
#include <PmLogLib.h>

extern PmLogContext gLsLogContext;

#define LS_LOG_HELPER(pmLogLevel__, ...) \
    PmLogPrint(gLsLogContext, (pmLogLevel__), __VA_ARGS__)

#define LS_LOG_DEBUG(...) \
    LS_LOG_HELPER(kPmLogLevel_Debug, __VA_ARGS__)

#define LS_LOG_INFO(...) \
    LS_LOG_HELPER(kPmLogLevel_Info, __VA_ARGS__)

#define LS_LOG_NOTICE(...) \
    LS_LOG_HELPER(kPmLogLevel_Notice, __VA_ARGS__)

#define LS_LOG_WARNING(...) \
    LS_LOG_HELPER(kPmLogLevel_Warning, __VA_ARGS__)

#define LS_LOG_ERROR(...) \
    LS_LOG_HELPER(kPmLogLevel_Error, __VA_ARGS__)

#define LS_LOG_CRITICAL(...) \
    LS_LOG_HELPER(kPmLogLevel_Critical, __VA_ARGS__)

#define LS_LOG_FATAL(...) \
    { LS_LOG_HELPER(kPmLogLevel_Critical, __VA_ARGS__); abort(); }

__BEGIN_DECLS

static int GlogLevelToPmLogLevel(int glib_level)
{
    switch (glib_level & G_LOG_LEVEL_MASK) {
        case G_LOG_LEVEL_ERROR:
            return kPmLogLevel_Alert;

        case G_LOG_LEVEL_CRITICAL:
            return kPmLogLevel_Critical;

        case G_LOG_LEVEL_WARNING:
            return kPmLogLevel_Warning;

        case G_LOG_LEVEL_MESSAGE:
            return kPmLogLevel_Notice;

        case G_LOG_LEVEL_INFO:
            return kPmLogLevel_Info;

        case G_LOG_LEVEL_DEBUG:
            return kPmLogLevel_Debug;
    }

    return kPmLogLevel_None;
}

static void lsloghandler(const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer user_data)
{
    PmLogPrint(gLsLogContext, GlogLevelToPmLogLevel(log_level), "%s", message);
}

__END_DECLS

/*
 __BEGIN_DECLS
 #define FEATURE_DLOG_DEBUG
 #ifdef FEATURE_DLOG_DEBUG

 #include <dlog.h>

 #ifndef LUNA_LOCATION_SERVICE_LOG_TAG
 #define LUNA_LOCATION_SERVICE_LOG_TAG "LUNA_LOCATION_SERVICE"
 #endif

 #define info(fmt,args...)  { SLOG(LOG_INFO, LUNA_LOCATION_SERVICE_LOG_TAG, fmt "\n", ##args); }
 #define msg(fmt,args...)  { SLOG(LOG_DEBUG, LUNA_LOCATION_SERVICE_LOG_TAG, fmt "\n", ##args); }
 #define LS_LOG_DEBUG(fmt,args...)  { SLOG(LOG_DEBUG, LUNA_LOCATION_SERVICE_LOG_TAG, "<%s:%d> " fmt "\n", __func__, __LINE__, ##args); }
 #define warn(fmt,args...)  { SLOG(LOG_WARN, LUNA_LOCATION_SERVICE_LOG_TAG, "<%s:%d> " fmt "\n", __func__, __LINE__, ##args); }
 #define err(fmt,args...)  { SLOG(LOG_FATAL, LUNA_LOCATION_SERVICE_LOG_TAG, "<%s:%d> " fmt "\n", __func__, __LINE__, ##args); }

 static void dbgloghandler (const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer user_data) {
 LS_LOG_DEBUG("[DEBUG] GPS Service log %s \n", message);
 }
 #else

 #define ANSI_COLOR_NORMAL "\e[0m"

 #define ANSI_COLOR_BLACK "\e[0;30m"
 #define ANSI_COLOR_RED "\e[0;31m"
 #define ANSI_COLOR_GREEN "\e[0;32m"
 #define ANSI_COLOR_BROWN "\e[0;33m"
 #define ANSI_COLOR_BLUE "\e[0;34m"
 #define ANSI_COLOR_MAGENTA "\e[0;35m"
 #define ANSI_COLOR_CYAN "\e[0;36m"
 #define ANSI_COLOR_LIGHTGRAY "\e[0;37m"

 #define ANSI_COLOR_DARKGRAY "\e[1;30m"
 #define ANSI_COLOR_LIGHTRED "\e[1;31m"
 #define ANSI_COLOR_LIGHTGREEN "\e[1;32m"
 #define ANSI_COLOR_YELLOW "\e[1;33m"
 #define ANSI_COLOR_LIGHTBLUE "\e[1;34m"
 #define ANSI_COLOR_LIGHTMAGENTA "\e[1;35m"
 #define ANSI_COLOR_LIGHTCYAN "\e[1;36m"
 #define ANSI_COLOR_WHITE "\e[1;37m"

 #ifndef LUNA_TELEPHONY_SERVICE_LOG_FILE
 #define LUNA_TELEPHONY_SERVICE_LOG_FILE stdout
 #endif

 #ifndef LUNA_TELEPHONY_SERVICE_LOG_FUNC
 #define LUNA_TELEPHONY_SERVICE_LOG_FUNC fprintf
 #endif

 #define info(fmt,args...)  LUNA_TELEPHONY_SERVICE_LOG_FUNC(LUNA_TELEPHONY_SERVICE_LOG_FILE, fmt "\n", ##args); fflush(LUNA_TELEPHONY_SERVICE_LOG_FILE);
 #define msg(fmt,args...)  LUNA_TELEPHONY_SERVICE_LOG_FUNC(LUNA_TELEPHONY_SERVICE_LOG_FILE, fmt "\n", ##args); fflush(LUNA_TELEPHONY_SERVICE_LOG_FILE);
 #define LS_LOG_DEBUG(fmt,args...)  LUNA_TELEPHONY_SERVICE_LOG_FUNC(LUNA_TELEPHONY_SERVICE_LOG_FILE, ANSI_COLOR_LIGHTGRAY "<%s:%s> " ANSI_COLOR_NORMAL fmt "\n", __FILE__, __FUNCTION__, ##args); fflush(LUNA_TELEPHONY_SERVICE_LOG_FILE);
 #define warn(fmt,args...) LUNA_TELEPHONY_SERVICE_LOG_FUNC(LUNA_TELEPHONY_SERVICE_LOG_FILE, ANSI_COLOR_YELLOW "<%s:%s> " ANSI_COLOR_NORMAL fmt "\n", __FILE__, __FUNCTION__, ##args); fflush(LUNA_TELEPHONY_SERVICE_LOG_FILE);
 #define err(fmt,args...)  LUNA_TELEPHONY_SERVICE_LOG_FUNC(LUNA_TELEPHONY_SERVICE_LOG_FILE, ANSI_COLOR_LIGHTRED "<%s:%s> " ANSI_COLOR_NORMAL fmt "\n", __FILE__, __FUNCTION__, ##args); fflush(LUNA_TELEPHONY_SERVICE_LOG_FILE);

 #endif

 __END_DECLS
 */
#endif // _LOCATION_SERVICE_LOG_H_
