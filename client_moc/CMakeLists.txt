project(proxyclient)

add_executable(${PROJECT_NAME}
    src/client_main.cpp
    src/proxyclient.cpp
    src/proxyclient.h
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Network
) 