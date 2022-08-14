cmake_minimum_required(VERSION 3.18.0)
project(Example VERSION 0.1.0)

if( TARGET_PLATFORM STREQUAL "GTK4" )
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK4 REQUIRED gtk4)
endif()


include_directories(
    /usr/include/freetype2
    /usr/include/libdrm
    ./EdgeUI/include
    ./source
    ${GTK4_INCLUDE_DIRS}
)

add_subdirectory(./EdgeUI)

add_executable(${PROJECT_NAME}
    main.cpp
)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17)

if( TARGET_PLATFORM STREQUAL "GTK4" )
    target_link_libraries(${PROJECT_NAME} EdgeUI.GTK4 stdc++ pthread m freetype GLESv2 EGL z i2c mosquitto curl ${GTK4_LINK_LIBRARIES})
elseif( TARGET_PLATFORM STREQUAL "DRM" )
    target_link_libraries(${PROJECT_NAME} EdgeUI.DRM stdc++ pthread m freetype GLESv2 EGL z i2c mosquitto curl gbm drm)
elseif( TARGET_PLATFORM STREQUAL "X11" )
    target_link_libraries(${PROJECT_NAME} EdgeUI.X11 stdc++ pthread m freetype GLESv2 EGL z i2c mosquitto curl X11 GL)
else()
    message(FATAL_ERROR "TARGET_PLATFORM not defined")
endif()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

