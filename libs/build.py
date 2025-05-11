#!/usr/bin/env python

import os
import sys
import json
import argparse

#============================================================================================================================================

def echo(message: str):
    print(f"-- [BUILDER]: {message}")


def cmd(command: str):
    try:
        echo("Executing command: " + command)
        os.system(command)
    except Exception as e:
        echo(e)
        echo("Failed to execute command: " + command)
        sys.exit(1)

#============================================================================================================================================

class PreCompileLibraries:
    def __init__(self, name: str, source_directory: str, build_directory: str, prefix: str, options: str):
        self.name = name
        self.source_directory = source_directory
        self.build_directory = build_directory
        self.prefix_directory = prefix
        self.options = options

    def __str__(self):
        return f"Name: {self.name}\nDir: {self.source_directory}\nBuild Dir: {self.build_directory}\nPrefix: {self.prefix_dir}\n Options: {self.options}"
    

#============================================================================================================================================

def get_base_configure_preset():
    return {
        "name": "common-base",
        "hidden": True,
        "binaryDir": "${sourceDir}/build/config",
        "installDir": "${sourceDir}/build/packages"
    }

def get_os_base_configure_preset(os: str, inherits: str, cache_variables: list):
    result = {
        "name": os.lower() + "-base",
        "hidden": True,
        "inherits": inherits,
        "condition": {
            "type": "equals",
            "lhs": "${hostSystemName}",
            "rhs": "Darwin" if os == "macOS" else os
        }
    }
    result["cacheVariables"] = {
        "CMAKE_BUILD_TYPE": f"{cache_variables[0]}",
        "CMAKE_INSTALL_PREFIX": f"{cache_variables[1]}",
        "CMAKE_PREFIX_PATH": f"{cache_variables[1]}"
    }
    result["vendor"] = {
        "microsoft.com/VisualStudioSettings/CMake/1.0": {"hostOS": [os]},
        "microsoft.com/VisualStudioRemoteSettings/CMake/1.0": {"sourceDir": "$env{HOME}/.vs/$ms{projectDirName}"}
    }
    return result

def get_os_preset(os: str, inherits: str, arch: str, conf: str):
    result = {
        "name": f"{os.lower()}-{arch}-{conf.lower()}",
        "inherits": inherits,
        "displayName": f"{arch}-{conf}",
        "architecture": { "value": arch, "strategy": "external"},
        "cacheVariables": {
            "CMAKE_BUILD_TYPE": f"{conf}"
        }
    }
    return result

def generate_preset(dir: str, preset_cache_variables: list):
    configure_presets = []
    build_presets = []
    test_presets = []
    base_configure_preset = get_base_configure_preset()
    base_build_preset = {
        'name': 'common-base',
        'hidden': True,
        'jobs': 1,
        'cleanFirst': False
    }
    base_test_preset = {
        "name": "common-base",
        "description": "Basic shared test settings",
        "hidden": True,
        "execution": {
            "noTestsAction": "error",
            "stopOnFailure": False
        },
        "output": {
            "outputOnFailure": True
        }
    }
    configure_presets.append(base_configure_preset)
    build_presets.append(base_build_preset)
    test_presets.append(base_test_preset)

    os_names = [ "Linux" , "Windows", "macOS"]
    configs = [ "Debug", "Release"]
    
    for _os in os_names:
        os_base_configure_preset = get_os_base_configure_preset(_os, base_configure_preset["name"], preset_cache_variables)
        configure_presets.append(os_base_configure_preset)
        
        for conf in configs:
            conf_preset = get_os_preset(_os, os_base_configure_preset["name"], "x64", conf)
            configure_presets.append(conf_preset)
            conf_preset = get_os_preset(_os, os_base_configure_preset["name"], "x86", conf)
            configure_presets.append(conf_preset)
        
        build_presets.append({"name": os_base_configure_preset["name"], "inherits": base_build_preset["name"], "hidden": True})
        test_presets.append({"name": os_base_configure_preset["name"], "inherits": base_test_preset["name"], "hidden": True}) 
    
    for conf_preset in configure_presets:
        if conf_preset.get('hidden', False) == True:
            continue
        build_presets.append({
            "name": conf_preset["name"],
            "inherits": conf_preset["inherits"],
            "displayName": conf_preset["displayName"],
            "configurePreset": conf_preset["name"],
        })
        test_presets.append({
            "name": conf_preset["name"],
            "inherits": conf_preset["inherits"],
            "displayName": conf_preset["displayName"],
            "configurePreset": conf_preset["name"],
        })
    root_presets = {
        "version": 3,
        "configurePresets": configure_presets,
        "buildPresets": build_presets,
        "testPresets": test_presets
    }
    if os.path.exists(f"{dir}/CMakePresets.json"):
        os.remove(f"{dir}/CMakePresets.json")
        echo(f"Deleting Old CMakePresets.json in {dir}")

    with open(f"{dir}/CMakePresets.json", 'a+') as f:
        json.dump(root_presets, f, indent=2)
    echo(f"Created CMakePresets.json in {dir}")


def get_preset_cache_variables(platform: str, build_type: str, projlibs: list):
    preset_cache_variables = str()
    for lib in projlibs:
        if platform == "Windows":
            preset_cache_variables += f"{os.path.abspath(lib.prefix_directory)};"
        else:
            preset_cache_variables += f"{os.path.abspath(lib.prefix_directory)}:"
            
    return preset_cache_variables


