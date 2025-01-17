project(proxyclient)

add_executable(${PROJECT_NAME}
    client_main.cpp
    proxyclient.cpp
    proxyclient.h
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Network
) 