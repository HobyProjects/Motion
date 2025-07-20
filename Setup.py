#!/usr/bin/env python3

import os
import sys
import json
import argparse
import subprocess
import hashlib
import platform
from dataclasses import dataclass

# ===================== Logger =====================
class Color:
    RESET = "\033[0m"           # Reset text color
    RED = "\033[91m"            # Red text color
    GREEN = "\033[92m"          # Green text color
    YELLOW = "\033[93m"         # Yellow text color
    CYAN = "\033[96m"           # Cyan text color
    BOLD = "\033[1m"            # Bold text style

class Logger:
    @staticmethod
    def info(msg):
        print(f"{Color.CYAN}[INFO]{Color.RESET} {msg}")

    @staticmethod
    def success(msg):
        print(f"{Color.GREEN}[OK]{Color.RESET} {msg}")

    @staticmethod
    def warn(msg):
        print(f"{Color.YELLOW}[WARN]{Color.RESET} {msg}")

    @staticmethod
    def error(msg):
        print(f"{Color.RED}[ERROR]{Color.RESET} {msg}")

    @staticmethod
    def command(msg):
        print(f"{Color.BOLD}{Color.CYAN}>> {msg}{Color.RESET}")


# ===================== Data Structures =====================
@dataclass
class Package:
    name: str
    source_directory: str
    build_directory: str
    prefix_directory: str
    options: str

# ===================== Helpers =====================
def run_cmd(command: str):
    try:
        Logger.command(command)
        subprocess.run(command, shell=True, check=True, text=True)
    except subprocess.CalledProcessError as e:
        Logger.error(f"Command failed: {e}")
        sys.exit(1)

def check_cmake():
    try:
        result = subprocess.run(["cmake", "--version"], capture_output=True, text=True, check=True)
        Logger.success(f"CMake version: {result.stdout.strip()}")
    except FileNotFoundError:
        Logger.error("CMake not found in PATH.  Make sure you have CMake installed and in your PATH variable set.")
        sys.exit(1)

def detect_generator():
    Logger.info("Detecting available generators...")
    preferred = ["Ninja", "NMake Makefiles", "Unix Makefiles"]
    found = {}

    commands = {
        "Ninja": ["ninja", "--version"],
        "NMake Makefiles": ["nmake", "/?"],
        "Unix Makefiles": ["make", "--version"]
    }

    for name, cmd in commands.items():
        try:
            subprocess.run(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=True)
            Logger.success(f" -- Found generator: {name}")
            found[name] = True
        except Exception:
            pass

    for gen in preferred:
        if gen in found:
            return gen

    # Fallback to first listed in cmake help
    try:
        output = subprocess.check_output(["cmake", "--help"], text=True)
        for line in output.splitlines():
            if "Visual Studio" in line and line.strip().startswith("*"):
                return line.split("=")[0].strip("* ")
    except Exception:
        pass

    return None

