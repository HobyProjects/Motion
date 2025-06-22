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
    """
    ANSI escape code constants for colored logging.
    """
    RESET = "\033[0m"           # Reset text color
    RED = "\033[91m"            # Red text color
    GREEN = "\033[92m"          # Green text color
    YELLOW = "\033[93m"         # Yellow text color
    CYAN = "\033[96m"           # Cyan text color
    BOLD = "\033[1m"            # Bold text style

class Logger:
    """Colored logger class for pretty-printing messages.

    Attributes:
        info (staticmethod): Info message
        success (staticmethod): Success message
        warn (staticmethod): Warning message
        error (staticmethod): Error message
        command (staticmethod): Command message
    """
    @staticmethod
    def info(msg):
        """Prints an info message.

        Args:
            msg (str): Message to print.
        """
        print(f"{Color.CYAN}[INFO]{Color.RESET} {msg}")

    @staticmethod
    def success(msg):
        """Prints a success message.

        Args:
            msg (str): Message to print.
        """
        print(f"{Color.GREEN}[OK]{Color.RESET} {msg}")

    @staticmethod
    def warn(msg):
        """Prints a warning message.

        Args:
            msg (str): Message to print.
        """
        print(f"{Color.YELLOW}[WARN]{Color.RESET} {msg}")

    @staticmethod
    def error(msg):
        """Prints an error message.

        Args:
            msg (str): Message to print.
        """
        print(f"{Color.RED}[ERROR]{Color.RESET} {msg}")

    @staticmethod
    def command(msg):
        """Prints a command message.

        Args:
            msg (str): Message to print.
        """
        print(f"{Color.BOLD}{Color.CYAN}>> {msg}{Color.RESET}")


# ===================== Data Structures =====================
@dataclass
class Package:
    """Represents a package to be built.

    Attributes:
        name: The name of the package.
        source_directory: The directory containing the package source code.
        build_directory: The directory where the package build will be stored.
        prefix_directory: The directory where the package will be installed.
        options: Additional build options.
    """
    name: str
    source_directory: str
    build_directory: str
    prefix_directory: str
    options: str

# ===================== Helpers =====================
def run_cmd(command: str):
    """Runs a shell command and logs the output.

    Args:
        command (str): The command to run.

    Raises:
        subprocess.CalledProcessError: If the command fails.
    """
    try:
        Logger.command(command)
        subprocess.run(command, shell=True, check=True, text=True)
    except subprocess.CalledProcessError as e:
        Logger.error(f"Command failed: {e}")
        sys.exit(1)

def check_cmake():
    """
    Checks if CMake is installed and logs its version.

    This function attempts to run the 'cmake --version' command to verify
    the presence of CMake in the system's PATH. If CMake is found, it logs
    the version information. If CMake is not found, it logs an error and
    terminates the program.

    Raises:
        SystemExit: If CMake is not found or an error occurs during execution.
    """

    try:
        result = subprocess.run(["cmake", "--version"], capture_output=True, text=True, check=True)
        Logger.success(f"CMake version: {result.stdout.strip()}")
    except FileNotFoundError:
        Logger.error("CMake not found in PATH.")
        sys.exit(1)

