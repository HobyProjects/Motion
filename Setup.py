#!/usr/bin/env python3

"""
Motion Engine Build Script
Version: 1.0.5
"""

import os
import sys
import json
import argparse
import subprocess
import hashlib
import platform
import shutil
import tempfile
import textwrap
from pathlib import Path
from dataclasses import dataclass

# ===================== Logger =====================
class Color:
    RESET = "\033[0m"
    RED = "\033[91m"
    GREEN = "\033[92m"
    YELLOW = "\033[93m"
    CYAN = "\033[96m"
    BOLD = "\033[1m"

class Logger:
    @staticmethod
    def _log(prefix, color, msg):
        print(f"{color}{prefix}{Color.RESET} {msg}")

    @staticmethod
    def info(msg): Logger._log("[INFO]", Color.CYAN, msg)
    @staticmethod
    def success(msg): Logger._log("[OK]", Color.GREEN, msg)
    @staticmethod
    def warn(msg): Logger._log("[WARN]", Color.YELLOW, msg)
    @staticmethod
    def error(msg): Logger._log("[ERROR]", Color.RED, msg)
    @staticmethod
    def command(msg): print(f"{Color.BOLD}{Color.CYAN}>> {msg}{Color.RESET}")


# ===================== Data Structures =====================
@dataclass
class Package:
    name: str
    source_directory: str
    build_directory: str
    prefix_directory: str
    options: str = ""


# ===================== Shell Helpers =====================
def run_cmd(command: str):
    Logger.command(command)
    try:
        subprocess.run(command, shell=True, check=True, text=True)
    except subprocess.CalledProcessError as e:
        Logger.error(f"Command failed: {e}")
        sys.exit(1)

def check_cmake():
    try:
        result = subprocess.run(["cmake", "--version"], capture_output=True, text=True, check=True)
        Logger.success(f"CMake version: {result.stdout.strip()}")
    except FileNotFoundError:
        Logger.error("CMake not found. Install it and ensure it's in your PATH.")
        sys.exit(1)

def _tool_exists(tool: str) -> bool:
    return shutil.which(tool) is not None

def _mkdirp(p: str) -> None:
    Path(p).mkdir(parents=True, exist_ok=True)

def _write_json(path: str, data: dict) -> None:
    _mkdirp(os.path.dirname(path))
    with open(path, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2)
    Logger.success(f"Wrote {path}")


# ===================== Presets =====================
def generate_presets(dir_path: str, packages: list[Package]):
    def hash_data(data): 
        return hashlib.md5(json.dumps(data, indent=2).encode()).hexdigest()

    def common_vars():
        system = platform.system()
        sep = ";" if system == "Windows" else ":"
        prefix_path = sep.join(os.path.abspath(pkg.prefix_directory) for pkg in packages)

        return {
            # Language standards
            "CMAKE_CXX_STANDARD": "20",
            "CMAKE_CXX_STANDARD_REQUIRED": "ON",
            "CMAKE_CXX_EXTENSIONS": "OFF",
            "CMAKE_C_STANDARD": "17",
            "CMAKE_C_STANDARD_REQUIRED": "ON",
            "CMAKE_C_EXTENSIONS": "OFF",

            # Paths
            "CMAKE_PREFIX_PATH": prefix_path,
            "CMAKE_INSTALL_PREFIX": prefix_path,
            "CMAKE_EXPORT_COMPILE_COMMANDS" : "ON"
        }

    def preset(name, cfg):
        return {
            "name": f"{name}-x64",
            "inherits": "base",
            "displayName": f"{name} x64",
            "description": f"{name} configuration for x64",
            "architecture": {"value": "x86_64", "strategy": "external"},
            "binaryDir": f"${{sourceDir}}/build/{name}-x64"
        }

    base = {
        "name": "base",
        "hidden": True,
        "binaryDir": "${sourceDir}/build",
        "cacheVariables": common_vars()
    }

    root = {
        "version": 3,
        "cmakeMinimumRequired": {"major": 3, "minor": 24, "patch": 0},
        "configurePresets": [
            base,
            preset("Debug", "Debug"),
            preset("RelWithDebInfo", "RelWithDebInfo"),
            preset("Release", "Release"),
            preset("MinSizeRel", "MinSizeRel"),
        ],
        "buildPresets": [
            {
                "name": "base",
                "hidden": True,
                "configurePreset": "base",
                "jobs": os.cpu_count() or 1,
                "cleanFirst": True
            },
            *[
                {
                    "name": f"{cfg}-x64",
                    "inherits": "base",
                    "configurePreset": f"{cfg}-x64"
                }
                for cfg in ["Debug", "RelWithDebInfo", "Release", "MinSizeRel"]
            ]
        ],
        "testPresets": [
            {
                "name": "base",
                "hidden": True,
                "configurePreset": "base",
                "execution": {"noTestsAction": "error", "stopOnFailure": False},
                "output": {"outputOnFailure": True}
            },
            *[
                {
                    "name": f"{cfg}-x64",
                    "inherits": "base",
                    "configurePreset": f"{cfg}-x64"
                }
                for cfg in ["Debug", "RelWithDebInfo", "Release", "MinSizeRel"]
            ]
        ]
    }

    file_path = os.path.join(dir_path, "CMakePresets.json")
    if os.path.exists(file_path):
        with open(file_path, "r") as f:
            old = json.load(f)
        if hash_data(old) == hash_data(root):
            Logger.info("CMakePresets.json is up-to-date.")
            return
    _write_json(file_path, root)

