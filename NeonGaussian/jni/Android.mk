LOCAL_PATH := ${call my-dir}

include $(CLEAR_VARS)

LOCAL_MODULE := gaussian_example

LOCAL_C_INCLUDES := ../src/inc

LOCAL_SRC_FILES := ../src/test/example.c
LOCAL_SRC_FILES += ../src/test/gaussian3x3_neon.c

LOCAL_ARM_MODE := arm
LOCAL_ARM_NEON := true

#LOCAL_STATIC_LIBRARIES := static_lib
#LOCAL_SHARED_LIBRARIES := share_lib

#build App
include $(BUILD_EXECUTABLE)
