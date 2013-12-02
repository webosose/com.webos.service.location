# @@@LICENSE
#
#   Copyright (c) 2012-2013 LG Electronics Inc. All Rights Reserved
#
# LICENSE@@@

set(DB_UTILS_NAME dbutils)
set(DB_UTILS_LIB_PATH ${CMAKE_INSTALL_PREFIX}/lib)
set(DB_UTILS_DATA_PATH ${CMAKE_INSTALL_PREFIX}/share/location)
set(DB_UTILS_SRC db_util.c Gps_stored_data.c)
set(LIBRARIES ${GLIB2_LDFLAGS} ${LIB2XML_LDFLAGS})

add_library(${DB_UTILS_NAME} SHARED ${DB_UTILS_SRC})
target_link_libraries(${DB_UTILS_NAME} ${LIBRARIES})
set_target_properties(${DB_UTILS_NAME} PROPERTIES VERSION 1.0.0 SOVERSION 1)
install(TARGETS ${DB_UTILS_NAME} DESTINATION ${DB_UTILS_LIB_PATH})
install(DIRECTORY DESTINATION ${DB_UTILS_DATA_PATH})
