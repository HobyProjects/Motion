cmake_minimum_required (VERSION 3.15)
project(imgui C CXX)

find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)

set(IMGUI_LIBRARY imgui)

set(
    IMGUI_HEADERS
        imgui/imconfig.h
		imgui/imgui.h
		imgui/imgui_internal.h
		imgui/imstb_rectpack.h
		imgui/imstb_textedit.h
		imgui/imstb_truetype.h
        imgui/backends/imgui_impl_glfw.h
        imgui/backends/imgui_impl_opengl3.h
)

set(
	IMGUI_SOURCES
		imgui/imgui.cpp
		imgui/imgui_draw.cpp
		imgui/imgui_widgets.cpp
		imgui/imgui_tables.cpp
		imgui/imgui_demo.cpp
		imgui/backends/imgui_impl_glfw.cpp
		imgui/backends/imgui_impl_opengl3.cpp
)

add_library(
	${IMGUI_LIBRARY}
		STATIC 
			${IMGUI_HEADERS}
			${IMGUI_SOURCES}
)

target_include_directories(${IMGUI_LIBRARY} PRIVATE imgui imgui/backends)
target_link_libraries(${IMGUI_LIBRARY} PRIVATE glfw OpenGL::GL)
add_library(imgui::imgui ALIAS ${IMGUI_LIBRARY})

set_target_properties(
	${IMGUI_LIBRARY} PROPERTIES
	CXX_STANDARD 20
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS OFF
	C_STANDARD 17
	C_STANDARD_REQUIRED ON
	C_EXTENSIONS OFF
)

include(GNUInstallDirs)

target_include_directories(
	${IMGUI_LIBRARY} PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
	$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

install(
	TARGETS ${IMGUI_LIBRARY}
	EXPORT "${IMGUI_LIBRARY}Targets"
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} 
)

install(
	FILES ${IMGUI_HEADERS}
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${IMGUI_LIBRARY}
)

install(
	EXPORT "${IMGUI_LIBRARY}Targets"
	FILE "${IMGUI_LIBRARY}Targets.cmake"
	NAMESPACE imgui::
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${IMGUI_LIBRARY}
)

include(CMakePackageConfigHelpers)

configure_package_config_file(
	"${CMAKE_CURRENT_LIST_DIR}/config/${IMGUI_LIBRARY}Config.cmake.in"
	"${CMAKE_CURRENT_BINARY_DIR}/${IMGUI_LIBRARY}Config.cmake"
	INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${IMGUI_LIBRARY}"
)

configure_package_config_file(
	"${CMAKE_CURRENT_LIST_DIR}/config/${IMGUI_LIBRARY}.pc.in"
	"${CMAKE_CURRENT_BINARY_DIR}/${IMGUI_LIBRARY}.pc"
	INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig"
)

install(
	FILES "${CMAKE_CURRENT_BINARY_DIR}/${IMGUI_LIBRARY}Config.cmake"
	DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${IMGUI_LIBRARY}"
)

install(
	FILES "${CMAKE_CURRENT_BINARY_DIR}/${IMGUI_LIBRARY}.pc"
	DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig"
)