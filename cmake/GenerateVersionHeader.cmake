function(generate_version_header)
    set(options "")
    set(oneValueArgs TARGET VERSION_FILE)
    set(multiValueArgs "")
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Get version components
    if(PROJECT_VERSION)
        string(REGEX MATCH "^([0-9]+)\\.([0-9]+)\\.([0-9]+)" _ ${PROJECT_VERSION})
        set(VERSION_MAJOR ${CMAKE_MATCH_1})
        set(VERSION_MINOR ${CMAKE_MATCH_2})
        set(VERSION_PATCH ${CMAKE_MATCH_3})
    else()
        set(VERSION_MAJOR 1)
        set(VERSION_MINOR 0)
        set(VERSION_PATCH 0)
    endif()

    # Try to get Git information
    find_package(Git QUIET)
    if(GIT_FOUND)
        # Get git commit hash
        execute_process(
            COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_COMMIT_HASH
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
        
        # Get git branch
        execute_process(
            COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_BRANCH
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
        
        # Check if working directory is dirty
        execute_process(
            COMMAND ${GIT_EXECUTABLE} diff-index --quiet HEAD --
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            RESULT_VARIABLE GIT_DIRTY_RESULT
            ERROR_QUIET
        )
        
        if(GIT_DIRTY_RESULT EQUAL 0)
            set(GIT_DIRTY "false")
        else()
            set(GIT_DIRTY "true")
        endif()
    else()
        set(GIT_COMMIT_HASH "unknown")
        set(GIT_BRANCH "unknown")
        set(GIT_DIRTY "false")
    endif()

    # Get build timestamp
    string(TIMESTAMP BUILD_TIMESTAMP "%Y-%m-%d %H:%M:%S UTC" UTC)
    
    # Get build type
    if(CMAKE_BUILD_TYPE)
        set(BUILD_TYPE ${CMAKE_BUILD_TYPE})
    else()
        set(BUILD_TYPE "Unknown")
    endif()

    # Generate version string
    set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
    if(GIT_COMMIT_HASH AND NOT GIT_COMMIT_HASH STREQUAL "unknown")
        set(VERSION_STRING "${VERSION_STRING}-${GIT_COMMIT_HASH}")
    endif()
    if(GIT_DIRTY STREQUAL "true")
        set(VERSION_STRING "${VERSION_STRING}-dirty")
    endif()

    # Configure the header file
    configure_file(
        ${CMAKE_SOURCE_DIR}/cmake/MotionVersion.hpp.in
        ${ARG_VERSION_FILE}
        @ONLY
    )
    
    message(STATUS "Generated version header: ${VERSION_STRING}")
endfunction()