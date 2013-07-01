# @@@LICENSE
#
#   Copyright (c) 2012-2013 LG Electronics Inc. All Rights Reserved
#
# LICENSE@@@

set(WIFI_PLUGIN_NAME wifi)
set(WIFI_PLUGIN_PATH ${CMAKE_INSTALL_PREFIX}/lib/location/plugins)

set(WIFI_PLUGIN_SRC Wifi_Plugin.c)

set(LIBRARIES ${GLIB2_LDFLAGS} ${GEOCLUE_LDFLAGS})

add_library(${WIFI_PLUGIN_NAME} SHARED ${WIFI_PLUGIN_SRC})
target_link_libraries(${WIFI_PLUGIN_NAME} ${LIBRARIES} dbutils)
#set_target_properties(${WIFI_PLUGIN_NAME} PROPERTIES VERSION 1.0.0 SOVERSION 1)
install(TARGETS ${WIFI_PLUGIN_NAME} DESTINATION ${WIFI_PLUGIN_PATH})