def detect_generator():
    """Detects available CMake generators and returns the most preferred one.

    This function runs each generator command with the '--version' or '/?' flag to
    detect if the generator is installed in the system's PATH. It logs the generators
    that are found and returns the most preferred one based on the following order:
    Ninja, NMake Makefiles, Unix Makefiles.

    If none of the preferred generators are found, it falls back to the first one
    listed in the CMake help output. If no generator is found, it returns None.

    Returns:
        str: The name of the most preferred available generator, or None if none is found.
    """
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
    """
    Generates a CMakePresets.json file based on the given list of packages and the
    preferred generator.

    The generated file contains the following presets:

    - 8 configure presets (base, Debug-x64, Debug-x86, RelWithDebInfo-x64, RelWithDebInfo-x86, Release-x64, Release-x86, MinSizeRel-x64, MinSizeRel-x86)
    - 8 build presets (base, Debug-x64, Debug-x86, RelWithDebInfo-x64, RelWithDebInfo-x86, Release-x64, Release-x86, MinSizeRel-x64, MinSizeRel-x86)
    - 8 test presets (base, Debug-x64, Debug-x86, RelWithDebInfo-x64, RelWithDebInfo-x86, Release-x64, Release-x86, MinSizeRel-x64, MinSizeRel-x86)

    Each preset inherits from the base preset and has the following settings:

    - The appropriate build type (Debug, Release, RelWithDebInfo, MinSizeRel)
    - The appropriate architecture (x64, x86)
    - The appropriate CMAKE_PREFIX_PATH, CMAKE_INSTALL_PREFIX, and CMAKE_FIND_ROOT_PATH
    - The appropriate CMAKE_CXX_FLAGS and CMAKE_C_FLAGS
    - The appropriate CMAKE_VERBOSE_MAKEFILE and CMAKE_SUPPRESS_REGENERATION

    The generated file is written to the current working directory and is named
    "CMakePresets.json". If the file already exists, it is overwritten only if the
    contents are different.

    :param packages: A list of packages to generate the presets for.
    :type packages: list[Package]
    :param generator: The preferred generator to use.
    :type generator: str
    """
    def hash_data(data):
        """
        Returns a hash of the given data.

        This function takes a dictionary or other JSON-serializable data structure
        and returns a hexadecimal string representation of its MD5 hash.

        :param data: The data to hash
        :return: A hexadecimal string representation of the MD5 hash of the data
        :rtype: str
        """
        return hashlib.md5(json.dumps(data, indent=2).encode()).hexdigest()

    def get_platform_flags():
        """
        Returns a dictionary of platform-specific CMake flags for the given configuration type.

        On Windows, the dictionary is:
        {
            "DEBUG": "/Zi /Ob0 /Od /RTC1",
            "RELEASE": "/O2",
            "RELWITHDEBINFO": "/O2 /Zi",
            "MINSIZEREL": "/O1"
        }

        On other platforms, the dictionary is:
        {
            "DEBUG": "-g -O0",
            "RELEASE": "-O3",
            "RELWITHDEBINFO": "-O2 -g",
            "MINSIZEREL": "-Os"
        }

        :return: A dictionary of platform-specific CMake flags
        :rtype: dict
        """
        system = platform.system()
        if system == "Windows":
            return {
                "DEBUG": "/Zi /Ob0 /Od /RTC1",
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
        """
        Returns a dictionary of common CMake variables that are used across all
        build configurations.

        These variables are:
        - C++ Standard: C++20 with no extensions
        - C Standard: C17 with no extensions
        - Build Behavior:
            - Export compile commands
            - Build with Position Independent Code (PIC)
        - Prefix and Find Paths: CMAKE_PREFIX_PATH, CMAKE_INSTALL_PREFIX, and
            CMAKE_FIND_ROOT_PATH are all set to the same prefix path
        - Debug/Release Flags: Debug, Release, RelWithDebInfo, and MinSizeRel flags
            are set with sensible defaults
        - System Info: CMAKE_SYSTEM_NAME, CMAKE_SYSTEM_VERSION, CMAKE_SYSTEM_PROCESSOR,
            CMAKE_SYSTEM_PROCESSOR_ARCHITECTURE, and CMAKE_SYSTEM_PROCESSOR_ARCHITECTURE_VENDOR
            are set with information about the build system
        - Common Flags: CMAKE_VERBOSE_MAKEFILE, BUILD_SHARED_LIBS, and
            CMAKE_SUPPRESS_REGENERATION are set with sensible defaults

        :return: A dictionary of common CMake variables
        """
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
            "CMAKE_VERBOSE_MAKEFILE": "ON",
            "BUILD_SHARED_LIBS": "OFF",
            "CMAKE_SUPPRESS_REGENERATION": "ON",
            "CMAKE_COLOR_MAKEFILE": "ON",
            "CMAKE_BUILD_TYPE_INIT": build_type
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
            "name": "Debug-x86",
            "inherits": "base",
            "displayName": "Debug x86",
            "description": "Debug configuration for x86",
            "architecture": { "value": "x86", "strategy": "external" },
            "cacheVariables": {
                "CMAKE_BUILD_TYPE": "Debug"
            },
            "binaryDir": "${sourceDir}/build/Debug-x86"
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
            "name": "RelWithDebInfo-x86",
            "inherits": "base",
            "displayName": "RelWithDebInfo x86",
            "description": "RelWithDebInfo configuration for x86",
            "architecture": { "value": "x86", "strategy": "external" },
            "cacheVariables": {
                "CMAKE_BUILD_TYPE": "RelWithDebInfo"
            },
            "binaryDir": "${sourceDir}/build/RelWithDebInfo-x86"
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
            "name": "Release-x86",
            "inherits": "base",
            "displayName": "Release x86",
            "description": "Release configuration for x86",
            "architecture": { "value": "x86", "strategy": "external" },
            "cacheVariables": {
                "CMAKE_BUILD_TYPE": "Release"
            },
            "binaryDir": "${sourceDir}/build/Release-x86"
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
        },
        {
            "name": "MinSizeRel-x86",
            "inherits": "base",
            "displayName": "MinSizeRel x86",
            "description": "MinSizeRel configuration for x86",
            "architecture": { "value": "x86", "strategy": "external" },
            "cacheVariables": {
                "CMAKE_BUILD_TYPE": "MinSizeRel"
            },
            "binaryDir": "${sourceDir}/build/MinSizeRel-x86"
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
            "name": "Debug-x86",
            "inherits": "base",
            "configurePreset": "Debug-x86"
        },
        {
            "name": "RelWithDebInfo-x64",
            "inherits": "base",
            "configurePreset": "RelWithDebInfo-x64"
        },
        {
            "name": "RelWithDebInfo-x86",
            "inherits": "base",
            "configurePreset": "RelWithDebInfo-x86"
        },
        {
            "name": "Release-x64",
            "inherits": "base",
            "configurePreset": "Release-x64"
        },
        {
            "name": "Release-x86",
            "inherits": "base",
            "configurePreset": "Release-x86"
        },
        {
            "name": "MinSizeRel-x64",
            "inherits": "base",
            "configurePreset": "MinSizeRel-x64"
        },
        {
            "name": "MinSizeRel-x86",
            "inherits": "base",
            "configurePreset": "MinSizeRel-x86"
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
            "name": "Debug-x86",
            "inherits": "base",
            "configurePreset": "Debug-x86"
        },
        {
            "name": "RelWithDebInfo-x64",
            "inherits": "base",
            "configurePreset": "RelWithDebInfo-x64"
        },
        {
            "name": "RelWithDebInfo-x86",
            "inherits": "base",
            "configurePreset": "RelWithDebInfo-x86"
        },
        {
            "name": "Release-x64",
            "inherits": "base",
            "configurePreset": "Release-x64"
        },
        {
            "name": "Release-x86",
            "inherits": "base",
            "configurePreset": "Release-x86"
        },
        {
            "name": "MinSizeRel-x64",
            "inherits": "base",
            "configurePreset": "MinSizeRel-x64"
        },
        {
            "name": "MinSizeRel-x86",
            "inherits": "base",
            "configurePreset": "MinSizeRel-x86"
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
    """
    Main entry point for the build script.

    This script is used to build the Motion Engine and its dependencies.

    It can be used to build a single package or all packages.

    Args:
        --config (str): The build configuration to use (Debug, Release, RelWithDebInfo, MinSizeRel)
        --arch (str): The build architecture to use (x86, x86_64)
        --pkg (str): The package to build (optional)
        --generate-presets (bool): Generate CMakePresets.json and exit (optional)

    Returns:
        int: The exit code of the script
    """
    check_cmake()

    packages = [
        Package("glfw", "glfw", "build/config/glfw", "build/packages/glfw", "-DGLFW_BUILD_EXAMPLES=OFF -DGLFW_BUILD_TESTS=OFF -DGLFW_BUILD_DOCS=OFF"),
        Package("spdlog", "spdlog", "build/config/spdlog", "build/packages/spdlog", "-DSPDLOG_BUILD_EXAMPLES=OFF"),
        Package("glad", "glad", "build/config/glad", "build/packages/glad", ""),
        Package("glm", "glm", "build/config/glm", "build/packages/glm", "-DGLM_BUILD_TESTS=OFF"),
        Package("SOIL2", "SOIL2", "build/config/SOIL2", "build/packages/SOIL2", ""),
        Package("imgui", "imgui_docking", "build/config/imgui", "build/packages/imgui", ""),
        Package("entt", "entt", "build/config/entt", "build/packages/entt", "-DENTT_INCLUDE_HEADERS=ON -DENTT_INCLUDE_NATVIS=ON -DENTT_INSTALL=ON"),
        Package("assimp", "assimp", "build/config/assimp", "build/packages/assimp", "-DASSIMP_BUILD_TESTS=OFF"),
        Package("yaml-cpp", "yaml-cpp", "build/config/yaml-cpp", "build/packages/yaml-cpp", "-DYAML_BUILD_SHARED_LIBS=OFF")
    ]

    parser = argparse.ArgumentParser(description="Motion Engine Build Script")
    parser.add_argument("--config", required=True, choices=["Debug", "Release", "RelWithDebInfo", "MinSizeRel"], help="Build configuration")
    parser.add_argument("--arch", required=True, choices=["x86", "x86_64"], help="Build architecture")
    parser.add_argument("--pkg", help="Specific package to build", choices=[pkg.name for pkg in packages])
    parser.add_argument("--presets", action="store_true", help="Generate CMakePresets.json and exit")
    args = parser.parse_args()

    Logger.info("Motion Engine Build Script v1.0.0")
    Logger.info(f"Build Configuration: {args.config}")
    Logger.info(f"Build Architecture: {args.arch}")
    Logger.info(f"Build Packages : {', '.join([pkg.name for pkg in packages])}")
    Logger.info(f"System Name: {platform.system()}")

    generator = detect_generator()
    if not generator:
        Logger.error("No CMake generator found.")
        sys.exit(1)

    Logger.info(f"Using generator: {generator}")

    if args.presets:
        generate_presets("../", args.config, generator, packages)
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