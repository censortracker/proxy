cmake_minimum_required(VERSION 3.25)

project(desktopproxy VERSION 0.1.0.5 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS 
    Core 
    Network 
    HttpServer 
    Widgets
)

# Add subprojects
add_subdirectory(proxyserver)
add_subdirectory(client_moc)