# min cmake version
cmake_minimum_required(VERSION 2.8.10)

include(cmake/config.cmake)

# toolchain file
if(CMAKE_TOOLCHAIN_FILE)
    set(LIBRARY_OUTPUT_PATH_ROOT ${CMAKE_BINARY_DIR} CACHE PATH "root for library output, set this to change where android libs are compiled to")
    get_filename_component(CMAKE_TOOLCHAIN_FILE_NAME ${CMAKE_TOOLCHAIN_FILE} NAME)
    find_file(CMAKE_TOOLCHAIN_FILE ${CMAKE_TOOLCHAIN_FILE_NAME} PATHS ${CMAKE_SOURCE_DIR} NO_DEFAULT_PATH)

    message(STATUS "CMAKE_TOOLCHAIN_FILE = ${CMAKE_TOOLCHAIN_FILE}")
endif()

# set install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Installation Directory")
endif()

message(STATUS "CMAKE_INSTALL_PREFIX = ${CMAKE_INSTALL_PREFIX}")

# set build type
if(NOT DEFINED CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "debug" CACHE STRING "debug build type" FORCE)
endif()

if(CMAKE_BUILD_TYPE MATCHES "(Release|RELEASE|release)")
    add_definitions(-DRELEASE)
else()
    add_definitions(-DDEBUG)
endif()

message(STATUS "CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")

project(gauss3x3)

# for windows platform
if(WIN32)
    add_definitions(-D_SCL_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_DEPRECATE)
    add_definitions(-DNOMINMAX)
    add_definitions(-DWIN32)
else()
    add_definitions(-Wall -Wextra)
    add_definitions(-fPIC)

    # for android platform
    if(ANDROID)
        # definitions setting
        add_definitions(-DANDROID)
        add_definitions(-Ofast)
        add_definitions(-ffast-math)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti -fno-exceptions")

        # extra libs and use gold link for android
        link_libraries("log")
        link_libraries("-fuse-ld=gold")
    endif()
endif()

add_subdirectory(src)
