project(proxyserver)

add_executable(${PROJECT_NAME}
    main.cpp
    proxyserver.h
    proxyserver.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Network
    Qt6::HttpServer
)

if(UNIX AND NOT APPLE)
    set(XRAY_PLATFORM_DIR "linux")
elseif(WIN32)
    set(XRAY_PLATFORM_DIR "windows/xray")
elseif(APPLE)
    set(XRAY_PLATFORM_DIR "macos")
endif()

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/xray-prebuilt/${XRAY_PLATFORM_DIR}
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/xray-prebuilt
    COMMENT "Copying xray binaries next to executable"
) 