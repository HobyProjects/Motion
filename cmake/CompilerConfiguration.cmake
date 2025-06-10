function(SetCompilerConfigurations cppStandard cStandard)
    set(CMAKE_CXX_STANDARD ${cppStandard})
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
    set(CMAKE_C_STANDARD ${cStandard})
    set(CMAKE_C_STANDARD_REQUIRED ON)
    set(CMAKE_C_EXTENSIONS OFF)

    message(STATUS "CMAKE_CXX_STANDARD: ${CMAKE_CXX_STANDARD}")
    message(STATUS "CMAKE_C_STANDARD: ${CMAKE_C_STANDARD}")
    message(STATUS "CMAKE_CXX_STANDARD_REQUIRED: ${CMAKE_CXX_STANDARD_REQUIRED}")
    message(STATUS "CMAKE_C_STANDARD_REQUIRED: ${CMAKE_C_STANDARD_REQUIRED}")
    message(STATUS "CMAKE_CXX_EXTENSIONS: ${CMAKE_CXX_EXTENSIONS}")
    message(STATUS "CMAKE_C_EXTENSIONS: ${CMAKE_C_EXTENSIONS}")

    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        add_compile_definitions(MOTION_COMPILER_CLANG)
        message(STATUS "CMAKE_CXX_COMPILER_ID: Clang")

    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        add_compile_definitions(MOTION_COMPILER_GCC)
        message(STATUS "CMAKE_CXX_COMPILER_ID: GNU")

    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
        add_compile_definitions(MOTION_COMPILER_APPLE_CLANG)
        message(STATUS "CMAKE_CXX_COMPILER_ID: AppleClang")

    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        add_compile_definitions(MOTION_COMPILER_MSVC)
        message(STATUS "CMAKE_CXX_COMPILER_ID: MSVC")

    else()
        message(FATAL_ERROR "Unsupported compiler: ${CMAKE_CXX_COMPILER_ID}")
    endif()

    message(STATUS "CMAKE_CXX_COMPILER_VERSION: ${CMAKE_CXX_COMPILER_VERSION}")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 11.0)
        message(FATAL_ERROR "C++20 requires at least GCC 11.0, Clang 11.0, or MSVC 19.29")
    endif()

    if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
        add_compile_definitions(MOTION_ARCH_X86_64)
        message(STATUS "CMAKE_SYSTEM_PROCESSOR: x86_64 or AMD64")

    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
        add_compile_definitions(MOTION_ARCH_X86_64)
        message(STATUS "CMAKE_SYSTEM_PROCESSOR: x86_64")

    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86")
        add_compile_definitions(MOTION_ARCH_X86)
        message(STATUS "CMAKE_SYSTEM_PROCESSOR: x86")

    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
        add_compile_definitions(MOTION_ARCH_ARM64)
        message(STATUS "CMAKE_SYSTEM_PROCESSOR: aarch64")

    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
        add_compile_definitions(MOTION_ARCH_ARM64)
        message(STATUS "CMAKE_SYSTEM_PROCESSOR: arm64")
    else()
        message(FATAL_ERROR "Unsupported architecture: ${CMAKE_SYSTEM_PROCESSOR}")
    endif()

    if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
        message(STATUS "Setting build type to 'Release' as none was specified.")
    
    elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_definitions(MOTION_BUILD_DEBUG)
        message(STATUS "CMAKE_BUILD_TYPE: Debug")

    elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_definitions(MOTION_BUILD_RELEASE)
        message(STATUS "CMAKE_BUILD_TYPE: Release")

    else()
        message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
    endif()
  
endfunction()

