cmake_minimum_required(VERSION 3.0.0)
project(EdgeUIExample VERSION 0.1.0)


add_definitions(-DPLATFORM_X11_GL -DVERBOSE_BUILD -DVERBOSE_SHADER_BUILD)

include_directories(
    /usr/include/freetype2
    /usr/include/libdrm
    ./EdgeUI/include
)

add_subdirectory(./EdgeUI)

add_executable(EdgeUIExample
    main.cpp
    TinyTools.cpp
    sgp30.cpp
    i2c_device.cpp
    MQTTData.cpp
)

set_property(TARGET EdgeUIExample PROPERTY CXX_STANDARD 17)
target_link_libraries(EdgeUIExample EdgeUI stdc++ pthread m freetype GL X11 z i2c mosquitto curl)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

