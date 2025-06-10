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

import platform

def get_base_configure_preset(generator: str):
    os_names = ["Linux", "Windows", "macOS", "FreeBSD", "OpenBSD", "NetBSD", "DragonFlyBSD"]
    archs = ["x86_64", "x86", "AMD64", "arm64", "armhf", "aarch64", "armv7l", "armv6l", "i386", "i686"]

    system_processor = platform.machine()
    system_name = platform.system()
    system_version = platform.release()
    system_processor_architecture = platform.architecture()[0]
    system_processor_architecture_vendor = platform.processor()

    if system_name not in os_names:
        log_error(f"Unsupported system name: {system_name}. Supported systems: {', '.join(os_names)}")
        sys.exit(1)

    if system_processor not in archs:
        log_error(f"Unsupported processor architecture: {system_processor}. Supported architectures: {', '.join(archs)}")
        sys.exit(1)

    if system_processor_architecture_vendor == "":
        log_warn("CMake could not detect the processor architecture vendor. Defaulting to 'unknown'.")
        system_processor_architecture_vendor = "unknown"
        
    if system_processor_architecture == "":
        log_warn("CMake could not detect the processor architecture. Defaulting to 'unknown'.")
        system_processor_architecture = "unknown"

    log_info(f"Detected system: {system_name} | {system_version} | ({system_processor} - {system_processor_architecture} - {system_processor_architecture_vendor})")
    log_info(f"Using generator: {generator}")

    return {
        "name": "common-base",
        "hidden": True,
        "binaryDir": "${sourceDir}/build/config",
        "installDir": "${sourceDir}/build/packages",
        "generator": generator,
        "cacheVariables": {
            "CMAKE_EXPORT_COMPILE_COMMANDS": "ON",
            "CMAKE_CXX_STANDARD": "20",
            "CMAKE_CXX_STANDARD_REQUIRED": "ON",   
            "CMAKE_CXX_EXTENSIONS": "OFF",
            "CMAKE_CXX_FLAGS_DEBUG": "-O0 -g",
            "CMAKE_CXX_FLAGS_RELEASE": "-O3 -DNDEBUG",
            "CMAKE_CXX_FLAGS_MINSIZEREL": "-Os -DNDEBUG",
            "CMAKE_CXX_FLAGS_RELWITHDEBINFO": "-O2 -g -DNDEBUG",
            "CMAKE_C_STANDARD": "17",
            "CMAKE_C_STANDARD_REQUIRED": "ON",
            "CMAKE_C_EXTENSIONS": "OFF",
            "CMAKE_C_FLAGS_DEBUG": "-O0 -g",
            "CMAKE_C_FLAGS_RELEASE": "-O3 -DNDEBUG",
            "CMAKE_C_FLAGS_MINSIZEREL": "-Os -DNDEBUG",
            "CMAKE_C_FLAGS_RELWITHDEBINFO": "-O2 -g -DNDEBUG",
            "CMAKE_POSITION_INDEPENDENT_CODE": "ON",
            "CMAKE_SYSTEM_PROCESSOR": system_processor,
            "CMAKE_SYSTEM_NAME": system_name,
            "CMAKE_SYSTEM_VERSION": system_version,
            "CMAKE_SYSTEM_PROCESSOR_ARCHITECTURE": system_processor_architecture,
            "CMAKE_SYSTEM_PROCESSOR_ARCHITECTURE_VENDOR": system_processor_architecture_vendor,   
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
            "CMAKE_PREFIX_PATH": prefix_path,
            "CMAKE_SYSTEM_NAME": os_name,
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

    os_names = ["Linux", "Windows", "macOS", "FreeBSD", "OpenBSD", "NetBSD", "DragonFlyBSD"]
    configs = ["Debug", "Release", "RelWithDebInfo", "MinSizeRel"]
    archs = ["x86_64", "x86", "arm64", "armhf", "aarch64", "armv7l", "armv6l", "i386", "i686"]

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
    found_generators = {}

    # Prefer Ninja
    try:
        subprocess.run(["ninja", "--version"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=True)
        log_success(" -- Generator found: Ninja")
        found_generators["Ninja"] = True
    except Exception:
        pass

    # Prefer NMake
    try:
        subprocess.run(["nmake", "/?"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=True)
        log_success(" -- Generator found: NMake Makefiles")
        found_generators["NMake Makefiles"] = True
    except Exception:
        pass

    # Unix Makefiles
    try:
        subprocess.run(["make", "--version"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=True)
        log_success(" -- Generator found: Unix Makefiles")
        found_generators["Unix Makefiles"] = True
    except Exception:
        pass

    # Visual Studio from cmake help
    try:
        output = subprocess.check_output(["cmake", "--help"], text=True)
        for line in output.splitlines():
            if "Visual Studio" in line and "=" in line:
                gen_name = line.split("=")[0].strip().lstrip("* ").strip()
                log_success(f" -- Generator found: {gen_name}")
                if line.strip().startswith("*"):
                    found_generators[gen_name] = "default"
    except Exception:
        pass

    # Priority list
    priority = ["Ninja", "NMake Makefiles", "Unix Makefiles"]
    for g in priority:
        if g in found_generators:
            return g

    # Fallback to default if no preferred generator was found
    for g, val in found_generators.items():
        if val == "default":
            return g

    return None

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
        Package("assimp", "assimp", "build/config/assimp", "build/packages/assimp", "-DASSIMP_BUILD_TESTS=OFF")
    ]

    parser = argparse.ArgumentParser(description="Build script for the Motion Engine")
    parser.add_argument("--config", type=str, required=True, choices=["Debug", "Release"])
    parser.add_argument("--arch", type=str, required=True, choices=["x86", "x86_64"])
    parser.add_argument("--pkg", type=str, help="Specific package to build")
    parser.add_argument("--list", action="store_true", help="List available packages and exit")
    parser.add_argument("--generate-presets", action="store_true", help="generate CMakePresets.json and exit")
    args = parser.parse_args()

    if args.list:
        log_info("Available packages:")
        for pkg in external_packages:
            log_success(f" -- {pkg.name}")
        sys.exit(0)
        
    build_type = args.config
    build_arch = args.arch
    build_package = args.pkg
    build_system_name = "Windows" if sys.platform == "win32" else "macOS" if sys.platform == "darwin" else "Linux"
    build_generator = detect_build_system()
    preset_cache = get_preset_cache_variables(build_system_name, build_type, external_packages)

    if build_generator == None:
        log_error("Unable to find generator. Can not produce CMakePresets.json without a generator.")
        sys.exit(1)
    else:
        log_info(f"Using system default generator: {build_generator}")

    if args.generate_presets:
        generate_presets("../", build_type, build_generator, preset_cache)
        log_success("CMakePresets.json generation completed.")
        sys.exit(0)

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
        cmd(f"cmake -DCMAKE_BUILD_TYPE=\"{build_type}\" -DCMAKE_SYSTEM_NAME=\"{build_system_name}\" -DCMAKE_PREFIX_PATH=\"{preset_cache}\" -DCMAKE_INSTALL_PREFIX=\"{pkg.prefix_directory}\" {pkg.options} -S \"{pkg.source_directory}\" -B \"{pkg.build_directory}\" -G \"{build_generator}\"")
        cmd(f"cmake --build \"{pkg.build_directory}\" --config \"{build_type}\"")
        cmd(f"cmake --install \"{pkg.build_directory}\" --config \"{build_type}\" --prefix \"{pkg.prefix_directory}\"")

    log_success("🎉 Build completed successfully! All packages were built.")