#!/usr/bin/env python3

import os
import sys
import json
import argparse

# ========== Color Logging ==========
class Color:
    RESET = "\033[0m"
    RED = "\033[91m"
    GREEN = "\033[92m"
    YELLOW = "\033[93m"
    CYAN = "\033[96m"
    BOLD = "\033[1m"

def log_info(msg: str): print(f"{Color.CYAN}[INFO]{Color.RESET} {msg}")
def log_success(msg: str): print(f"{Color.GREEN}[OK]{Color.RESET} {msg}")
def log_warn(msg: str): print(f"{Color.YELLOW}[WARN]{Color.RESET} {msg}")
def log_error(msg: str): print(f"{Color.RED}[ERROR]{Color.RESET} {msg}")
def log_command(msg: str): print(f"{Color.BOLD}{Color.CYAN}>> {msg}{Color.RESET}")

# ========== Command Executor ==========
def cmd(command: str):
    try:
        log_command(f"Executing command: {command}")     
        os.system(command)
    except Exception as e:
        log_error(str(e))
        sys.exit(1)

# ========== Package Definition ==========
class Package:
    def __init__(self, name: str, source_directory: str, build_directory: str, prefix: str, options: str):
        self.name = name
        self.source_directory = source_directory
        self.build_directory = build_directory
        self.prefix_directory = prefix
        self.options = options

# ========== CMake Preset Generators ==========
def get_base_configure_preset():
    return {
        "name": "common-base",
        "hidden": True,
        "binaryDir": "${sourceDir}/build/config",
        "installDir": "${sourceDir}/build/packages"
    }

def get_os_base_configure_preset(os: str, inherits: str, cache_variables: list):
    return {
        "name": os.lower() + "-base",
        "hidden": True,
        "inherits": inherits,
        "condition": {
            "type": "equals",
            "lhs": "${hostSystemName}",
            "rhs": "Darwin" if os == "macOS" else os
        },
        "cacheVariables": {
            "CMAKE_BUILD_TYPE": cache_variables[0],
            "CMAKE_INSTALL_PREFIX": cache_variables[1],
            "CMAKE_PREFIX_PATH": cache_variables[1]
        },
        "vendor": {
            "microsoft.com/VisualStudioSettings/CMake/1.0": { "hostOS": [os] },
            "microsoft.com/VisualStudioRemoteSettings/CMake/1.0": {
                "sourceDir": "$env{HOME}/.vs/$ms{projectDirName}"
            }
        }
    }

def get_os_preset(os: str, inherits: str, arch: str, conf: str):
    return {
        "name": f"{os.lower()}-{arch}-{conf.lower()}",
        "inherits": inherits,
        "displayName": f"{arch}-{conf}",
        "architecture": {"value": arch, "strategy": "external"},
        "cacheVariables": {"CMAKE_BUILD_TYPE": conf}
    }

def generate_preset(dir: str, preset_cache_variables: list):
    configure_presets = []
    build_presets = []
    test_presets = []

    base_configure = get_base_configure_preset()
    base_build = {"name": "common-base", "hidden": True, "jobs": 1, "cleanFirst": False}
    base_test = {
        "name": "common-base",
        "hidden": True,
        "execution": {"noTestsAction": "error", "stopOnFailure": False},
        "output": {"outputOnFailure": True}
    }

    configure_presets.append(base_configure)
    build_presets.append(base_build)
    test_presets.append(base_test)

    os_names = ["Linux", "Windows", "macOS"]
    configs = ["Debug", "Release"]

    for os_name in os_names:
        os_base = get_os_base_configure_preset(os_name, base_configure["name"], preset_cache_variables)
        configure_presets.append(os_base)

        for conf in configs:
            for arch in ["x64", "x86"]:
                conf_preset = get_os_preset(os_name, os_base["name"], arch, conf)
                configure_presets.append(conf_preset)
                build_presets.append({
                    "name": conf_preset["name"],
                    "inherits": os_base["name"],
                    "displayName": conf_preset["displayName"],
                    "configurePreset": conf_preset["name"]
                })
                test_presets.append({
                    "name": conf_preset["name"],
                    "inherits": os_base["name"],
                    "displayName": conf_preset["displayName"],
                    "configurePreset": conf_preset["name"]
                })

    root_presets = {
        "version": 3,
        "configurePresets": configure_presets,
        "buildPresets": build_presets,
        "testPresets": test_presets
    }

    preset_path = os.path.join(dir, "CMakePresets.json")
    if os.path.exists(preset_path):
        os.remove(preset_path)
        log_warn(f"Deleted old CMakePresets.json in {dir}")

    with open(preset_path, "w") as f:
        json.dump(root_presets, f, indent=2)
    log_success(f"Created CMakePresets.json in {dir}")

