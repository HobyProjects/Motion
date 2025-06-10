function(ConfigurePlatform)
    if(NOT CMAKE_SYSTEM_NAME)
        message(FATAL_ERROR "CMAKE_SYSTEM_NAME is not set. Please specify the target platform.")
    endif()

    message(STATUS "CMAKE_SYSTEM_NAME: ${CMAKE_SYSTEM_NAME}")
    message(STATUS "CMAKE_SYSTEM_VERSION: ${CMAKE_SYSTEM_VERSION}")
    message(STATUS "CMAKE_SYSTEM_PROCESSOR_ARCHITECTURE: ${CMAKE_SYSTEM_PROCESSOR_ARCHITECTURE}")
    message(STATUS "CMAKE_SYSTEM_PROCESSOR_ARCHITECTURE_VENDOR: ${CMAKE_SYSTEM_PROCESSOR_ARCHITECTURE_VENDOR}")

    if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        message(STATUS "Building for Windows")
        add_compile_definitions(MOTION_PLATFORM_WINDOWS)

    elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        message(STATUS "Building for Linux")
        add_compile_definitions(MOTION_PLATFORM_LINUX)
        message(WARNING "Motion Engine does not support Linux builds yet. This is a placeholder configuration.")

    elseif(CMAKE_SYSTEM_NAME STREQUAL "iOS")
        message(STATUS "Building for iOS")
        add_compile_definitions(MOTION_PLATFORM_IOS)
        message(WARNING "Motion Engine does not support iOS builds yet. This is a placeholder configuration.")

    elseif(CMAKE_SYSTEM_NAME STREQUAL "Android")
        message(STATUS "Building for Android")
        if(NOT ANDROID_NDK)
            message(FATAL_ERROR "ANDROID_NDK is not set. Please specify the path to the Android NDK.")
        endif()
        add_compile_definitions(MOTION_PLATFORM_ANDROID)
        message(WARNING "Motion Engine does not support Android builds yet. This is a placeholder configuration.")

    elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        message(STATUS "Building for macOS")
        add_compile_definitions(MOTION_PLATFORM_MACOS)
        message(WARNING "Motion Engine does not support macOS builds yet. This is a placeholder configuration.")

    else()
        message(FATAL_ERROR "Unsupported platform: ${CMAKE_SYSTEM_NAME}")
        add_compile_definitions(MOTION_PLATFORM_UNKNOWN)
    endif()

endfunction()