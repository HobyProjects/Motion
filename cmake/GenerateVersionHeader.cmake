
# Version Control System
find_package(Git QUIET)

function(generate_version_header)
    set(options "")
    set(oneValueArgs TARGET VERSION_FILE)
    set(multiValueArgs "")
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT ARG_TARGET)
        message(FATAL_ERROR "generate_version_header: TARGET argument is required")
    endif()

    if(NOT ARG_VERSION_FILE)
        message(FATAL_ERROR "generate_version_header: VERSION_FILE argument is required")
    endif()

    # Get Git information
    if(GIT_FOUND)
        # Get Git commit hash
        execute_process(
            COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_COMMIT_HASH
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
        
        # Get Git branch
        execute_process(
            COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_BRANCH
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
        
        # Get Git tag
        execute_process(
            COMMAND ${GIT_EXECUTABLE} describe --tags --abbrev=0
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_TAG
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
        
        # Check if working directory is clean
        execute_process(
            COMMAND ${GIT_EXECUTABLE} diff-index --quiet HEAD --
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            RESULT_VARIABLE GIT_IS_DIRTY
            ERROR_QUIET
        )
        
        if(GIT_IS_DIRTY)
            set(GIT_IS_DIRTY_FLAG "true")
        else()
            set(GIT_IS_DIRTY_FLAG "false")
        endif()
        
        # Get commit count
        execute_process(
            COMMAND ${GIT_EXECUTABLE} rev-list --count HEAD
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_COMMIT_COUNT
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
    else()
        set(GIT_COMMIT_HASH "unknown")
        set(GIT_BRANCH "unknown")
        set(GIT_TAG "unknown")
        set(GIT_IS_DIRTY_FLAG "false")
        set(GIT_COMMIT_COUNT "0")
    endif()

    # Get build timestamp
    string(TIMESTAMP BUILD_TIMESTAMP "%Y-%m-%d %H:%M:%S UTC" UTC)

    # Get project version
    if(NOT DEFINED PROJECT_VERSION_MAJOR)
        set(PROJECT_VERSION_MAJOR 0)
    endif()
    if(NOT DEFINED PROJECT_VERSION_MINOR)
        set(PROJECT_VERSION_MINOR 0)
    endif()
    if(NOT DEFINED PROJECT_VERSION_PATCH)
        set(PROJECT_VERSION_PATCH 0)
    endif()

    # Set version string
    set(PROJECT_VERSION_STRING "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")

    # Find the template file
    set(TEMPLATE_FILE "${CMAKE_SOURCE_DIR}/cmake/MotionVersion.hpp.in")
    if(NOT EXISTS ${TEMPLATE_FILE})
        message(FATAL_ERROR "Template file not found: ${TEMPLATE_FILE}")
    endif()

    # Configure the header file
    configure_file(
        ${TEMPLATE_FILE}
        ${ARG_VERSION_FILE}
        @ONLY
    )

    message(STATUS "===============================================")
    message(STATUS " Generated version header: ${ARG_VERSION_FILE}")
    message(STATUS "===============================================")
    message(STATUS " Version    : ${PROJECT_VERSION_STRING}")
    message(STATUS " Git Hash   : ${GIT_COMMIT_HASH}")
    message(STATUS " Git Branch : ${GIT_BRANCH}")
    message(STATUS " Build Type : ${CMAKE_BUILD_TYPE}")
    message(STATUS "===============================================")

    # Make sure the generated directory is added to the target's include directories
    # (Already handled in your CMakeLists.txt but documenting here for clarity)
    
    # Make the version header a dependency of configure
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
        ${CMAKE_SOURCE_DIR}/.git/HEAD
        ${CMAKE_SOURCE_DIR}/.git/index
    )

endfunction()
