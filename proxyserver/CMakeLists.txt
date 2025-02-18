project(CensorTrackerProxy)

add_subdirectory(xray-serialization)

qt_add_resources(RESOURCES resources.qrc)

if(WIN32)
    set(APP_ICON_RESOURCE_WINDOWS "${CMAKE_CURRENT_SOURCE_DIR}/proxyserver.rc")
endif()

set(SOURCES
    src/main.cpp
    src/proxyserver.cpp
    src/proxyserver.h
    src/trayicon.cpp
    src/trayicon.h
    src/configmanager.cpp
    src/configmanager.h
    src/xraycontroller.cpp
    src/xraycontroller.h
    src/iproxyservice.h
    src/proxyservice.cpp
    src/proxyservice.h
    src/httpapi.cpp
    src/httpapi.h
    src/logger.cpp
    src/logger.h
    ${APP_ICON_RESOURCE_WINDOWS}
)

if(WIN32)
    add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${RESOURCES})
else()
    add_executable(${PROJECT_NAME} ${SOURCES} ${RESOURCES})
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Network
    Qt6::HttpServer
    Qt6::Widgets
    xray-serialization
)

if(UNIX AND NOT APPLE)
    set(XRAY_PLATFORM_DIR "linux")
elseif(WIN32)
    set(XRAY_PLATFORM_DIR "windows")
elseif(APPLE)
    set(XRAY_PLATFORM_DIR "macos")
endif()

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/xray-prebuilt/${XRAY_PLATFORM_DIR}
    $<TARGET_FILE_DIR:${PROJECT_NAME}>
    COMMENT "Copying xray binaries next to executable"
)

# Копируем библиотеку xray-serialization
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:xray-serialization>
    $<TARGET_FILE_DIR:${PROJECT_NAME}>
    COMMENT "Copying xray-serialization library next to executable"
) 