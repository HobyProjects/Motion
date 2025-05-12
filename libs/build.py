#!/usr/bin/env python3

import os
import sys
import json
import argparse
import subprocess
import hashlib

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
        log_command(f"{command}")
        result = subprocess.run(command, shell=True, check=True, text=True)
    except subprocess.CalledProcessError as e:
        log_error(f"Command failed: {e}")
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

def get_base_configure_preset(generator: str):
    return {
        "name": "common-base",
        "hidden": True,
        "binaryDir": "${sourceDir}/build/config",
        "installDir": "${sourceDir}/build/packages",
        "generator": generator,
        "cacheVariables": {
            "CMAKE_EXPORT_COMPILE_COMMANDS": "ON"
        }
    }

def get_os_base_configure_preset(os_name: str, inherits_from: str, build_type: str, prefix_path: str):
    return {
        "name": os_name.lower() + "-base",
        "hidden": True,
        "inherits": inherits_from,
        "condition": {
            "type": "equals",
            "lhs": "${hostSystemName}",
            "rhs": "Darwin" if os_name == "macOS" else os_name
        },
        "cacheVariables": {
            "CMAKE_BUILD_TYPE": build_type,
            "CMAKE_INSTALL_PREFIX": prefix_path,
            "CMAKE_PREFIX_PATH": prefix_path
        }
    }

def get_os_preset(os: str, inherits: str, arch: str, conf: str, generator: str):
    return {
        "name": f"{os.lower()}-{arch.lower()}-{conf.lower()}",
        "inherits": inherits,
        "displayName": f"{arch}-{conf}",
        "architecture": {
            "value": arch,
            "strategy": "external"
        },
        "cacheVariables": {
            "CMAKE_BUILD_TYPE": conf
        },
        "generator": generator,
        "binaryDir": f"${{sourceDir}}/build/config/{os.lower()}-{arch.lower()}-{conf.lower()}"
    }

def generate_presets(dir: str, build_type: str, generator: str, prefix_path: str):
    configure_presets = []
    build_presets = []
    test_presets = []

    base_configure = get_base_configure_preset(generator)
    base_build = {
        "name": "common-base",
        "hidden": True,
        "jobs": os.cpu_count(),  # Dynamically set to available CPU cores
        "cleanFirst": True
    }
    base_test = {
        "name": "common-base",
        "hidden": True,
        "execution": {
            "noTestsAction": "error",
            "stopOnFailure": False
        },
        "output": {
            "outputOnFailure": True
        }
    }

    configure_presets.append(base_configure)
    build_presets.append(base_build)
    test_presets.append(base_test)

    os_names = ["Linux", "Windows", "macOS"]
    configs = ["Debug", "Release"]
    archs = ["x86_64", "x86"]

    for os_name in os_names:
        os_base = get_os_base_configure_preset(os_name, base_configure["name"], build_type, prefix_path)
        configure_presets.append(os_base)

        for conf in configs:
            for arch in archs:
                preset_name = f"{os_name.lower()}-{arch.lower()}-{conf.lower()}"
                conf_preset = get_os_preset(os_name, os_base["name"], arch, conf, generator)
                configure_presets.append(conf_preset)

                build_presets.append({
                    "name": preset_name,
                    "inherits": base_build["name"],
                    "configurePreset": preset_name
                })

                test_presets.append({
                    "name": preset_name,
                    "inherits": base_test["name"],
                    "configurePreset": preset_name
                })

    root_presets = {
        "version": 3,
        "cmakeMinimumRequired": {
            "major": 3,
            "minor": 24,
            "patch": 0
        },
        "configurePresets": configure_presets,
        "buildPresets": build_presets,
        "testPresets": test_presets
    }

    preset_path = os.path.join(dir, "CMakePresets.json")
    
    # Avoid regenerating if the file is unchanged
    if os.path.exists(preset_path):
        with open(preset_path, "r") as f:
            existing_data = json.load(f)
            new_data = json.dumps(root_presets, indent=2)
            if hashlib.md5(new_data.encode()).hexdigest() == hashlib.md5(json.dumps(existing_data, indent=2).encode()).hexdigest():
                log_info("CMakePresets.json is already up-to-date, skipping regeneration.")
                return

        os.remove(preset_path)
        log_warn(f"Deleted old CMakePresets.json in {dir}")

    with open(preset_path, "w") as f:
        json.dump(root_presets, f, indent=2)
    log_success(f"Created CMakePresets.json in {dir}")

# ========== Helpers ==========
def get_preset_cache_variables(platform: str, build_type: str, projlibs: list):
    return ":".join(os.path.abspath(lib.prefix_directory) for lib in projlibs) if platform != "Windows" \
        else ";".join(os.path.abspath(lib.prefix_directory) for lib in projlibs)

