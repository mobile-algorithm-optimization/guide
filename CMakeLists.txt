cmake_minimum_required(VERSION 3.0.0)

################################### NDK Toolchain ########################################
if (ANDROID)
    # set command lines for Android and neon
    set(NDK_PATH $ENV{NDK_PATH})
    set(CMAKE_TOOLCHAIN_FILE ${NDK_PATH}/build/cmake/android.toolchain.cmake)
    set(ANDROID_ABI "arm64-v8a")
    set(ANDROID_ARM_NEON ON)
    set(ANDROID_PLATFORM android-23)

    message(STATUS "NDK_PATH = ${NDK_PATH}")
    message(STATUS "CMAKE_TOOLCHAIN_FILE = ${CMAKE_TOOLCHAIN_FILE}")
    message(STATUS "ANDROID_ABI = ${ANDROID_ABI}")
    message(STATUS "ANDROID_ARM_NEON = ${ANDROID_ARM_NEON}")
    message(STATUS "ANDROID_PLATFORM = ${ANDROID_PLATFORM}")
endif()

# toolchain file
if (CMAKE_TOOLCHAIN_FILE)
    set(LIBRARY_OUTPUT_PATH_ROOT ${CMAKE_BINARY_DIR} CACHE PATH "root for library output, set this to change where android libs are compiled to")
    message("LIBRARY_OUTPUT_PATH_ROOT = ${LIBRARY_OUTPUT_PATH_ROOT}")
    get_filename_component(CMAKE_TOOLCHAIN_FILE_NAME ${CMAKE_TOOLCHAIN_FILE} NAME)
    find_file(CMAKE_TOOLCHAIN_FILE ${CMAKE_TOOLCHAIN_FILE_NAME} PATHS ${CMAKE_SOURCE_DIR} NO_DEFAULT_PATH)
    message(STATUS "CMAKE_TOOLCHAIN_FILE = ${CMAKE_TOOLCHAIN_FILE}")
endif()


################################### Build Type Default = debug ########################################

# set build type
if(NOT DEFINED CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "debug" CACHE STRING "debug build type" FORCE)
endif()

if(CMAKE_BUILD_TYPE MATCHES "(Release|RELEASE|release)")
    add_definitions(-DRELEASE)
else()
    add_definitions(-DDEBUG)
endif()

# C/CXX FLAGS

add_definitions(-Wall -Wextra)
add_definitions(-fPIC)

if (ANDROID)
    add_definitions(-Ofast)
    add_definitions(-ffast-math)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -frtti -fno-exceptions")
    add_definitions(-DANDROID)
    add_definitions(-DQCOM)
    # extra libs and use gold link for android
    link_libraries("log")
    link_libraries("-fuse-ld=gold")
endif()

# set install
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Installation Directory")
endif()
message(STATUS "CMAKE_INSTALL_PREFIX = ${CMAKE_INSTALL_PREFIX}")

#########################Find OpenCL######################################
if (QCOM)
    set(OpenCL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/opencl/qcom/opencl200/include/")
    set(OpenCL_SHARED_LIB "${CMAKE_CURRENT_SOURCE_DIR}/opencl/qcom/opencl200/lib64/libOpenCL.so")
    add_definitions(-DQCOM_DEVICE)

    include_directories(${OpenCL_DIR})
    link_libraries( ${OpenCL_SHARED_LIB})
    message(STATUS "QCOMM opencl set")

    add_definitions(-DKERNEL_PROFILING)
endif()
if (MTK)
    set(OpenCL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/opencl/mtk/opencl210/include/")
    set(OpenCL_SHARED_LIB "${CMAKE_CURRENT_SOURCE_DIR}/opencl/mtk/opencl210/lib64/libOpenCL.so")
    add_definitions(-DMTK_DEVICE)
    add_definitions(-DCL_TARGET_OPENCL_VERSION=210)
    
    include_directories(${OpenCL_DIR})
    link_libraries( ${OpenCL_SHARED_LIB})
    message(STATUS "MTK opencl set")
endif()
###############################################################

project(main)
add_executable(main ./main.cpp)
install(TARGETS main DESTINATION bin)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/kerneltest.cl" DESTINATION bin)