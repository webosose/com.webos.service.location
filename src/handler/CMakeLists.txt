# @@@LICENSE
#
#   Copyright (c) 2012-2013 LG Electronics Inc. All Rights Reserved
#
# LICENSE@@@

set(LOCATION_SERVICE_NAME location)
set(SRC
        ${PROJECT_SOURCE_DIR}/src/Main.cpp
        ${PROJECT_SOURCE_DIR}/ServiceAgent/LocationService.cpp
        ${PROJECT_SOURCE_DIR}/ServiceAgent/ConnectionStateObserver.cpp
        ${PROJECT_SOURCE_DIR}/ServiceAgent/JsonUtility.c
        ${PROJECT_SOURCE_DIR}/ServiceAgent/LunaLocationServiceUtil.cpp
        ${PROJECT_SOURCE_DIR}/ServiceAgent/LunaCriteriaCategoryHandler.cpp
        Gps_Handler.c
        Wifi_Handler.c
        Handler_Interface.c
        Cell_Handler.c
        Plugin_Loader.c
        Position.c
        Accuracy.c
        Address.c
        Satellite.c
        Network_Handler.c
        Lbs_Handler.c
)
set(LIBRARIES
        ${GLIB2_LDFLAGS}
        ${GOBJ_LDFLAGS}
        ${GMOD_LDFLAGS}
        ${LUNASERVICE_LDFLAGS}
        ${LUNAPREFS_LDFLAGS}
        ${LIBPBNJSON_LDFLAGS}
        ${PMLOGLIB_LDFLAGS}
)

add_executable(${LOCATION_SERVICE_NAME} ${SRC})
target_link_libraries(${LOCATION_SERVICE_NAME} ${LIBRARIES} dbutils)

webos_build_daemon()
webos_build_system_bus_files()