def check_cmake_installed():
    try:
        # Check if 'cmake' is available in the system's PATH
        result = subprocess.run(["cmake", "--version"], capture_output=True, text=True, check=True)
        log_success(f"CMake version: {result.stdout.strip()}")
    except FileNotFoundError:
        log_error("CMake is not installed or not found in the system PATH.")
        sys.exit(1)
    except subprocess.CalledProcessError as e:
        log_error(f"Error checking CMake version: {e}")
        sys.exit(1)

def detect_build_system():
    log_info("Search for generators...")
    default_generator = None

    # Check for Ninja
    try:
        output = subprocess.call(["ninja", "--version"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        if output == 0:
            log_success(" -- Generator found: Ninja")
            default_generator = "Ninja"
    except FileNotFoundError:
        pass

    # Check for NMake (MSVC)
    try:
        output = subprocess.call(["nmake", "/?"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        if output == 0:
            log_success(" -- Generator found: NMake Makefiles")
            default_generator = "NMake Makefiles"
    except FileNotFoundError:
        pass

    # Check for Unix Makefiles
    try:
        output = subprocess.call(["make", "--version"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        if output == 0:
            log_success(" -- Generator found: Unix Makefiles")
            default_generator = "Unix Makefiles"
    except FileNotFoundError:
        pass

    # Visual Studio generators from cmake help
    try:
        output = subprocess.check_output(["cmake", "--help"], text=True)
        for line in output.splitlines():
            if "Visual Studio" in line and "=" in line:
                # Split at '=' and clean the generator name
                gen_name = line.split("=")[0].strip().lstrip("* ").strip()
                log_success(f" -- Generator found: {gen_name}")
                if line.strip().startswith("*"):
                    default_generator = gen_name
    except Exception:
        pass

    return default_generator

# ========== Main ==========
if __name__ == "__main__":
    # Check if CMake is installed
    check_cmake_installed()

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
    parser.add_argument("--arch", type=str, required=True, choices=["x86", "x86_64"])
    parser.add_argument("--pkg", type=str, help="Specific package to build")
    parser.add_argument("--dry-run", action="store_true", help="Only print the commands without executing them")
    parser.add_argument("--clean", action="store_true", help="Clean the build directories before building")
    args = parser.parse_args()

    build_type = args.config
    build_arch = args.arch
    build_package = args.pkg
    build_system_name = "Windows" if sys.platform == "win32" else "macOS" if sys.platform == "darwin" else "Linux"

    build_generator = detect_build_system()
    if build_generator == None:
        log_error("Unable to find generator. Can not produce CMakePresets.json without a generator.")
        sys.exit(2)

    log_info(f"Using system default generator: {build_generator}")

    preset_cache = get_preset_cache_variables(build_system_name, build_type, external_packages)
    generate_presets("../", build_type, build_generator, preset_cache)

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

    for pkg in packages_to_build:
        log_info(f"Building package: {Color.BOLD}{pkg.name}{Color.RESET}")
        
        if args.clean:
            cmd(f"rm -rf {pkg.build_directory}")
            log_success(f"Cleaned build directory for {pkg.name}")

        if args.dry_run:
            log_info(f"Dry-run mode: cmake -DCMAKE_BUILD_TYPE=\"{build_type}\" -DCMAKE_SYSTEM_NAME=\"{build_system_name}\" -DCMAKE_PREFIX_PATH=\"{preset_cache}\" -DCMAKE_INSTALL_PREFIX=\"{pkg.prefix_directory}\" {pkg.options} -S \"{pkg.source_directory}\" -B \"{pkg.build_directory}\" -G \"{build_generator}\"")
            log_info(f"Dry-run mode: cmake --build \"{pkg.build_directory}\" --config \"{build_type}\"")
            log_info(f"Dry-run mode: cmake --install \"{pkg.build_directory}\" --config \"{build_type}\" --prefix \"{pkg.prefix_directory}\"")
        else:
            cmd(f"cmake -DCMAKE_BUILD_TYPE=\"{build_type}\" -DCMAKE_SYSTEM_NAME=\"{build_system_name}\" -DCMAKE_PREFIX_PATH=\"{preset_cache}\" -DCMAKE_INSTALL_PREFIX=\"{pkg.prefix_directory}\" {pkg.options} -S \"{pkg.source_directory}\" -B \"{pkg.build_directory}\" -G \"{build_generator}\"")
            cmd(f"cmake --build \"{pkg.build_directory}\" --config \"{build_type}\"")
            cmd(f"cmake --install \"{pkg.build_directory}\" --config \"{build_type}\" --prefix \"{pkg.prefix_directory}\"")

    log_success("Build Success!!!")