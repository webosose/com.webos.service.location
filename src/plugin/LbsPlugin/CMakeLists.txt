# @@@LICENSE
#
#   Copyright (c) 2012-2013 LG Electronics Inc. All Rights Reserved
#
# LICENSE@@@

set(LBS_PLUGIN_NAME lbs)
set(LBS_PLUGIN_PATH ${CMAKE_INSTALL_PREFIX}/lib/location/plugins)

set(LBS_PLUGIN_SRC Lbs_Plugin.c)

set(LIBRARIES ${GLIB2_LDFLAGS} ${GEOCLUE_LDFLAGS})

add_library(${LBS_PLUGIN_NAME} SHARED ${LBS_PLUGIN_SRC})
target_link_libraries(${LBS_PLUGIN_NAME} ${LIBRARIES} dbutils)
install (TARGETS ${LBS_PLUGIN_NAME} DESTINATION ${LBS_PLUGIN_PATH})
