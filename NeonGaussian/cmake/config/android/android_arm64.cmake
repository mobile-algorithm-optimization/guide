message(STATUS "********** android_none_arm64.cmake **********")

if(NOT DEFINED ENV{NDK_PATH})
    message(FATAL_ERROR "android ndk error")
endif()

# arm
set(NDK_PATH $ENV{NDK_PATH})
set(CMAKE_TOOLCHAIN_FILE ${NDK_PATH}/build/cmake/android.toolchain.cmake)
set(ANDROID_ABI arm64-v8a)
set(ANDROID_ARM_NEON ON)
set(ANDROID_PLATFORM android-23)

message(STATUS "NDK_PATH = ${NDK_PATH}")
message(STATUS "CMAKE_TOOLCHAIN_FILE = ${CMAKE_TOOLCHAIN_FILE}")
message(STATUS "ANDROID_ABI = ${ANDROID_ABI}")
message(STATUS "ANDROID_ARM_NEON = ${ANDROID_ARM_NEON}")
message(STATUS "ANDROID_PLATFORM = ${ANDROID_PLATFORM}")

message(STATUS "***************************************")
