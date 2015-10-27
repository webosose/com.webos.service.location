# @@@LICENSE
#
#   Copyright (c) 2012-2013 LG Electronics Inc. All Rights Reserved
#
# LICENSE@@@

cmake_minimum_required(VERSION 2.8.7)
project(location)

include(webOS/webOS)
webos_modules_init(1 0 0 QUALIFIER RC4)
#webos_component(1 0 0)

include(FindPkgConfig)

pkg_check_modules(GLIB2 REQUIRED glib-2.0)
add_definitions(${GLIB2_CFLAGS})

pkg_check_modules(GOBJ REQUIRED gobject-2.0)
add_definitions(${GOBJ_CFLAGS})

pkg_check_modules(GMOD REQUIRED gmodule-2.0)
add_definitions(${GMOD_CFLAGS})

pkg_check_modules(GEOCLUE REQUIRED geoclue)
add_definitions(${GEOCLUE_CFLAGS})

pkg_check_modules(LUNASERVICE REQUIRED luna-service2)
add_definitions(${LUNASERVICE_CFLAGS})

pkg_check_modules(LUNAPREFS REQUIRED luna-prefs)
add_definitions(${LUNAPREFS_CFLAGS})

pkg_check_modules(LIBPBNJSON REQUIRED pbnjson_c)
add_definitions(${LIBPBNJSON_CFLAGS})

pkg_check_modules(LIB2XML REQUIRED libxml-2.0)
add_definitions(${LIB2XML_CFLAGS})

pkg_check_modules(PMLOGLIB REQUIRED PmLogLib)
add_definitions(${PMLOGLIB_CFLAGS})

pkg_check_modules(LOCUTILS REQUIRED loc-utils)
add_definitions(${LOCUTILS_CFLAGS})

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

include_directories("${PROJECT_SOURCE_DIR}/include")

add_subdirectory(src/plugin/GpsPlugin)
add_subdirectory(src/plugin/NetworkPlugin)

set(DB_UTILS_NAME dbutils)
set(DB_UTILS_LIB_PATH ${CMAKE_INSTALL_PREFIX}/lib)
set(DB_UTILS_DATA_PATH ${WEBOS_INSTALL_LOCALSTATEDIR}/location)
set(DB_UTILS_SRC ${PROJECT_SOURCE_DIR}/src/utils/db_util.c ${PROJECT_SOURCE_DIR}/src/utils/Gps_stored_data.c)
set(LIBRARIES ${GLIB2_LDFLAGS} ${LIB2XML_LDFLAGS})

add_library(${DB_UTILS_NAME} SHARED ${DB_UTILS_SRC})
target_link_libraries(${DB_UTILS_NAME} ${LIBRARIES})
set_target_properties(${DB_UTILS_NAME} PROPERTIES VERSION 1.0.0 SOVERSION 1)
install(TARGETS ${DB_UTILS_NAME} DESTINATION ${DB_UTILS_LIB_PATH})
install(DIRECTORY DESTINATION ${DB_UTILS_DATA_PATH})


set(LOCATION_SERVICE_NAME location)

file(GLOB UTIL_SOURCES  ${PROJECT_SOURCE_DIR}/src/utils/*.cpp  ${PROJECT_SOURCE_DIR}/src/utils/*.c)
file(GLOB GOOGLE_WSP_SOURCES ${PROJECT_SOURCE_DIR}/src/handler/lbs/geoservices/google/*.cpp)
file(GLOB HANDLER_SOURCE ${PROJECT_SOURCE_DIR}/src/handler/*.c ${PROJECT_SOURCE_DIR}/src/handler/lbs/*.cpp)
file(GLOB LUNA_IPC  ${PROJECT_SOURCE_DIR}/src/lunaIpc/*.cpp  ${PROJECT_SOURCE_DIR}/src/lunaIpc/*.c)

set(SRC
        ${PROJECT_SOURCE_DIR}/src/Main.cpp
        ${LUNA_IPC}
        ${SERVICE_UTIL_SOURCES}
        ${GOOGLE_WSP_SOURCES}
        ${HANDLER_SOURCE}
        ${UTIL_SOURCES}
)
set(LIBRARIES
        ${GLIB2_LDFLAGS}
        ${GOBJ_LDFLAGS}
        ${GMOD_LDFLAGS}
        ${LUNASERVICE_LDFLAGS}
        ${LUNAPREFS_LDFLAGS}
        ${LIBPBNJSON_LDFLAGS}
        ${PMLOGLIB_LDFLAGS}
        ${LOCUTILS_LDFLAGS}
        ${LIB2XML_LDFLAGS}
)

add_executable(${LOCATION_SERVICE_NAME} ${SRC})
target_link_libraries(${LOCATION_SERVICE_NAME} ${LIBRARIES})

webos_build_program(NAME ${LOCATION_SERVICE_NAME})
webos_build_system_bus_files()


install(DIRECTORY include/
        DESTINATION ${CMAKE_INSTALL_PREFIX}/include/location
        FILES_MATCHING PATTERN "*.h")
install(DIRECTORY files/etc/
        DESTINATION ${WEBOS_INSTALL_SYSCONFDIR}
        FILE_PERMISSIONS OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ
        PATTERN "*")