def generate_presets(dir_path: str, build_type: str, generator: str, packages: list[Package]):
    def hash_data(data):
        return hashlib.md5(json.dumps(data, indent=2).encode()).hexdigest()

    def get_platform_flags():
        system = platform.system()
        if system == "Windows":
            return {
                "DEBUG": "/ZI",
                "RELEASE": "/O2",
                "RELWITHDEBINFO": "/O2 /Zi",
                "MINSIZEREL": "/O1"
            }
        else:
            return {
                "DEBUG": "-g -O0",
                "RELEASE": "-O3",
                "RELWITHDEBINFO": "-O2 -g",
                "MINSIZEREL": "-Os"
            }

    def common_vars():
        flags = get_platform_flags()
        system_name = platform.system()
        system_version = platform.release()
        system_processor = platform.machine()
        system_arch = platform.architecture()[0]
        system_vendor = platform.processor() or "unknown"

        # Join prefix paths
        prefix_path = str()
        if platform.system() != "Windows":
            prefix_path = ":".join(os.path.abspath(pkg.prefix_directory ) for pkg in packages)
        else:
            prefix_path = ";".join(os.path.abspath(pkg.prefix_directory) for pkg in packages)

        return {
            # C++ Standard
            "CMAKE_CXX_STANDARD": "20",
            "CMAKE_CXX_STANDARD_REQUIRED": "ON",
            "CMAKE_CXX_EXTENSIONS": "OFF",

            # C Standard
            "CMAKE_C_STANDARD": "17",
            "CMAKE_C_STANDARD_REQUIRED": "ON",
            "CMAKE_C_EXTENSIONS": "OFF",

            # Build Behavior
            "CMAKE_EXPORT_COMPILE_COMMANDS": "ON",
            "CMAKE_POSITION_INDEPENDENT_CODE": "ON",

            # Prefix and Find Paths
            "CMAKE_PREFIX_PATH": prefix_path,
            "CMAKE_INSTALL_PREFIX": prefix_path,
            "CMAKE_FIND_ROOT_PATH": prefix_path,

            # Debug/Release Flags
            "CMAKE_CXX_FLAGS_DEBUG": flags["DEBUG"],
            "CMAKE_CXX_FLAGS_RELEASE": flags["RELEASE"],
            "CMAKE_CXX_FLAGS_RELWITHDEBINFO": flags["RELWITHDEBINFO"],
            "CMAKE_CXX_FLAGS_MINSIZEREL": flags["MINSIZEREL"],
            "CMAKE_C_FLAGS_DEBUG": flags["DEBUG"],
            "CMAKE_C_FLAGS_RELEASE": flags["RELEASE"],
            "CMAKE_C_FLAGS_RELWITHDEBINFO": flags["RELWITHDEBINFO"],
            "CMAKE_C_FLAGS_MINSIZEREL": flags["MINSIZEREL"],

            # System Info
            "CMAKE_SYSTEM_NAME": system_name,
            "CMAKE_SYSTEM_VERSION": system_version,
            "CMAKE_SYSTEM_PROCESSOR": system_processor,
            "CMAKE_SYSTEM_PROCESSOR_ARCHITECTURE": system_arch,
            "CMAKE_SYSTEM_PROCESSOR_ARCHITECTURE_VENDOR": system_vendor,

            # Common Flags
            "BUILD_SHARED_LIBS": "OFF",
            "CMAKE_BUILD_TYPE": build_type,
            "CMAKE_COMPILE_COMMANDS": "ON",
            "CMAKE_VERBOSE_MAKEFILE": "ON",
        }

    configure_presets = [
        {
            "name": "base",
            "hidden": True,
            "generator": generator,
            "binaryDir": "${sourceDir}/build",
            "cacheVariables": common_vars()
        },
        {
            "name": "Debug-x64",
            "inherits": "base",
            "displayName": "Debug x64",
            "description": "Debug configuration for x64",
            "architecture": { "value": "x86_64", "strategy": "external" },
            "cacheVariables": {
                "CMAKE_BUILD_TYPE": "Debug"
            },
            "binaryDir": "${sourceDir}/build/Debug-x64"
        },
        {
            "name": "RelWithDebInfo-x64",
            "inherits": "base",
            "displayName": "RelWithDebInfo x64",
            "description": "RelWithDebInfo configuration for x64",
            "architecture": { "value": "x86_64", "strategy": "external" },
            "cacheVariables": {
                "CMAKE_BUILD_TYPE": "RelWithDebInfo"
            },
            "binaryDir": "${sourceDir}/build/RelWithDebInfo-x64"
        },
        {
            "name": "Release-x64",
            "inherits": "base",
            "displayName": "Release x64",
            "description": "Release configuration for x64",
            "architecture": { "value": "x86_64", "strategy": "external" },
            "cacheVariables": {
                "CMAKE_BUILD_TYPE": "Release"
            },
            "binaryDir": "${sourceDir}/build/Release-x64"
        },
        {
            "name": "MinSizeRel-x64",
            "inherits": "base",
            "displayName": "MinSizeRel x64",
            "description": "MinSizeRel configuration for x64",
            "architecture": { "value": "x86_64", "strategy": "external" },
            "cacheVariables": {
                "CMAKE_BUILD_TYPE": "MinSizeRel"
            },
            "binaryDir": "${sourceDir}/build/MinSizeRel-x64"
        }
    ]

    build_presets = [
        {
            "name": "base",
            "hidden": True,
            "configurePreset": "base",
            "jobs": os.cpu_count() or 1,
            "cleanFirst": True
        },
        {
            "name": "Debug-x64",
            "inherits": "base",
            "configurePreset": "Debug-x64"
        },
        {
            "name": "RelWithDebInfo-x64",
            "inherits": "base",
            "configurePreset": "RelWithDebInfo-x64"
        },
        {
            "name": "Release-x64",
            "inherits": "base",
            "configurePreset": "Release-x64"
        },
        {
            "name": "MinSizeRel-x64",
            "inherits": "base",
            "configurePreset": "MinSizeRel-x64"
        }
    ]

    test_presets = [
        {
            "name": "base",
            "hidden": True,
            "configurePreset": "base",
            "execution": {
                "noTestsAction": "error",
                "stopOnFailure": False
            },
            "output": {
                "outputOnFailure": True
            }
        },
        {
            "name": "Debug-x64",
            "inherits": "base",
            "configurePreset": "Debug-x64"
        },
        {
            "name": "RelWithDebInfo-x64",
            "inherits": "base",
            "configurePreset": "RelWithDebInfo-x64"
        },
        {
            "name": "Release-x64",
            "inherits": "base",
            "configurePreset": "Release-x64"
        },
        {
            "name": "MinSizeRel-x64",
            "inherits": "base",
            "configurePreset": "MinSizeRel-x64"
        }
    ]

    root = {
        "version": 3,
        "cmakeMinimumRequired": {"major": 3, "minor": 24, "patch": 0},
        "configurePresets": configure_presets,
        "buildPresets": build_presets,
        "testPresets": test_presets
    }

    file_path = os.path.join(dir_path, "CMakePresets.json")

    if os.path.exists(file_path):
        with open(file_path, "r") as f:
            old = json.load(f)
        if hash_data(old) == hash_data(root):
            Logger.info("CMakePresets.json is up-to-date.")
            return

    with open(file_path, "w") as f:
        json.dump(root, f, indent=2)
    Logger.success(f"CMakePresets.json created at {file_path}")