# ========== Helpers ==========
def get_preset_cache_variables(platform: str, build_type: str, projlibs: list):
    return ":".join(os.path.abspath(lib.prefix_directory) for lib in projlibs) if platform != "Windows" \
        else ";".join(os.path.abspath(lib.prefix_directory) for lib in projlibs)

# ========== Main ==========
if __name__ == "__main__":
    external_packages = [
        Package("glfw", "glfw", "build/config/glfw", "build/packages/glfw", "-DGLFW_BUILD_EXAMPLES=OFF -DGLFW_BUILD_TESTS=OFF -DGLFW_BUILD_DOCS=OFF"),
        Package("spdlog", "spdlog", "build/config/spdlog", "build/packages/spdlog", "-DSPDLOG_BUILD_EXAMPLES=OFF"),
        Package("glad", "glad", "build/config/glad", "build/packages/glad", ""),
        Package("glm", "glm", "build/config/glm", "build/packages/glm", "-DGLM_BUILD_TESTS=OFF"),
        Package("SOIL2", "SOIL2", "build/config/SOIL2", "build/packages/SOIL2", ""),
        Package("imgui", "imgui_docking", "build/config/imgui", "build/packages/imgui", ""),
        Package("entt", "entt", "build/config/entt", "build/packages/entt", "-DENTT_INCLUDE_HEADERS=ON -DENTT_INCLUDE_NATVIS=ON -DENTT_INSTALL=ON"),
        Package("fastgltf", "fastgltf", "build/config/fastgltf", "build/packages/fastgltf", "")
    ]

    parser = argparse.ArgumentParser(description="Build script for the Motion Engine")
    parser.add_argument("--config", type=str, required=True, choices=["Debug", "Release"])
    parser.add_argument("--arch", type=str, required=True, choices=["x86", "x64"])
    parser.add_argument("--pkg", type=str, help="Specific package to build")
    args = parser.parse_args()

    build_type = args.config
    build_arch = args.arch
    build_package = args.pkg
    build_system_name = "Windows" if sys.platform == "win32" else "macOS" if sys.platform == "darwin" else "Linux"

    preset_cache = get_preset_cache_variables(build_system_name, build_type, external_packages)

    # Pick which packages to build
    packages_to_build = []
    if build_package:
        for pkg in external_packages:
            if pkg.name.lower() == build_package.lower():
                packages_to_build.append(pkg)
                break
        if not packages_to_build:
            log_error(f"Package '{build_package}' not found.")
            sys.exit(1)
    else:
        packages_to_build = external_packages

    # Build the packages
    for pkg in packages_to_build:
        log_info(f"Building package: {Color.BOLD}{pkg.name}{Color.RESET}")
        cmd(f"cmake -DCMAKE_BUILD_TYPE_INIT=\"{build_type}\" -DCMAKE_SYSTEM_NAME=\"{build_system_name}\" -DCMAKE_PREFIX_PATH=\"{preset_cache}\" -DCMAKE_INSTALL_PREFIX=\"{preset_cache}\" {pkg.options} -S \"{pkg.source_directory}\" -B \"{pkg.build_directory}\"")
        cmd(f"cmake --build \"{pkg.build_directory}\" --config \"{build_type}\"")
        cmd(f"cmake --install \"{pkg.build_directory}\" --config \"{build_type}\" --prefix \"{pkg.prefix_directory}\"")

    # Generate CMakePresets
    generate_preset("../", [build_type, preset_cache])