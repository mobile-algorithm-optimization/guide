if(CONFIG)
    if(${CONFIG} STREQUAL "android_arm32")
        include(cmake/config/android/android_arm32.cmake)
    elseif(${CONFIG} STREQUAL "android_arm64")
        include(cmake/config/android/android_arm64.cmake)
    endif()
endif()


