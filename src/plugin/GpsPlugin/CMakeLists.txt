# @@@LICENSE
#
#   Copyright (c) 2012-2013 LG Electronics Inc. All Rights Reserved
#
# LICENSE@@@

set(GPS_PLUGIN_NAME gps)
set(GPS_PLUGIN_PATH ${CMAKE_INSTALL_PREFIX}/lib/location/plugins)

set(GPS_PLUGIN_SRC Gps_Plugin.c)

set(LIBRARIES ${GLIB2_LDFLAGS} ${GEOCLUE_LDFLAGS} ${PMLOGLIB_LDFLAGS})

add_library(${GPS_PLUGIN_NAME} SHARED ${GPS_PLUGIN_SRC})
target_link_libraries(${GPS_PLUGIN_NAME} ${LIBRARIES} dbutils)
#set_target_properties(${GPS_PLUGIN_NAME} PROPERTIES VERSION 1.0.0 SOVERSION 1)
install(TARGETS ${GPS_PLUGIN_NAME} DESTINATION ${GPS_PLUGIN_PATH})