def get_cmake_cache_variables(platform: str, build_type: str, projlibs: list):
    targets_cache_variables = str()
    for lib in projlibs:
        if platform == "Windows":
            targets_cache_variables += f"{lib.prefix_directory};"
        else:
            targets_cache_variables += f"{lib.prefix_directory}:"

    return targets_cache_variables

#============================================================================================================================================

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Build script for the Dvimana Engine"
    )
    parser.add_argument(
        "--config",
        type=str,
        help="The build configuration to use. The following values are valid:"
            "  - Debug: Build with debugging symbols and no optimizations."
            "  - Release: Build with optimizations but no debugging symbols."
    )
    parser.add_argument(
        "--arch",
        type=str,
        help="The architecture to build for. The following values are valid:"
            "  - x86: Build for x86 architecture."
            "  - x64: Build for x64 architecture."
    )

    args = parser.parse_args()

    if args.config != "Debug" and args.config != "Release":
        echo(f"Invalid build configuration: {args.config}")
        echo("Valid configurations are: Debug, Release")
        echo("")
        echo("The build configuration determines whether the build is done with")
        echo("debugging symbols and no optimizations, or with optimizations but")
        echo("no debugging symbols.")
        sys.exit(1)

    if args.arch != "x86" and args.arch != "x64":
        echo(f"Invalid architecture: {args.arch}")
        echo("Valid architectures are: x86, x64")
        echo("")
        echo("The target architecture determines whether the build is done for")
        echo("x86 or x64 architecture.")
        sys.exit(1)


    TARGETS_BUILD_TYPE = args.config
    TARGETS_BUILD_ARCHITECTURE = args.arch
    TARGETS_BUILD_SYSTEM_NAME = str()
    
    if sys.platform == "win32":
        TARGETS_BUILD_SYSTEM_NAME = "Windows"
    elif sys.platform == "darwin":
        TARGETS_BUILD_SYSTEM_NAME = "MacOS"
    else:
        TARGETS_BUILD_SYSTEM_NAME = "Linux"

#============================================================================================================================================

    EXTERNAL_LIBRARIES = [
        PreCompileLibraries("glfw", "glfw", "build/config/glfw", "build/packages/glfw", "-DGLFW_BUILD_EXAMPLES=OFF -DGLFW_BUILD_TESTS=OFF -DGLFW_BUILD_DOCS=OFF"),
        PreCompileLibraries("spdlog", "spdlog", "build/config/spdlog", "build/packages/spdlog", "-DSPDLOG_BUILD_EXAMPLES=OFF"),
        #PreCompileLibraries("SDL", "SDL", "build/config/SDL", "build/packages/SDL", "-DSDL_TEST_LIBRARY=OFF -DSDL_TESTS=OFF -DSDL_EXAMPLES=OFF -DSDL_STATIC=ON"),
        PreCompileLibraries("glad", "glad", "build/config/glad", "build/packages/glad", ""),
        PreCompileLibraries("glm", "glm", "build/config/glm", "build/packages/glm", "-DGLM_BUILD_TESTS=OFF"),
        #PreCompileLibraries("stb", "stb_image", "build/config/stb", "build/packages/stb", ""),
        PreCompileLibraries("SOIL2", "SOIL2", "build/config/SOIL2", "build/packages/SOIL2", ""),
        PreCompileLibraries("assimp", "assimp", "build/config/assimp", "build/packages/assimp", ""),
        PreCompileLibraries("imgui", "imgui_docking", "build/config/imgui", "build/packages/imgui", ""),
        PreCompileLibraries("entt", "entt", "build/config/entt", "build/packages/entt", "-DENTT_INCLUDE_HEADERS=ON -DENTT_INCLUDE_NATVIS=ON -DENTT_INSTALL=ON"),
        PreCompileLibraries("json", "json", "build/config/json", "build/packages/json", ""),
    ]

#============================================================================================================================================

    preset_cache_variables = get_preset_cache_variables(TARGETS_BUILD_SYSTEM_NAME, TARGETS_BUILD_TYPE, EXTERNAL_LIBRARIES)
    cmake_cache_variables = get_cmake_cache_variables(TARGETS_BUILD_SYSTEM_NAME, TARGETS_BUILD_TYPE, EXTERNAL_LIBRARIES)

    for external_module in EXTERNAL_LIBRARIES:
        cmd(f"cmake -DCMAKE_BUILD_TYPE_INIT={TARGETS_BUILD_TYPE} -DCMAKE_SYSTEM_NAME={TARGETS_BUILD_SYSTEM_NAME} -DCMAKE_PREFIX_PATH={cmake_cache_variables} -DCMAKE_INSTALL_PREFIX={cmake_cache_variables} {external_module.options} -S {external_module.source_directory} -B {external_module.build_directory}")
        cmd(f"cmake --build {external_module.build_directory} --config {TARGETS_BUILD_TYPE}")
        cmd(f"cmake --install {external_module.build_directory} --config {TARGETS_BUILD_TYPE} --prefix {external_module.prefix_directory}")

    generate_preset("../", [ TARGETS_BUILD_TYPE, preset_cache_variables ])

#============================================================================================================================================