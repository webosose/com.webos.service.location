# @@@LICENSE
#
#   Copyright (c) 2012-2013 LG Electronics Inc. All Rights Reserved
#
# LICENSE@@@

set(LOCATION_SERVICE_NAME location)
set(SRC
        ${PROJECT_SOURCE_DIR}/src/Main.cpp
        ${PROJECT_SOURCE_DIR}/src/LocationManagerService.c
        ${PROJECT_SOURCE_DIR}/ServiceAgent/LocationService.cpp
        ${PROJECT_SOURCE_DIR}/ServiceAgent/ConnectionStateObserver.cpp
        ${PROJECT_SOURCE_DIR}/ServiceAgent/JsonUtility.c
        ${PROJECT_SOURCE_DIR}/ServiceAgent/LunaLocationService_Util.cpp
        Gps_Handler.c
        Wifi_Handler.c
        Handler_Interface.c
        Cell_Handler.c
        Plugin_Loader.c
        Position.c
        Velocity.c
        Accuracy.c
        Address.c
        Satellite.c
        Network_Handler.c
        Lbs_Handler.c
)
set(LIBRARIES
        ${GMOD_LDFLAGS}
        ${GOBJ_LDFLAGS}
        ${LUNASERVICE_LDFLAGS}
        ${CJSON_LDFLAGS}
        -lffi
)
add_executable(${LOCATION_SERVICE_NAME} ${SRC})
target_link_libraries(${LOCATION_SERVICE_NAME} ${LIBRARIES} geoclue dbus-1 dbus-glib-1 xml2 z glib-2.0 gio-2.0 gmodule-2.0 dbutils luna-prefs sqlite3 PmLogLib curl ssl crypto)

webos_build_daemon()
webos_build_system_bus_files()
