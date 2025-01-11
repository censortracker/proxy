cmake_minimum_required(VERSION 3.25)

set(PROJECT_NAME desktopproxy)
set(PROJECT_VERSION 0.1.0)

project(${PROJECT_NAME} VERSION ${PROJECT_VERSION} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Network HttpServer)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/proxyserver.h
    src/proxyserver.cpp
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
    ${CMAKE_BINARY_DIR}/xray-prebuilt/${XRAY_PLATFORM_DIR}
    COMMENT "Copying xray binaries to build directory"
)