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

include_directories("${PROJECT_SOURCE_DIR}/include")

add_subdirectory(src/plugin/GpsPlugin)
add_subdirectory(src/plugin/WifiPlugin)
add_subdirectory(src/plugin/CellPlugin)
if(DEFINED NOMINATIUM_LBS)
add_definitions(-DNOMINATIUM_LBS)
add_subdirectory(src/plugin/LbsPlugin)
else()
add_subdirectory(src/plugin/GoogleLbsPlugin)
endif()
add_subdirectory(src/handler)
add_subdirectory(src/db_utils)

install(DIRECTORY include/
        DESTINATION ${CMAKE_INSTALL_PREFIX}/include/location
        FILES_MATCHING PATTERN "*.h")
