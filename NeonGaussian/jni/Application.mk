APP_BUILD_SCRIPT=./Android.mk
APP_OPTIM :=release
APP_PLATFORM := android-24
APP_ABI := armeabi-v7a
#APP_ABI := arm64-v8a
APP_CFLAGS := -fexceptions -frtti -fdeclspec
APP_CFLAGS += -DANDROID
APP_STL:=c++_shared
