cmake_minimum_required(VERSION 3.15)
project(imguizmo C CXX)

set(PROJECT_DESCRIPTION "Immediate mode 3D gizmo for scene editing and other controls based on Dear Imgui")
set(PROJECT_HOMEPAGE_URL "https://github.com/CedricGuillemet/ImGuizmo.git")
set(PROJECT_VERSION "v1.83")

set(
    IMGUIZMO_HEADERS
        imgui/imconfig.h
		imgui/imgui.h
		imgui/imgui_internal.h
		imgui/imstb_rectpack.h
		imgui/imstb_textedit.h
		imgui/imstb_truetype.h
        ImGuizmo/ImGuizmo.h
        ImGuizmo/ImSequencer.h
        ImGuizmo/ImZoomSlider.h
        ImGuizmo/ImGradient.h
        ImGuizmo/ImCurveEdit.h
        ImGuizmo/GraphEditor.h
)

set(
    IMGUIZMO_SOURCES
		imgui/imgui.cpp
		imgui/imgui_draw.cpp
		imgui/imgui_widgets.cpp
		imgui/imgui_tables.cpp
		imgui/imgui_demo.cpp
        ImGuizmo/ImGuizmo.cpp
        ImGuizmo/ImSequencer.cpp
        ImGuizmo/ImGradient.cpp
        ImGuizmo/ImCurveEdit.cpp
        ImGuizmo/GraphEditor.cpp
)

add_library(
    ${PROJECT_NAME}
        STATIC 
            ${IMGUIZMO_HEADERS}
            ${IMGUIZMO_SOURCES}
)

target_include_directories(${PROJECT_NAME} PRIVATE imgui ImGuizmo)
add_library(imguizmo::imguizmo ALIAS ${PROJECT_NAME})

set_target_properties(
    ${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    C_STANDARD 17
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF
)

include(GNUInstallDirs)

target_include_directories(
    ${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

install(
    TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}Targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(
    FILES ${IMGUIZMO_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
)

install(
    EXPORT ${PROJECT_NAME}Targets
    FILE ${PROJECT_NAME}Targets.cmake
    NAMESPACE imguizmo::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

include(CMakePackageConfigHelpers)

configure_package_config_file(
	${CMAKE_CURRENT_LIST_DIR}/config/${PROJECT_NAME}Config.cmake.in
	${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
	INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}Config
)

configure_package_config_file(
	${CMAKE_CURRENT_LIST_DIR}/config/${PROJECT_NAME}.pc.in
	${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc
	INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

install(
	FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

install(
	FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)
