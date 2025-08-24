#!/usr/bin/env python3
"""
Motion Engine Build Script
Version: 1.0.5

Changes:
- Robust generator selection: we PROBE generators by running a real CMake configure
  on a temporary project. We only use a generator if the probe succeeds.
- Honors CMAKE_GENERATOR env var *only if* it passes the probe; else falls back.
- Generates CMakePresets.json with the chosen generator pinned.
- VS Code settings/tasks/launch generation retained.

Usage examples:
  python Setup.py --config RelWithDebInfo
  python Setup.py --config Debug --vscode
  python Setup.py --config Release --presets
  python Setup.py --config Debug --list-generators
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


# ===================== Generator Probing =====================
def _candidate_generators_by_platform() -> list[str]:
    if platform.system() == "Windows":
        # Probe order: fastest/more portable first
        return [
            "Ninja",
            "MinGW Makefiles",
            "Visual Studio 17 2022",
            "Visual Studio 16 2019",
            "Visual Studio 15 2017",
            "NMake Makefiles",
            "Unix Makefiles",  # MSYS/Make if available
        ]
    else:
        return ["Ninja", "Unix Makefiles"]

def _gen_needs_tool(gen: str) -> str | None:
    if gen == "Ninja":
        return "ninja"
    if gen == "Unix Makefiles":
        return "make"
    if gen == "NMake Makefiles":
        return "nmake"
    if gen == "MinGW Makefiles":
        return "mingw32-make"
    return None  # Visual Studio gens don't map to a single CLI tool here

def _probe_generator(gen: str) -> tuple[bool, str]:
    """
    Try to run a real cmake configure with -G <gen> on a tiny temp project.
    For Visual Studio generators, also pass -A x64 to avoid Win32 default surprises.
    Return (ok, message).
    """
    needed = _gen_needs_tool(gen)
    if needed and not _tool_exists(needed):
        return False, f"Required tool '{needed}' not found on PATH"

    # Minimal project to configure
    cmakelists = textwrap.dedent("""
        cmake_minimum_required(VERSION 3.20)
        project(Probe C CXX)
        add_executable(probe main.cpp)
    """).strip()

    main_cpp = "int main(){return 0;}\n"

    with tempfile.TemporaryDirectory(prefix="cmake-probe-src-") as src, \
         tempfile.TemporaryDirectory(prefix="cmake-probe-bld-") as bld:
        Path(src, "CMakeLists.txt").write_text(cmakelists, encoding="utf-8")
        Path(src, "main.cpp").write_text(main_cpp, encoding="utf-8")

        # Build configure command
        vs_arch = ' -A "x64"' if gen.startswith("Visual Studio") else ""
        cmd = f'cmake -S "{src}" -B "{bld}" -G "{gen}"{vs_arch}'

        try:
            subprocess.run(cmd, shell=True, check=True, text=True,
                           stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            return True, "configure OK"
        except subprocess.CalledProcessError as e:
            return False, f"configure failed ({e})"

def select_generator(probe_only: bool = False) -> tuple[str | None, list[tuple[str, str]]]:
    """
    Choose a working generator by probing:
      1) If CMAKE_GENERATOR is set, probe it first; use if OK.
      2) Else try platform candidates in order; use the first that configures OK.
    Returns (chosen_generator_or_None, probe_log), where probe_log is a list of (generator, result_msg).
    """
    log: list[tuple[str, str]] = []

    env_gen = os.environ.get("CMAKE_GENERATOR")
    if env_gen:
        ok, msg = _probe_generator(env_gen)
        log.append((f"CMAKE_GENERATOR={env_gen}", msg))
        if ok:
            return env_gen, log
        # continue probing

    for gen in _candidate_generators_by_platform():
        ok, msg = _probe_generator(gen)
        log.append((gen, msg))
        if ok:
            return gen, log

    # Nothing worked; return None. Caller can try without -G (CMake's internal default may still succeed).
    return None, log


# ===================== Presets =====================
def generate_presets(dir_path: str, packages: list[Package], generator: str | None):
    def hash_data(data): return hashlib.md5(json.dumps(data, indent=2).encode()).hexdigest()

    def platform_flags():
        if platform.system() == "Windows":
            return {"DEBUG": "/ZI", "RELEASE": "/O2", "RELWITHDEBINFO": "/O2 /Zi", "MINSIZEREL": "/O1"}
        return {"DEBUG": "-g -O0", "RELEASE": "-O3", "RELWITHDEBINFO": "-O2 -g", "MINSIZEREL": "-Os"}

    def common_vars():
        flags = platform_flags()
        system = platform.system()
        sep = ";" if system == "Windows" else ":"
        prefix_path = sep.join(os.path.abspath(pkg.prefix_directory) for pkg in packages)

        return {
            "CMAKE_CXX_STANDARD": "20", 
            "CMAKE_CXX_STANDARD_REQUIRED": "ON", 
            "CMAKE_CXX_EXTENSIONS": "OFF",
            "CMAKE_C_STANDARD": "17", 
            "CMAKE_C_STANDARD_REQUIRED": "ON",
            "CMAKE_C_EXTENSIONS": "OFF",
            "CMAKE_PREFIX_PATH": prefix_path,
            "CMAKE_INSTALL_PREFIX": prefix_path,
            "CMAKE_CXX_FLAGS_DEBUG": flags["DEBUG"], 
            "CMAKE_CXX_FLAGS_RELEASE": flags["RELEASE"],
            "CMAKE_CXX_FLAGS_RELWITHDEBINFO": flags["RELWITHDEBINFO"], 
            "CMAKE_CXX_FLAGS_MINSIZEREL": flags["MINSIZEREL"],
            "CMAKE_C_FLAGS_DEBUG": flags["DEBUG"], 
            "CMAKE_C_FLAGS_RELEASE": flags["RELEASE"],
            "CMAKE_C_FLAGS_RELWITHDEBINFO": flags["RELWITHDEBINFO"], 
            "CMAKE_C_FLAGS_MINSIZEREL": flags["MINSIZEREL"],
            "CMAKE_SYSTEM_NAME": system, 
            "CMAKE_SYSTEM_VERSION": platform.release(), 
            "CMAKE_SYSTEM_PROCESSOR": platform.machine(),
            "CMAKE_GENERATOR": generator
        }

    def preset(name, cfg):
        return {
            "name": f"{name}-x64", 
            "inherits": "base", 
            "displayName": f"{name} x64",
            "description": f"{name} configuration for x64",
            "architecture": {"value": "x86_64", "strategy": "external"},
            "cacheVariables": {"CMAKE_BUILD_TYPE": cfg},
            "binaryDir": f"${{sourceDir}}/build/{name}-x64"
        }

    base = {
        "name": "base",
        "hidden": True,
        "binaryDir": "${sourceDir}/build",
        "cacheVariables": common_vars()
    }
    if generator:
        base["generator"] = generator

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
            {"name": "base", "hidden": True, "configurePreset": "base", "jobs": os.cpu_count() or 1, "cleanFirst": True},
            *[{"name": f"{cfg}-x64", "inherits": "base", "configurePreset": f"{cfg}-x64"}
              for cfg in ["Debug", "RelWithDebInfo", "Release", "MinSizeRel"]]
        ],
        "testPresets": [
            {"name": "base", "hidden": True, "configurePreset": "base",
             "execution": {"noTestsAction": "error", "stopOnFailure": False},
             "output": {"outputOnFailure": True}},
            *[{"name": f"{cfg}-x64", "inherits": "base", "configurePreset": f"{cfg}-x64"}
              for cfg in ["Debug", "RelWithDebInfo", "Release", "MinSizeRel"]]
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


# ===================== VS Code =====================
def generate_vscode_files(packages: list[Package]):
    workspace = os.getcwd()
    vscode_dir = os.path.join(workspace, ".vscode")

    settings = {
        "cmake.buildDirectory": "${workspaceFolder}/build/${buildKit}-${buildType}",
        "C_Cpp.default.cppStandard": "c++20",
        "C_Cpp.default.cStandard": "c17"
    }
    _write_json(os.path.join(vscode_dir, "settings.json"), settings)

    pkg_names = [p.name for p in packages]
    tasks = {
        "version": "2.0.0",
        "tasks": [
            {"label": "ME: Generate CMake Presets", "type": "shell",
             "command": "python", "args": ["${workspaceFolder}/Setup.py", "--config", "${input:cfg}", "--presets"],
             "group": "build", "problemMatcher": []},
            {"label": "ME: Build All Packages", "type": "shell",
             "command": "python", "args": ["${workspaceFolder}/Setup.py", "--config", "${input:cfg}"],
             "group": "build", "problemMatcher": []},
            {"label": "ME: Build Single Package", "type": "shell",
             "command": "python", "args": ["${workspaceFolder}/Setup.py", "--config", "${input:cfg}", "--pkg", "${input:pkg}"],
             "group": "build", "problemMatcher": []}
        ],
        "inputs": [
            {"id": "cfg", "type": "pickString", "description": "Choose CMake configuration",
             "options": ["Debug", "RelWithDebInfo", "Release", "MinSizeRel"], "default": "RelWithDebInfo"},
            {"id": "pkg", "type": "pickString", "description": "Choose package", "options": pkg_names,
             "default": pkg_names[0] if pkg_names else ""}
        ]
    }
    _write_json(os.path.join(vscode_dir, "tasks.json"), tasks)

    system = platform.system()
    if system == "Windows":
        debug_type = "cppvsdbg"
        program_path = "${workspaceFolder}/build/Debug-x64/YourEngine.exe"
    else:
        debug_type = "cppdbg"
        program_path = "${workspaceFolder}/build/Debug-x64/YourEngine"
    launch = {
        "version": "0.2.0",
        "configurations": [
            {
                "name": "Launch Motion Engine",
                "type": debug_type,
                "request": "launch",
                "program": program_path,
                "args": [],
                "cwd": "${workspaceFolder}",
                "environment": [],
                "console": "integratedTerminal",
                **({"MIMode": "gdb"} if system != "Windows" else {})
            }
        ]
    }
    _write_json(os.path.join(vscode_dir, "launch.json"), launch)

    Logger.success("VS Code settings, tasks, and launch configs generated.")

def clean_all_builds(packages):
    # root builds by config flavors
    roots = ["build/Debug-x64", "build/RelWithDebInfo-x64", "build/Release-x64", "build/MinSizeRel-x64", "build"]
    for p in roots:
        if os.path.exists(p): shutil.rmtree(p, ignore_errors=True)
    # per-package builds
    for pkg in packages:
        if os.path.exists(pkg.build_directory):
            shutil.rmtree(pkg.build_directory, ignore_errors=True)

# ===================== Main =====================
def main():
    check_cmake()

    packages = [
        Package("glfw", "libs/glfw", "libs/build/config/glfw", "build/packages/glfw",
                "-DGLFW_BUILD_EXAMPLES=OFF -DGLFW_BUILD_TESTS=OFF -DGLFW_BUILD_DOCS=OFF"),
        Package("spdlog", "libs/spdlog", "libs/build/config/spdlog", "build/packages/spdlog",
                "-DSPDLOG_BUILD_EXAMPLES=OFF"),
        Package("glad", "libs/glad", "libs/build/config/glad", "build/packages/glad"),
        Package("glm", "libs/glm", "libs/build/config/glm", "build/packages/glm", "-DGLM_BUILD_TESTS=OFF"),
        Package("imgui", "libs/imgui_docking", "libs/build/config/imgui", "build/packages/imgui"),
        Package("entt", "libs/entt", "libs/build/config/entt", "build/packages/entt",
                "-DENTT_INCLUDE_HEADERS=ON -DENTT_INCLUDE_NATVIS=ON -DENTT_INSTALL=ON"),
        Package("assimp", "libs/assimp", "libs/build/config/assimp", "build/packages/assimp",
                "-DASSIMP_BUILD_TESTS=OFF"),
        Package("stb_image", "libs/stb_image", "libs/build/config/stb_image", "build/packages/stb_image"),
        Package("yaml-cpp", "libs/yaml-cpp", "libs/build/config/yaml-cpp", "build/packages/yaml-cpp",
                "-DYAML_BUILD_SHARED_LIBS=OFF"),
        Package("fastgltf", "libs/fastgltf", "libs/build/config/fastgltf", "build/packages/fastgltf",
                "-DFASTGLTF_COMPILE_AS_CPP20=ON -DFASTGLTF_ENABLE_KHR_PHYSICS_RIGID_BODIES=ON -DFASTGLTF_ENABLE_KHR_IMPLICIT_SHAPES=ON"),
        Package("MikkTSpace", "libs/MikkTSpace", "libs/build/config/MikkTSpace", "build/packages/MikkTSpace"),
        Package("imguizmo", "libs/imguizmo", "libs/build/config/imguizmo", "build/packages/imguizmo"),
    ]

    parser = argparse.ArgumentParser(description="Motion Engine Build Script")
    parser.add_argument("--config", choices=["Debug", "Release", "RelWithDebInfo", "MinSizeRel"],
                        required=True, help="Build configuration")
    parser.add_argument("--pkg", help="Specific package", choices=[pkg.name for pkg in packages])
    parser.add_argument("--presets", action="store_true", help="Generate CMakePresets.json and exit")
    parser.add_argument("--vscode", action="store_true", help="Generate VS Code config files and exit")
    parser.add_argument("--list-generators", action="store_true", help="Probe and list generators, then exit")
    parser.add_argument("--clean", action="store_true", help="Delete all build trees before configuring")
    args = parser.parse_args()

    Logger.info("=" * 42)
    Logger.info("     Motion Engine Build Script v1.0.5    ")
    Logger.info("=" * 42)
    Logger.info(f"Build Configuration: {args.config}")
    Logger.info(f"System: {platform.system()}")
    Logger.info(f"Packages: {', '.join(pkg.name for pkg in packages)}")
    Logger.info("=" * 42)

    chosen_gen, probe_log = select_generator()
    for name, note in probe_log:
        Logger.info(f"Probe {name}: {note}")
    if chosen_gen:
        Logger.success(f"Using generator: {chosen_gen}")
    else:
        Logger.warn("No working generator found during probe. Proceeding WITHOUT -G; CMake will try its internal default.")

    if args.list_generators:
        return

    if args.vscode:
        generate_vscode_files(packages)
        generate_presets(".", packages, generator=chosen_gen)
        return

    if args.presets:
        generate_presets(".", packages, generator=chosen_gen)
        Logger.success("Presets generated.")
        return
    
    if args.clean:
        Logger.warn("Cleaning all build trees…")
        clean_all_builds(packages)

    # Build list
    build_list = [pkg for pkg in packages if args.pkg and pkg.name.lower() == args.pkg.lower()] or packages

    # Prefix path aggregation
    sep = ";" if platform.system() == "Windows" else ":"
    prefix_path = sep.join(os.path.abspath(pkg.prefix_directory) for pkg in build_list)

    # Build/install loop
    for pkg in build_list:
        Logger.info(f"Building: {Color.BOLD}{pkg.name}{Color.RESET}")
        gen_arg = f' -G "{chosen_gen}"' if chosen_gen else ""
        run_cmd(
            f'cmake --fresh -DCMAKE_BUILD_TYPE="{args.config}" '
            f'-DCMAKE_SYSTEM_NAME="{platform.system()}" '
            f'-DCMAKE_PREFIX_PATH="{prefix_path}" '
            f'-DCMAKE_INSTALL_PREFIX="{prefix_path}" '
            f'{pkg.options} -S "{pkg.source_directory}" -B "{pkg.build_directory}"{gen_arg}'
        )
        run_cmd(f'cmake --build "{pkg.build_directory}" --config "{args.config}"')
        run_cmd(f'cmake --install "{pkg.build_directory}" --config "{args.config}" --prefix "{pkg.prefix_directory}"')

    Logger.success("🎉 Build completed successfully!")


if __name__ == "__main__":
    main()