# ===================== Main =====================
def main():
    check_cmake()

    packages = [
        Package("glfw", "libs/glfw", "build/vendors/glfw", "build/packages/glfw", "-DGLFW_BUILD_EXAMPLES=OFF -DGLFW_BUILD_TESTS=OFF -DGLFW_BUILD_DOCS=OFF"),
        Package("spdlog", "libs/spdlog", "build/vendors/spdlog", "build/packages/spdlog", "-DSPDLOG_BUILD_EXAMPLE=OFF"),
        Package("glad", "libs/glad", "build/vendors/glad", "build/packages/glad"),
        Package("glm", "libs/glm", "build/vendors/glm", "build/packages/glm", "-DGLM_BUILD_TESTS=OFF"),
        Package("imgui", "libs/imgui_docking", "build/vendors/imgui", "build/packages/imgui"),
        Package("entt", "libs/entt", "build/vendors/entt", "build/packages/entt", "-DENTT_INCLUDE_HEADERS=ON -DENTT_INCLUDE_NATVIS=ON -DENTT_INSTALL=ON"),
        Package("assimp", "libs/assimp", "build/vendors/assimp", "build/packages/assimp", "-DASSIMP_BUILD_TESTS=OFF"),
        Package("stb_image", "libs/stb_image", "build/vendors/stb_image", "build/packages/stb_image"),     
        Package("yaml-cpp", "libs/yaml-cpp", "build/vendors/yaml-cpp", "build/packages/yaml-cpp", "-DYAML_BUILD_SHARED_LIBS=OFF"),
        Package("MikkTSpace", "libs/MikkTSpace", "build/vendors/MikkTSpace", "build/packages/MikkTSpace"),
        Package("imguizmo", "libs/imguizmo", "build/vendors/imguizmo", "build/packages/imguizmo"),
        
        Package("ReactPhysics3D", "libs/reactphysics3d", "build/vendors/reactphysics3d", "build/packages/ReactPhysics3D", 
                "-DRP3D_COMPILE_LIBRARY=ON " \
                "-DRP3D_COMPILE_TESTBED=OFF " \
                "-DRP3D_COMPILE_TESTS=OFF " \
                "-DRP3D_PROFILING_ENABLED=OFF " \
                "-DRP3D_GENERATE_DOCUMENTATION=OFF " \
                "-DRP3D_CODE_COVERAGE_ENABLED=OFF " \
                "-DRP3D_DOUBLE_PRECISION_ENABLED=OFF"),
    ]

    parser = argparse.ArgumentParser(description="Motion Engine Build Script")
    parser.add_argument("--config", choices=["Debug", "Release", "RelWithDebInfo", "MinSizeRel"], required=True, help="Build configuration")
    parser.add_argument("--pkg", help="Specific package", choices=[pkg.name for pkg in packages])
    parser.add_argument("--presets", action="store_true", help="Generate CMakePresets.json and exit")
    args = parser.parse_args()

    Logger.info("=" * 42)
    Logger.info("     Motion Engine Build Script v1.0.5    ")
    Logger.info("=" * 42)
    Logger.info(f"Build Configuration: {args.config}")
    Logger.info(f"System: {platform.system()}")
    Logger.info(f"Packages: {', '.join(pkg.name for pkg in packages)}")
    Logger.info("=" * 42)

    if args.presets:
        generate_presets(".", packages)
        Logger.success("Presets generated.")
        return

    # Build list
    build_list = [pkg for pkg in packages if args.pkg and pkg.name.lower() == args.pkg.lower()] or packages

    # Prefix path aggregation
    sep = ";" if platform.system() == "Windows" else ":"
    prefix_path = sep.join(os.path.abspath(pkg.prefix_directory) for pkg in build_list)

    # Build/install loop
    for pkg in build_list:
        Logger.info(f"Building: {Color.BOLD}{pkg.name}{Color.RESET}")
        run_cmd(
            f'cmake --fresh -DCMAKE_BUILD_TYPE="{args.config}" '
            f'-DCMAKE_SYSTEM_NAME="{platform.system()}" '
            f'-DCMAKE_PREFIX_PATH="{prefix_path}" '
            f'-DCMAKE_INSTALL_PREFIX="{prefix_path}" '
            f'{pkg.options} -S "{pkg.source_directory}" -B "{pkg.build_directory}"'
        )
        run_cmd(f'cmake --build "{pkg.build_directory}" --config "{args.config}"')
        run_cmd(f'cmake --install "{pkg.build_directory}" --config "{args.config}" --prefix "{pkg.prefix_directory}"')

    Logger.success("🎉 Build completed successfully!")


if __name__ == "__main__":
    main()