# ===================== Main =====================
def main():
    check_cmake()

    packages = [
        Package("glfw", "libs/glfw", "libs/build/config/glfw", "build/packages/glfw", "-DGLFW_BUILD_EXAMPLES=OFF -DGLFW_BUILD_TESTS=OFF -DGLFW_BUILD_DOCS=OFF"),
        Package("spdlog", "libs/spdlog", "libs/build/config/spdlog", "build/packages/spdlog", "-DSPDLOG_BUILD_EXAMPLES=OFF"),
        Package("glad", "libs/glad", "libs/build/config/glad", "build/packages/glad", ""),
        Package("glm", "libs/glm", "libs/build/config/glm", "build/packages/glm", "-DGLM_BUILD_TESTS=OFF"),
        Package("SOIL2", "libs/SOIL2", "libs/build/config/SOIL2", "build/packages/SOIL2", ""),
        Package("imgui", "libs/imgui_docking", "libs/build/config/imgui", "build/packages/imgui", ""),
        Package("entt", "libs/entt", "libs/build/config/entt", "build/packages/entt", "-DENTT_INCLUDE_HEADERS=ON -DENTT_INCLUDE_NATVIS=ON -DENTT_INSTALL=ON"),
        Package("assimp", "libs/assimp", "libs/build/config/assimp", "build/packages/assimp", "-DASSIMP_BUILD_TESTS=OFF"),
        Package("yaml-cpp", "libs/yaml-cpp", "libs/build/config/yaml-cpp", "build/packages/yaml-cpp", "-DYAML_BUILD_SHARED_LIBS=OFF")
    ]

    parser = argparse.ArgumentParser(description="Motion Engine Build Script")
    parser.add_argument("--config", choices=["Debug", "Release", "RelWithDebInfo", "MinSizeRel"], help="packages build configuration")
    parser.add_argument("--pkg", help="Specific package to build", choices=[pkg.name for pkg in packages])
    parser.add_argument("--presets", action="store_true", help="Generate CMakePresets.json and exit")
    args = parser.parse_args()

    Logger.info("==========================================")
    Logger.info("     Motion Engine Build Script v1.0.0    ")
    Logger.info("==========================================")
    Logger.info(f"Build Configuration: {args.config}")
    Logger.info(f"Build Packages : {', '.join([pkg.name for pkg in packages])}")
    Logger.info(f"System Name: {platform.system()}")
    Logger.info("==========================================")

    generator = detect_generator()
    if not generator:
        Logger.error("No CMake generator found.")
        sys.exit(1)

    Logger.info(f"Using generator: {generator}")

    if args.presets:
        generate_presets(".", args.config, generator, packages)
        Logger.success("Presets generated.")
        sys.exit(0)

    build_list = []
    if args.pkg:
        selected = [pkg for pkg in packages if pkg.name.lower() == args.pkg.lower()]
        if not selected:
            Logger.error(f"Package '{args.pkg}' not found.")
            sys.exit(1)
        build_list = selected
    else:
        build_list = packages

    prefix_path = str()
    if platform.system() != "Windows":
        prefix_path = ":".join(os.path.abspath(pkg.prefix_directory ) for pkg in build_list)
    else:
        prefix_path = ";".join(os.path.abspath(pkg.prefix_directory) for pkg in build_list)   


    for pkg in build_list:
        Logger.info(f"Building: {Color.BOLD}{pkg.name}{Color.RESET}")
        run_cmd(f"cmake -DCMAKE_BUILD_TYPE=\"{args.config}\" -DCMAKE_SYSTEM_NAME=\"{platform.system()}\" -DCMAKE_PREFIX_PATH=\"{prefix_path}\" -DCMAKE_INSTALL_PREFIX=\"{prefix_path}\" {pkg.options} -S \"{pkg.source_directory}\" -B \"{pkg.build_directory}\" -G \"{generator}\"")
        run_cmd(f"cmake --build \"{pkg.build_directory}\" --config \"{args.config}\"")
        run_cmd(f"cmake --install \"{pkg.build_directory}\" --config \"{args.config}\" --prefix \"{pkg.prefix_directory}\"")

    Logger.success("\U0001F389 Build completed successfully!")


if __name__ == "__main__":
    main()
    