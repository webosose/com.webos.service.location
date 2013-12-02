# @@@LICENSE
#
#   Copyright (c) 2012-2013 LG Electronics Inc. All Rights Reserved
#
# LICENSE@@@

set(CELL_PLUGIN_NAME cell)
set(CELL_PLUGIN_PATH ${CMAKE_INSTALL_PREFIX}/lib/location/plugins)

set(CELL_PLUGIN_SRC Cell_Plugin.c)

set(LIBRARIES ${GLIB2_LDFLAGS} ${GEOCLUE_LDFLAGS} ${PMLOGLIB_LDFLAGS})

add_library(${CELL_PLUGIN_NAME} SHARED ${CELL_PLUGIN_SRC})
target_link_libraries(${CELL_PLUGIN_NAME} ${LIBRARIES} dbutils)
#set_target_properties(${CELL_PLUGIN_NAME} PROPERTIES VERSION 1.0.0 SOVERSION 1)
install(TARGETS ${CELL_PLUGIN_NAME} DESTINATION ${CELL_PLUGIN_PATH})
