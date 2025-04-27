cmake_minimum_required(VERSION 3.15)
project(glad C)

set(PROJECT_DESCRIPTION "Multi-Language Vulkan/GL/GLES/EGL/GLX/WGL Loader-Generator based on the official specs.")
set(PROJECT_HOMEPAGE_URL "https://github.com/Dav1dde/glad.git")
set(PROJECT_VERSION "2.0.7")

find_package(OpenGL REQUIRED)

set(
    GLAD_HEADERS
        include/glad/glad.h
        include/KHR/khrplatform.h
)

set(
    GLAD_SOURCES
        src/glad.c
)

add_library(
    glad
        STATIC
            ${GLAD_HEADERS}
            ${GLAD_SOURCES}
)

target_include_directories(
    glad
        PRIVATE
            include
            src
)

target_link_libraries(
    glad
        PRIVATE
            OpenGL::GL
)

add_library(glad::glad ALIAS glad)

set_target_properties(
	glad 
        PROPERTIES
            CXX_STANDARD 20
            CXX_STANDARD_REQUIRED ON
            CXX_EXTENSIONS OFF
            C_STANDARD 17
            C_STANDARD_REQUIRED ON
            C_EXTENSIONS OFF
)

include(GNUInstallDirs)

target_include_directories(
	glad 
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

install(
	TARGETS glad
	EXPORT gladTargets
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} 
)

install(
	FILES  include/glad/glad.h 
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/glad
)

install(
	FILES  include/KHR/khrplatform.h 
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/KHR
)

install(
	EXPORT gladTargets
	FILE gladTargets.cmake
	NAMESPACE glad::
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/glad
)

include(CMakePackageConfigHelpers)

configure_package_config_file(
	${CMAKE_CURRENT_LIST_DIR}/config/gladConfig.cmake.in
	${CMAKE_CURRENT_BINARY_DIR}/gladConfig.cmake
	INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/glad
)

configure_package_config_file(
	${CMAKE_CURRENT_LIST_DIR}/config/glad.pc.in
	${CMAKE_CURRENT_BINARY_DIR}/glad.pc
	INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

install(
	FILES ${CMAKE_CURRENT_BINARY_DIR}/gladConfig.cmake
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/glad
)

install(
	FILES ${CMAKE_CURRENT_BINARY_DIR}/glad.pc
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)