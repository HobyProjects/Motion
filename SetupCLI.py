#!/usr/bin/env python3

"""
Motion Engine Build System
Version: 2.3.0 - Enhanced CMakePresets Generation
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
import threading
import queue
import time
import re

from pathlib import Path
from dataclasses import dataclass, field, asdict
from typing import List, Dict, Optional, Set, Any
from datetime import datetime, timedelta
from enum import Enum
from abc import ABC, abstractmethod

# Optional dependencies for enhanced features
try:
    import matplotlib  # type: ignore
    matplotlib.use('Agg')
    import matplotlib.pyplot as plt  # type: ignore
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False

try:
    import networkx as nx
    HAS_NETWORKX = True
except ImportError:
    HAS_NETWORKX = False


# ===================== Configuration =====================
class BuildConfig:
    """Central configuration for the build system"""
    VERSION = "2.3.0"
    CACHE_FILE = "build/.build_cache.json"
    STATE_FILE = "build/.build_state.json"
    STATS_FILE = "build/.build_stats.json"
    BENCHMARK_FILE = "build/.build_benchmark.json"
    INCREMENTAL_CACHE = "build/.incremental_cache.json"
    MIN_CMAKE_VERSION = (3, 24, 0)
    
    # Build configurations
    CONFIGS = ["Debug", "Release", "RelWithDebInfo", "MinSizeRel"]
    
    # C++ Standards
    CXX_STANDARD = "20"
    C_STANDARD = "17"


# ===================== Logger =====================
class Color:
    """ANSI color codes for terminal output"""
    RESET = "\033[0m"
    RED = "\033[91m"
    GREEN = "\033[92m"
    YELLOW = "\033[93m"
    CYAN = "\033[96m"
    MAGENTA = "\033[95m"
    BOLD = "\033[1m"
    DIM = "\033[2m"


class LogLevel(Enum):
    """Log levels for different message types"""
    DEBUG = 0
    INFO = 1
    SUCCESS = 2
    WARNING = 3
    ERROR = 4


class Logger:
    """Thread-safe logging system with multiple log levels"""
    
    _lock = threading.Lock()
    _log_level = LogLevel.INFO
    _log_file: Optional[Path] = None
    
    @classmethod
    def set_level(cls, level: LogLevel):
        cls._log_level = level
    
    @classmethod
    def set_log_file(cls, path: Path):
        cls._log_file = path
        cls._log_file.parent.mkdir(parents=True, exist_ok=True)
    
    @classmethod
    def _log(cls, level: LogLevel, prefix: str, color: str, msg: str):
        if level.value < cls._log_level.value:
            return
            
        with cls._lock:
            timestamp = datetime.now().strftime("%H:%M:%S")
            formatted = f"{Color.DIM}[{timestamp}]{Color.RESET} {color}{prefix}{Color.RESET} {msg}"
            print(formatted)
            
            if cls._log_file:
                with open(cls._log_file, "a", encoding="utf-8") as f:
                    f.write(f"[{timestamp}] {prefix} {msg}\n")
    
    @classmethod
    def debug(cls, msg: str):
        cls._log(LogLevel.DEBUG, "[DEBUG]", Color.MAGENTA, msg)
    
    @classmethod
    def info(cls, msg: str):
        cls._log(LogLevel.INFO, "[INFO]", Color.CYAN, msg)
    
    @classmethod
    def success(cls, msg: str):
        cls._log(LogLevel.SUCCESS, "[OK]", Color.GREEN, msg)
    
    @classmethod
    def warn(cls, msg: str):
        cls._log(LogLevel.WARNING, "[WARN]", Color.YELLOW, msg)
    
    @classmethod
    def error(cls, msg: str):
        cls._log(LogLevel.ERROR, "[ERROR]", Color.RED, msg)
    
    @classmethod
    def command(cls, msg: str):
        with cls._lock:
            print(f"{Color.BOLD}{Color.CYAN}>> {msg}{Color.RESET}")


# ===================== Exceptions =====================
class BuildException(Exception):
    """Base exception for build system errors"""
    pass


class DependencyException(BuildException):
    """Exception for dependency-related errors"""
    pass


class ConfigurationException(BuildException):
    """Exception for configuration errors"""
    pass


# ===================== Data Models =====================
@dataclass
class Package:
    """Package definition with build configuration"""
    name: str
    source_directory: str
    build_directory: str
    prefix_directory: str
    options: str = ""
    dependencies: List[str] = field(default_factory=list)
    enabled: bool = True
    
    def get_hash(self) -> str:
        """Generate hash of package configuration for cache validation"""
        data = f"{self.name}:{self.source_directory}:{self.options}"
        return hashlib.sha256(data.encode()).hexdigest()[:16]


@dataclass
class BuildResult:
    """Result of a package build operation"""
    package: Package
    success: bool
    duration: float
    error_message: Optional[str] = None
    timestamp: str = field(default_factory=lambda: datetime.now().isoformat())


@dataclass
class BuildCache:
    """Build cache for tracking package states"""
    package_hashes: Dict[str, str] = field(default_factory=dict)
    build_times: Dict[str, str] = field(default_factory=dict)
    last_config: Optional[str] = None
    
    def is_package_cached(self, package: Package, config: str) -> bool:
        """Check if package needs rebuilding"""
        if config != self.last_config:
            return False
        
        cached_hash = self.package_hashes.get(package.name)
        current_hash = package.get_hash()
        
        return cached_hash == current_hash
    
    def update_package(self, package: Package, config: str):
        """Update cache for a built package"""
        self.package_hashes[package.name] = package.get_hash()
        self.build_times[package.name] = datetime.now().isoformat()
        self.last_config = config


# ==================== Build Profiles ====================
class BuildProfile(Enum):
    """Predefined build profiles for different scenarios"""
    QUICK = "quick"      # Essential packages only
    FULL = "full"        # All packages (default)
    MINIMAL = "minimal"  # Absolute minimum
    DEV = "dev"          # Development with debug tools
    RELEASE = "release"  # Optimized release build
    
    def filter_packages(self, all_packages: List[Package]) -> List[Package]:
        """Filter packages based on profile"""
        if self == BuildProfile.FULL:
            return all_packages
        
        essential = {"glfw", "glad", "glm", "imgui", "entt", "ReactPhysics3D", "spdlog"}
        minimal = {"glfw", "glad", "glm"}
        dev_extra = essential | {"assimp"}
        
        if self == BuildProfile.MINIMAL:
            return [p for p in all_packages if p.name in minimal]
        elif self == BuildProfile.QUICK:
            return [p for p in all_packages if p.name in essential]
        elif self == BuildProfile.DEV:
            return [p for p in all_packages if p.name in dev_extra]
        elif self == BuildProfile.RELEASE:
            # Release includes everything except test/debug tools
            return [p for p in all_packages if "test" not in p.name.lower()]
        
        return all_packages


# ==================== Version Control Setup ====================
class VersionControlSetup:
    """Setup handler for Motion Engine Version Control System"""
    
    REQUIRED_FILES = {
        "cmake/GenerateVersionHeader.cmake": r"""
# Version Control System
find_package(Git QUIET)

function(generate_version_header)
    set(options "")
    set(oneValueArgs TARGET VERSION_FILE)
    set(multiValueArgs "")
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT ARG_TARGET)
        message(FATAL_ERROR "generate_version_header: TARGET argument is required")
    endif()

    if(NOT ARG_VERSION_FILE)
        message(FATAL_ERROR "generate_version_header: VERSION_FILE argument is required")
    endif()

    # Get Git information
    if(GIT_FOUND)
        # Get Git commit hash
        execute_process(
            COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_COMMIT_HASH
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
        
        # Get Git branch
        execute_process(
            COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_BRANCH
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
        
        # Get Git tag
        execute_process(
            COMMAND ${GIT_EXECUTABLE} describe --tags --abbrev=0
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_TAG
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
        
        # Check if working directory is clean
        execute_process(
            COMMAND ${GIT_EXECUTABLE} diff-index --quiet HEAD --
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            RESULT_VARIABLE GIT_IS_DIRTY
            ERROR_QUIET
        )
        
        if(GIT_IS_DIRTY)
            set(GIT_IS_DIRTY_FLAG "true")
        else()
            set(GIT_IS_DIRTY_FLAG "false")
        endif()
        
        # Get commit count
        execute_process(
            COMMAND ${GIT_EXECUTABLE} rev-list --count HEAD
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_COMMIT_COUNT
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
    else()
        set(GIT_COMMIT_HASH "unknown")
        set(GIT_BRANCH "unknown")
        set(GIT_TAG "unknown")
        set(GIT_IS_DIRTY_FLAG "false")
        set(GIT_COMMIT_COUNT "0")
    endif()

    # Get build timestamp
    string(TIMESTAMP BUILD_TIMESTAMP "%Y-%m-%d %H:%M:%S UTC" UTC)

    # Get project version
    if(NOT DEFINED PROJECT_VERSION_MAJOR)
        set(PROJECT_VERSION_MAJOR 0)
    endif()
    if(NOT DEFINED PROJECT_VERSION_MINOR)
        set(PROJECT_VERSION_MINOR 0)
    endif()
    if(NOT DEFINED PROJECT_VERSION_PATCH)
        set(PROJECT_VERSION_PATCH 0)
    endif()

    # Set version string
    set(PROJECT_VERSION_STRING "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")

    # Find the template file
    set(TEMPLATE_FILE "${CMAKE_SOURCE_DIR}/cmake/MotionVersion.hpp.in")
    if(NOT EXISTS ${TEMPLATE_FILE})
        message(FATAL_ERROR "Template file not found: ${TEMPLATE_FILE}")
    endif()

    # Configure the header file
    configure_file(
        ${TEMPLATE_FILE}
        ${ARG_VERSION_FILE}
        @ONLY
    )

    message(STATUS "===============================================")
    message(STATUS " Generated version header: ${ARG_VERSION_FILE}")
    message(STATUS "===============================================")
    message(STATUS " Version    : ${PROJECT_VERSION_STRING}")
    message(STATUS " Git Hash   : ${GIT_COMMIT_HASH}")
    message(STATUS " Git Branch : ${GIT_BRANCH}")
    message(STATUS " Build Type : ${CMAKE_BUILD_TYPE_INIT}")
    message(STATUS "===============================================")

    # Make sure the generated directory is added to the target's include directories
    # (Already handled in your CMakeLists.txt but documenting here for clarity)
    
    # Make the version header a dependency of configure
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
        ${CMAKE_SOURCE_DIR}/.git/HEAD
        ${CMAKE_SOURCE_DIR}/.git/index
    )

endfunction()
""",

        "cmake/MotionVersion.hpp.in": r"""
#pragma once
/**
 * @file MotionVersion.hpp
 * @brief Auto-generated version information for Motion Engine
 * 
 * This file is automatically generated during the CMake configure process.
 * Do not edit manually - your changes will be overwritten.
 * 
 * Generated at: @BUILD_TIMESTAMP@
 */

#include <string>
#include <sstream>

namespace Motion 
{
    // ==================== Semantic Version ====================
    constexpr int MAJOR = @PROJECT_VERSION_MAJOR@;
    constexpr int MINOR = @PROJECT_VERSION_MINOR@;
    constexpr int PATCH = @PROJECT_VERSION_PATCH@;
    
    // Version string (e.g., "1.0.0")
    inline constexpr const char* VERSION = "@PROJECT_VERSION_STRING@";
    
    // ==================== Git Information ====================
    inline constexpr const char* GIT_COMMIT_HASH    = "@GIT_COMMIT_HASH@";
    inline constexpr const char* GIT_BRANCH         = "@GIT_BRANCH@";
    inline constexpr const char* GIT_TAG            = "@GIT_TAG@";
    inline constexpr bool GIT_IS_DIRTY              = @GIT_IS_DIRTY_FLAG@;
    inline constexpr const char* GIT_COMMIT_COUNT   = "@GIT_COMMIT_COUNT@";
    
    // ==================== Build Information ====================
    inline constexpr const char* BUILD_TIMESTAMP    = "@BUILD_TIMESTAMP@";
    inline constexpr const char* BUILD_TYPE         = "@CMAKE_BUILD_TYPE_INIT@";
    
    // ==================== Helper Functions ====================
    
    /**
     * @brief Get the full version string with Git information
     * @return Version string like "1.0.0 (abc123)" or "1.0.0 (abc123-dirty)"
     */
    inline std::string GetFullVersion() {
        std::string version = VERSION;
        
        std::string hash = GIT_COMMIT_HASH;
        if (hash != "unknown" && !hash.empty()) {
            version += " (" + hash;
            if (GIT_IS_DIRTY) {
                version += "-dirty";
            }
            version += ")";
        }
        
        return version;
    }
    
    /**
     * @brief Get detailed version information with all metadata
     * @return Multi-line string with complete version info
     */
    inline std::string GetDetailedVersion() {
        std::ostringstream info;
        
        info << "Motion Engine v" << VERSION << "\n";
        info << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        info << "Build Configuration: " << BUILD_TYPE << "\n";
        info << "Build Timestamp:     " << BUILD_TIMESTAMP << "\n";
        
        std::string hash = GIT_COMMIT_HASH;
        if (hash != "unknown" && !hash.empty()) {
            info << "\nVersion Control:\n";
            info << "  Branch:       " << GIT_BRANCH << "\n";
            info << "  Commit:       " << GIT_COMMIT_HASH;
            if (GIT_IS_DIRTY) {
                info << " (with uncommitted changes)";
            }
            info << "\n";
            
            std::string tag = GIT_TAG;
            if (!tag.empty() && tag != "unknown") {
                info << "  Tag:          " << tag << "\n";
            }
            
            info << "  Total Commits: " << GIT_COMMIT_COUNT << "\n";
        }
        
        return info.str();
    }
    
    /**
     * @brief Get a short version identifier for display
     * @return Short version like "v1.0.0-abc123"
     */
    inline std::string GetShortVersion() {
        std::string version = "v";
        version += VERSION;
        
        std::string hash = GIT_COMMIT_HASH;
        if (hash != "unknown" && !hash.empty()) {
            version += "-" + hash;
        }
        
        return version;
    }
    
    /**
     * @brief Check if this is a release build (no dirty working directory)
     * @return true if working directory was clean at build time
     */
    inline constexpr bool IsReleaseBuild() {
        return !GIT_IS_DIRTY;
    }
    
    /**
     * @brief Get semantic version as comparable integer
     * @return Integer representation (MAJOR * 10000 + MINOR * 100 + PATCH)
     */
    inline constexpr int GetVersionNumber() {
        return MAJOR * 10000 + MINOR * 100 + PATCH;
    }
    
    /**
     * @brief Get version banner for splash screens or logs
     * @return Formatted banner string
     */
    inline std::string GetBanner() {
        std::ostringstream banner;
        
        banner << "╔═══════════════════════════════════════════════════════════╗\n";
        banner << "║              MOTION ENGINE v" << VERSION;
        
        // Pad to center align
        std::string versionStr = std::string("MOTION ENGINE v") + VERSION;
        int padding = (55 - versionStr.length()) / 2;
        for (int i = 0; i < padding; ++i) {
            banner << " ";
        }
        banner << "║\n";
        banner << "╠═══════════════════════════════════════════════════════════╣\n";
        banner << "║  Physics Simulation for Educational Purposes              ║\n";
        banner << "╚═══════════════════════════════════════════════════════════╝\n";
        
        return banner.str();
    }

} 

#define MOTION_VERSION Motion::VERSION
#define MOTION_VERSION_MAJOR Motion::MAJOR
#define MOTION_VERSION_MINOR Motion::MINOR
#define MOTION_VERSION_PATCH Motion::PATCH
#define MOTION_GIT_HASH Motion::GIT_COMMIT_HASH
#define MOTION_BUILD_TYPE Motion::BUILD_TYPE

"""
    }
    
    CMAKE_INTEGRATION = r"""
# Version Control Integration - Define version before project()
set(PROJECT_VERSION_MAJOR 1)
set(PROJECT_VERSION_MINOR 0)
set(PROJECT_VERSION_PATCH 0)

project(Motion VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH} LANGUAGES C CXX)

# Generate version header at configure time
include(${CMAKE_SOURCE_DIR}/cmake/GenerateVersionHeader.cmake)

# Make sure generated directory exists
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/generated)

# Make sure to rebuild when Git state changes (optional but recommended)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    ${CMAKE_SOURCE_DIR}/.git/HEAD
    ${CMAKE_SOURCE_DIR}/.git/index
)
"""
    
    @staticmethod
    def check_project_root() -> bool:
        """Check if we're in a valid project root"""
        return Path("CMakeLists.txt").exists()
    
    @staticmethod
    def backup_file(filepath: Path) -> Path:
        """Create a backup of a file"""
        if not filepath.exists():
            return filepath
        
        backup_path = filepath.with_suffix(filepath.suffix + ".backup")
        counter = 1
        while backup_path.exists():
            backup_path = filepath.with_suffix(f"{filepath.suffix}.backup{counter}")
            counter += 1
        
        shutil.copy2(filepath, backup_path)
        Logger.success(f"Backed up {filepath} to {backup_path}")
        return backup_path
    
    @classmethod
    def setup(cls) -> bool:
        """Setup version control system"""
        try:
            Logger.info("Setting up Version Control System for Motion Engine...")
            
            # Check if we're in project root
            if not cls.check_project_root():
                Logger.error("Please run this command from your project root directory")
                Logger.error("(Directory containing CMakeLists.txt)")
                return False
            
            # Create cmake directory if needed
            cmake_dir = Path("cmake")
            if not cmake_dir.exists():
                Logger.info("Creating cmake directory...")
                cmake_dir.mkdir(parents=True, exist_ok=True)
            
            # Create required files
            Logger.info("Creating version control files...")
            for filepath, content in cls.REQUIRED_FILES.items():
                file_path = Path(filepath)
                file_path.parent.mkdir(parents=True, exist_ok=True)
                
                # Backup existing file
                if file_path.exists():
                    cls.backup_file(file_path)
                
                # Write new content
                with open(file_path, 'w', encoding='utf-8') as f:
                    f.write(content)
                Logger.success(f"Created {filepath}")
            
            # Create generated directory
            generated_dir = Path("build/generated")
            generated_dir.mkdir(parents=True, exist_ok=True)
            
            # Check if CMakeLists.txt needs updating
            cmake_file = Path("CMakeLists.txt")
            with open(cmake_file, 'r', encoding='utf-8') as f:
                cmake_content = f.read()
            
            needs_update = "GenerateVersionHeader.cmake" not in cmake_content
            
            if needs_update:
                Logger.warn("CMakeLists.txt needs to be updated with version control integration")
                Logger.info("Please add the following to your CMakeLists.txt:")
                print(f"\n{Color.YELLOW}{cls.CMAKE_INTEGRATION}{Color.RESET}\n")
                
                # Offer to automatically update
                response = input("Would you like to automatically update CMakeLists.txt? (y/N): ").strip().lower()
                if response == 'y':
                    cls.backup_file(cmake_file)
                    
                    # Insert after project() declaration
                    pattern = r'(project\s*\([^)]+\))'
                    updated_content = re.sub(
                        pattern,
                        r'\1\n' + cls.CMAKE_INTEGRATION,
                        cmake_content,
                        count=1
                    )
                    
                    with open(cmake_file, 'w', encoding='utf-8') as f:
                        f.write(updated_content)
                    Logger.success("Updated CMakeLists.txt")
            
            # Display next steps
            print(f"\n{Color.GREEN}{'=' * 60}{Color.RESET}")
            print(f"{Color.BOLD}✓ Version Control System installed successfully!{Color.RESET}")
            print(f"{Color.GREEN}{'=' * 60}{Color.RESET}\n")
            
            print(f"{Color.BOLD}Next steps:{Color.RESET}")
            print("1. Review the changes (original files backed up with .backup extension)")
            print("2. Reconfigure your build:")
            print(f"   {Color.CYAN}python Setup.py build --config Debug{Color.RESET}")
            print("3. Include version info in your code:")
            print(f"   {Color.CYAN}#include <MotionVersion.hpp>{Color.RESET}")
            print("4. Access version information:")
            print(f"   {Color.CYAN}Motion::GetFullVersion(){Color.RESET}\n")
            
            print(f"{Color.BOLD}Usage Example:{Color.RESET}")
            print(f"{Color.DIM}#include <MotionVersion.hpp>")
            print(f"#include <iostream>")
            print()
            print(f"int main() {{")
            print(f"    std::cout << Motion::GetDetailedVersion();")
            print(f"    return 0;")
            print(f"}}{Color.RESET}\n")
            
            return True
            
        except Exception as e:
            Logger.error(f"Failed to setup version control: {e}")
            import traceback
            traceback.print_exc()
            return False


# ==================== Enhanced Preset Generator ====================
class PresetGenerator:
    """
    Enhanced CMakePresets.json generator with proper compiler flags
    for each build configuration (Debug, Release, RelWithDebInfo, MinSizeRel)
    """
    
    def __init__(self, packages: List[Package], compiler_c: Optional[str] = None, compiler_cxx: Optional[str] = None):
        self.packages = packages
        self.system = platform.system()
        self.is_windows = self.system == "Windows"
        self.is_linux = self.system == "Linux"
        self.is_macos = self.system == "Darwin"
        self.compiler_c = compiler_c
        self.compiler_cxx = compiler_cxx
    
    def generate(self, project_root: Path):
        """Generate comprehensive CMakePresets.json with proper compiler settings"""
        presets = {
            "version": 6,
            "cmakeMinimumRequired": {
                "major": BuildConfig.MIN_CMAKE_VERSION[0],
                "minor": BuildConfig.MIN_CMAKE_VERSION[1],
                "patch": BuildConfig.MIN_CMAKE_VERSION[2]
            },
            "configurePresets": self._generate_configure_presets(),
            "buildPresets": self._generate_build_presets(),
            "testPresets": self._generate_test_presets()
        }
        
        output_file = project_root / "CMakePresets.json"
        with open(output_file, 'w', encoding='utf-8') as f:
            json.dump(presets, f, indent=2)
        
        Logger.success(f"Generated {output_file}")
        Logger.info(f"  - {len(presets['configurePresets'])} configure presets")
        Logger.info(f"  - {len(presets['buildPresets'])} build presets")
        Logger.info(f"  - Platform: {self.system}")
    
    def _get_generator(self) -> str:
        """Get appropriate CMake generator for platform"""
        if self.is_windows:
            # Check if Ninja is available
            try:
                subprocess.run(["ninja", "--version"], capture_output=True, check=True)
                return "Ninja Multi-Config"
            except (subprocess.CalledProcessError, FileNotFoundError):
                return "Visual Studio 17 2022"  # Fallback to VS2022
        else:
            # Try Ninja first, fallback to Unix Makefiles
            try:
                subprocess.run(["ninja", "--version"], capture_output=True, check=True)
                return "Ninja Multi-Config"
            except (subprocess.CalledProcessError, FileNotFoundError):
                return "Unix Makefiles"
    
    def _get_msvc_flags(self, config: str) -> Dict[str, List[str]]:
        """Get MSVC-specific compiler flags for each configuration"""
        flags = {
            "Debug": {
                "compile": [
                    "/MDd",      # Multi-threaded Debug DLL runtime
                    "/Od",       # Disable optimization
                    "/Zi",       # Debug information
                    "/RTC1",     # Runtime checks
                    "/JMC",      # Just My Code debugging
                    "/W4",       # Warning level 4
                    "/permissive-",  # Standards conformance
                    "/Zc:__cplusplus",  # Enable updated __cplusplus macro
                    "/EHsc",     # Exception handling
                    "/bigobj"    # Large object files
                ],
                "link": [
                    "/DEBUG:FULL",  # Full debug information
                    "/INCREMENTAL"  # Incremental linking
                ]
            },
            "Release": {
                "compile": [
                    "/MD",       # Multi-threaded DLL runtime
                    "/O2",       # Maximum optimization
                    "/Ob2",      # Inline expansion
                    "/Oi",       # Intrinsic functions
                    "/Ot",       # Favor fast code
                    "/GL",       # Whole program optimization
                    "/GS-",      # Disable security checks
                    "/Gy",       # Function-level linking
                    "/W3",       # Warning level 3
                    "/permissive-",
                    "/Zc:__cplusplus",
                    "/EHsc",
                    "/DNDEBUG"   # Define NDEBUG
                ],
                "link": [
                    "/LTCG",     # Link-time code generation
                    "/OPT:REF",  # Remove unreferenced functions
                    "/OPT:ICF",  # Identical COMDAT folding
                    "/INCREMENTAL:NO"
                ]
            },
            "RelWithDebInfo": {
                "compile": [
                    "/MD",
                    "/O2",
                    "/Ob1",      # Inline expansion (less aggressive)
                    "/Oi",
                    "/Zi",       # Debug information
                    "/W3",
                    "/permissive-",
                    "/Zc:__cplusplus",
                    "/EHsc",
                    "/DNDEBUG"
                ],
                "link": [
                    "/DEBUG",
                    "/INCREMENTAL:NO",
                    "/OPT:REF",
                    "/OPT:ICF"
                ]
            },
            "MinSizeRel": {
                "compile": [
                    "/MD",
                    "/O1",       # Minimize size
                    "/Os",       # Favor small code
                    "/Ob1",
                    "/GS-",
                    "/Gy",
                    "/W3",
                    "/permissive-",
                    "/Zc:__cplusplus",
                    "/EHsc",
                    "/DNDEBUG"
                ],
                "link": [
                    "/INCREMENTAL:NO",
                    "/OPT:REF",
                    "/OPT:ICF"
                ]
            }
        }
        return flags.get(config, flags["Release"])
    
    def _get_gcc_clang_flags(self, config: str) -> Dict[str, List[str]]:
        """Get GCC/Clang compiler flags for each configuration"""
        flags = {
            "Debug": {
                "compile": [
                    "-g3",           # Maximum debug information
                    "-O0",           # No optimization
                    "-Wall",         # All warnings
                    "-Wextra",       # Extra warnings
                    "-Wpedantic",    # Pedantic warnings
                    "-fno-omit-frame-pointer",  # Keep frame pointer
                    "-fno-inline",   # Disable inlining
                    "-fstack-protector-strong",  # Stack protection
                    "-D_GLIBCXX_DEBUG",  # STL debug mode (GCC)
                    "-D_GLIBCXX_DEBUG_PEDANTIC"
                ],
                "link": [
                    "-rdynamic"      # Export dynamic symbols for backtrace
                ]
            },
            "Release": {
                "compile": [
                    "-O3",           # Maximum optimization
                    "-march=native", # Optimize for current CPU
                    "-mtune=native",
                    "-flto",         # Link-time optimization
                    "-ffast-math",   # Fast math operations
                    "-funroll-loops", # Unroll loops
                    "-fomit-frame-pointer",  # Omit frame pointer
                    "-ffunction-sections",  # Function sections for linker
                    "-fdata-sections",
                    "-DNDEBUG",      # Disable assertions
                    "-Wall",
                    "-Wextra"
                ],
                "link": [
                    "-flto",
                    "-Wl,--gc-sections",  # Garbage collect unused sections
                    "-Wl,-O3"        # Linker optimization
                ]
            },
            "RelWithDebInfo": {
                "compile": [
                    "-O2",           # Standard optimization
                    "-g",            # Debug information
                    "-march=native",
                    "-mtune=native",
                    "-ffunction-sections",
                    "-fdata-sections",
                    "-DNDEBUG",
                    "-Wall",
                    "-Wextra"
                ],
                "link": [
                    "-Wl,--gc-sections"
                ]
            },
            "MinSizeRel": {
                "compile": [
                    "-Os",           # Optimize for size
                    "-march=native",
                    "-mtune=native",
                    "-flto",
                    "-ffunction-sections",
                    "-fdata-sections",
                    "-fomit-frame-pointer",
                    "-DNDEBUG",
                    "-Wall"
                ],
                "link": [
                    "-flto",
                    "-Wl,--gc-sections",
                    "-Wl,-s"         # Strip symbols
                ]
            }
        }
        return flags.get(config, flags["Release"])
    
    def _generate_configure_presets(self) -> List[Dict]:
        """Generate configure presets with proper compiler settings"""
        presets = []
        generator = self._get_generator()
        is_multi_config = "Multi-Config" in generator or "Visual Studio" in generator
        
        for config in BuildConfig.CONFIGS:
            preset_name = config.lower()
            
            # Base cache variables
            cache_vars = {
                "CMAKE_CXX_STANDARD": BuildConfig.CXX_STANDARD,
                "CMAKE_CXX_STANDARD_REQUIRED": "ON",
                "CMAKE_CXX_EXTENSIONS": "OFF",
                "CMAKE_C_STANDARD": BuildConfig.C_STANDARD,
                "CMAKE_C_STANDARD_REQUIRED": "ON",
                "CMAKE_C_EXTENSIONS": "OFF",
                "CMAKE_EXPORT_COMPILE_COMMANDS": "ON",
                "CMAKE_COLOR_DIAGNOSTICS": "ON",
                "CMAKE_PREFIX_PATH": "${sourceDir}/build/install",
            }
            
            # Set explicit compilers if specified
            if self.compiler_c:
                cache_vars["CMAKE_C_COMPILER"] = self.compiler_c
            if self.compiler_cxx:
                cache_vars["CMAKE_CXX_COMPILER"] = self.compiler_cxx
            
            # For single-config generators, set build type
            if not is_multi_config:
                cache_vars["CMAKE_BUILD_TYPE"] = config
            
            # Platform-specific compiler flags
            if self.is_windows:
                flags = self._get_msvc_flags(config)
                
                # MSVC flags
                cache_vars["CMAKE_CXX_FLAGS"] = " ".join(flags["compile"])
                cache_vars["CMAKE_C_FLAGS"] = " ".join(flags["compile"])
                cache_vars["CMAKE_EXE_LINKER_FLAGS"] = " ".join(flags["link"])
                cache_vars["CMAKE_SHARED_LINKER_FLAGS"] = " ".join(flags["link"])
                cache_vars["CMAKE_STATIC_LINKER_FLAGS"] = " ".join(flags["link"])
                
                # Runtime library
                if config == "Debug":
                    cache_vars["CMAKE_MSVC_RUNTIME_LIBRARY"] = "MultiThreadedDebugDLL"
                else:
                    cache_vars["CMAKE_MSVC_RUNTIME_LIBRARY"] = "MultiThreadedDLL"
                
            else:
                # GCC/Clang flags
                flags = self._get_gcc_clang_flags(config)
                
                cache_vars["CMAKE_CXX_FLAGS"] = " ".join(flags["compile"])
                cache_vars["CMAKE_C_FLAGS"] = " ".join(flags["compile"])
                cache_vars["CMAKE_EXE_LINKER_FLAGS"] = " ".join(flags["link"])
                cache_vars["CMAKE_SHARED_LINKER_FLAGS"] = " ".join(flags["link"])
            
            # Additional configuration-specific settings
            if config == "Debug":
                cache_vars["CMAKE_INTERPROCEDURAL_OPTIMIZATION"] = "OFF"
                cache_vars["BUILD_SHARED_LIBS"] = "OFF"  # Static libs for debugging
            elif config == "Release" or config == "MinSizeRel":
                if not self.is_windows:  # LTO works better on Unix
                    cache_vars["CMAKE_INTERPROCEDURAL_OPTIMIZATION"] = "ON"
                cache_vars["BUILD_SHARED_LIBS"] = "ON"
            elif config == "RelWithDebInfo":
                cache_vars["CMAKE_INTERPROCEDURAL_OPTIMIZATION"] = "OFF"
                cache_vars["BUILD_SHARED_LIBS"] = "ON"
            
            # Create preset
            preset = {
                "name": preset_name,
                "displayName": f"{config} Build",
                "description": f"{config} configuration with optimized compiler settings for {self.system}",
                "generator": generator,
                "binaryDir": "${sourceDir}/build/" + preset_name,
                "cacheVariables": cache_vars
            }
            
            # Add architecture for Visual Studio
            if "Visual Studio" in generator and self.is_windows:
                preset["architecture"] = {
                    "value": "x64",
                    "strategy": "external"
                }
            
            # Add toolchain file location if exists
            toolchain_file = Path("cmake/toolchain.cmake")
            if toolchain_file.exists():
                preset["cacheVariables"]["CMAKE_TOOLCHAIN_FILE"] = "${sourceDir}/cmake/toolchain.cmake"
            
            presets.append(preset)
        
        return presets
    
    def _generate_build_presets(self) -> List[Dict]:
        """Generate build presets for each configuration"""
        presets = []
        
        for config in BuildConfig.CONFIGS:
            preset_name = config.lower()
            
            preset: Dict[str, Any] = {
                "name": preset_name,
                "displayName": f"Build {config}",
                "description": f"Build using {config} configuration",
                "configurePreset": preset_name,
                "configuration": config,
            }
            
            # Add parallel build jobs
            preset["jobs"] = os.cpu_count() or 4
            
            # Add verbose output option
            preset["verbose"] = False
            
            presets.append(preset)
            
            # Add verbose variant
            verbose_preset = {
                "name": f"{preset_name}-verbose",
                "displayName": f"Build {config} (Verbose)",
                "description": f"Build using {config} configuration with verbose output",
                "configurePreset": preset_name,
                "configuration": config,
                "verbose": True,
                "jobs": os.cpu_count() or 4
            }
            presets.append(verbose_preset)
        
        return presets
    
    def _generate_test_presets(self) -> List[Dict]:
        """Generate test presets (if testing is enabled)"""
        presets = []
        
        for config in BuildConfig.CONFIGS:
            preset_name = config.lower()
            
            preset = {
                "name": preset_name,
                "displayName": f"Test {config}",
                "description": f"Run tests for {config} configuration",
                "configurePreset": preset_name,
                "configuration": config,
                "output": {
                    "outputOnFailure": True
                },
                "execution": {
                    "noTestsAction": "error",
                    "stopOnFailure": False
                }
            }
            
            presets.append(preset)
        
        return presets


# ==================== System Validator ====================
class SystemValidator:
    """Validates system requirements"""
    
    @staticmethod
    def check_cmake():
        """Check if CMake is installed and meets minimum version"""
        try:
            result = subprocess.run(
                ["cmake", "--version"],
                capture_output=True,
                text=True,
                check=True
            )
            
            version_line = result.stdout.split('\n')[0]
            version_match = re.search(r'(\d+)\.(\d+)\.(\d+)', version_line)
            
            if version_match:
                major, minor, patch = map(int, version_match.groups())
                required = BuildConfig.MIN_CMAKE_VERSION
                
                if (major, minor, patch) >= required:
                    Logger.success(f"CMake {major}.{minor}.{patch} found")
                    return
                else:
                    raise ConfigurationException(
                        f"CMake {major}.{minor}.{patch} found, but {required[0]}.{required[1]}.{required[2]}+ required"
                    )
            
        except FileNotFoundError:
            raise ConfigurationException("CMake not found. Please install CMake.")
        except subprocess.CalledProcessError:
            raise ConfigurationException("Failed to check CMake version")
    
    @staticmethod
    def check_compiler():
        """Check if a C++ compiler is available"""
        compilers = []
        
        if platform.system() == "Windows":
            compilers = [
                ("cl", "MSVC"),
                ("clang++", "Clang"),
                ("g++", "GCC")
            ]
        else:
            compilers = [
                ("g++", "GCC"),
                ("clang++", "Clang")
            ]
        
        for compiler_cmd, name in compilers:
            try:
                result = subprocess.run(
                    [compiler_cmd, "--version"],
                    capture_output=True,
                    text=True,
                    check=True
                )
                Logger.success(f"{name} compiler found")
                return
            except (FileNotFoundError, subprocess.CalledProcessError):
                continue
        
        raise ConfigurationException(
            "No C++ compiler found. Please install GCC, Clang, or MSVC."
        )
    
    @staticmethod
    def check_disk_space(required_gb: int = 5):
        """Check if sufficient disk space is available"""
        try:
            stat = shutil.disk_usage(os.getcwd())
            free_gb = stat.free / (1024 ** 3)
            
            if free_gb < required_gb:
                Logger.warn(
                    f"Low disk space: {free_gb:.1f}GB free (recommended: {required_gb}GB+)"
                )
            else:
                Logger.success(f"Disk space: {free_gb:.1f}GB free")
        except Exception as e:
            Logger.warn(f"Could not check disk space: {e}")


# ==================== Compiler Detection ====================
def detect_and_list_compilers():
    """Detect and list available compilers on the system"""
    print(f"\n{Color.BOLD}Available Compilers:{Color.RESET}\n")
    
    compilers_to_check = {
        "C": ["gcc", "clang", "cl"],
        "C++": ["g++", "clang++", "cl"]
    }
    
    for lang, compilers in compilers_to_check.items():
        print(f"{Color.CYAN}{lang} Compilers:{Color.RESET}")
        for compiler in compilers:
            try:
                result = subprocess.run(
                    [compiler, "--version"],
                    capture_output=True,
                    check=True,
                    text=True
                )
                version_line = result.stdout.split('\n')[0]
                print(f"  {Color.GREEN}✓{Color.RESET} {compiler:15} {version_line[:60]}")
            except (subprocess.CalledProcessError, FileNotFoundError):
                print(f"  {Color.RED}✗{Color.RESET} {compiler:15} Not found")
        print()


# ==================== Package Registry ====================
def get_package_registry() -> List[Package]:
    """Define all third-party packages for Motion Engine"""

    if not Path.exists(Path("libs")):
        raise ConfigurationException(f"Vendor directory not found: {"libs"}")
    
    packages = [
        Package(
            name="glfw",
            source_directory="libs/glfw",
            build_directory="build/vendor/glfw",
            prefix_directory="build/install",
            options="-DGLFW_BUILD_DOCS=OFF -DGLFW_BUILD_EXAMPLES=OFF -DGLFW_BUILD_TESTS=OFF"
        ),
        Package(
            name="glad",
            source_directory="libs/glad",
            build_directory="build/vendor/glad",
            prefix_directory="build/install"
        ),
        Package(
            name="glm",
            source_directory="libs/glm",
            build_directory="build/vendor/glm",
            prefix_directory="build/install",
            options="-DGLM_BUILD_TESTS=OFF"
        ),
        Package(
            name="imgui",
            source_directory="libs/imgui_docking",
            build_directory="build/vendor/imgui",
            prefix_directory="build/install",
            dependencies=["glfw", "glad"]
        ),
        Package(
            name="imguizmo",
            source_directory="libs/imGuizmo",
            build_directory="build/vendor/imguizmo",
            prefix_directory="build/install"
        ),
        Package(
            name="MikkTSpace",
            source_directory="libs/MikkTSpace",
            build_directory="build/vendor/MikkTSpace",
            prefix_directory="build/install"
        ),
        Package(
            name="entt",
            source_directory="libs/entt",
            build_directory="build/vendor/entt",
            prefix_directory="build/install",
            options="-DENTT_BUILD_TESTING=OFF -DENTT_INSTALL=ON"
        ),
        Package(
            name="ReactPhysics3D",
            source_directory="libs/reactphysics3d",
            build_directory="build/vendor/reactphysics3d",
            prefix_directory="build/install",
            options="-DRP3D_COMPILE_TESTS=OFF"
        ),
        Package(
            name="spdlog",
            source_directory="libs/spdlog",
            build_directory="build/vendor/spdlog",
            prefix_directory="build/install",
            options="-DSPDLOG_BUILD_EXAMPLE=OFF"
        ),
        Package(
            name="assimp",
            source_directory="libs/assimp",
            build_directory="build/vendor/assimp",
            prefix_directory="build/install",
            options="-DASSIMP_BUILD_TESTS=OFF -DASSIMP_BUILD_ASSIMP_TOOLS=OFF -DASSIMP_INSTALL_PDB=OFF"
        ),
        Package(
            name="stb",
            source_directory="libs/stb_image",
            build_directory="build/vendor/stb",
            prefix_directory="build/install"
        ),
        Package(
            name="yaml-cpp",
            source_directory="libs/yaml-cpp",
            build_directory="build/vendor/yaml-cpp",
            prefix_directory="build/install"
        )
    ]
    
    return packages


# ==================== Dependency Graph ====================
class DependencyGraph:
    """Manages package dependencies and build order"""
    
    def __init__(self, packages: List[Package]):
        self.packages = {pkg.name: pkg for pkg in packages}
        self.graph = self._build_graph()
    
    def _build_graph(self) -> Dict[str, Set[str]]:
        """Build dependency graph"""
        graph = {}
        for pkg in self.packages.values():
            graph[pkg.name] = set(pkg.dependencies)
        return graph
    
    def get_build_order(self) -> List[str]:
        """Get topologically sorted build order"""
        visited = set()
        order = []
        
        def visit(name: str):
            if name in visited:
                return
            visited.add(name)
            
            for dep in self.graph.get(name, set()):
                if dep in self.packages:
                    visit(dep)
            
            order.append(name)
        
        for pkg_name in self.packages:
            visit(pkg_name)
        
        return order
    
    def find_circular_dependencies(self) -> List[List[str]]:
        """Detect circular dependencies"""
        cycles = []
        visited = set()
        rec_stack = set()
        
        def visit(name: str, path: List[str]) -> bool:
            if name in rec_stack:
                cycle_start = path.index(name)
                cycles.append(path[cycle_start:] + [name])
                return True
            
            if name in visited:
                return False
            
            visited.add(name)
            rec_stack.add(name)
            
            for dep in self.graph.get(name, set()):
                if dep in self.packages:
                    if visit(dep, path + [name]):
                        return True
            
            rec_stack.remove(name)
            return False
        
        for pkg_name in self.packages:
            if pkg_name not in visited:
                visit(pkg_name, [])
        
        return cycles
    
    def print_analysis(self):
        """Print dependency analysis"""
        print(f"\n{Color.BOLD}Dependency Analysis:{Color.RESET}\n")
        
        order = self.get_build_order()
        print(f"{Color.CYAN}Build Order:{Color.RESET}")
        for i, pkg_name in enumerate(order, 1):
            print(f"  {i:2}. {pkg_name}")
        
        print(f"\n{Color.CYAN}Dependencies:{Color.RESET}")
        for pkg_name, deps in sorted(self.graph.items()):
            if deps:
                print(f"  {pkg_name:20} → {', '.join(sorted(deps))}")
            else:
                print(f"  {pkg_name:20} (no dependencies)")
        
        print()
    
    def visualize(self, output_path: Path):
        """Visualize dependency graph"""
        if not HAS_NETWORKX or not HAS_MATPLOTLIB:
            Logger.warn("Install networkx and matplotlib to visualize dependencies")
            return
        
        G = nx.DiGraph()
        
        for pkg_name, deps in self.graph.items():
            G.add_node(pkg_name)
            for dep in deps:
                if dep in self.packages:
                    G.add_edge(dep, pkg_name)
        
        plt.figure(figsize=(12, 8))
        pos = nx.spring_layout(G, k=2, iterations=50)
        
        nx.draw_networkx_nodes(G, pos, node_color='lightblue',
                              node_size=2000, alpha=0.9)
        nx.draw_networkx_labels(G, pos, font_size=10, font_weight='bold')
        nx.draw_networkx_edges(G, pos, edge_color='gray',
                              arrows=True, arrowsize=20, width=2,
                              connectionstyle='arc3,rad=0.1')
        
        plt.title("Motion Engine Dependency Graph", fontsize=16, fontweight='bold')
        plt.axis('off')
        plt.tight_layout()
        
        output_path.parent.mkdir(parents=True, exist_ok=True)
        plt.savefig(output_path, dpi=300, bbox_inches='tight')
        Logger.success(f"Dependency graph saved to {output_path}")


# ==================== Statistics Tracker ====================
class StatisticsTracker:
    """Track build statistics"""
    
    def __init__(self):
        self.stats_file = Path(BuildConfig.STATS_FILE)
        self.stats = self._load_stats()
    
    def _load_stats(self) -> Dict:
        """Load statistics from disk"""
        if self.stats_file.exists():
            try:
                with open(self.stats_file, 'r') as f:
                    return json.load(f)
            except Exception as e:
                Logger.warn(f"Failed to load statistics: {e}")
        
        return {
            "total_builds": 0,
            "successful_builds": 0,
            "failed_builds": 0,
            "total_build_time": 0.0,
            "package_stats": {},
            "history": []
        }
    
    def _save_stats(self):
        """Save statistics to disk"""
        self.stats_file.parent.mkdir(parents=True, exist_ok=True)
        with open(self.stats_file, 'w') as f:
            json.dump(self.stats, f, indent=2)
    
    def record_build(self, results: List[BuildResult]):
        """Record build results"""
        total_time = sum(r.duration for r in results)
        successful = sum(1 for r in results if r.success)
        failed = len(results) - successful
        
        self.stats["total_builds"] += 1
        self.stats["successful_builds"] += successful
        self.stats["failed_builds"] += failed
        self.stats["total_build_time"] += total_time
        
        # Update package statistics
        for result in results:
            pkg_name = result.package.name
            if pkg_name not in self.stats["package_stats"]:
                self.stats["package_stats"][pkg_name] = {
                    "builds": 0,
                    "successes": 0,
                    "failures": 0,
                    "total_time": 0.0,
                    "avg_time": 0.0
                }
            
            pkg_stats = self.stats["package_stats"][pkg_name]
            pkg_stats["builds"] += 1
            pkg_stats["successes"] += 1 if result.success else 0
            pkg_stats["failures"] += 0 if result.success else 1
            pkg_stats["total_time"] += result.duration
            pkg_stats["avg_time"] = pkg_stats["total_time"] / pkg_stats["builds"]
        
        # Add to history (keep last 50)
        self.stats["history"].append({
            "timestamp": datetime.now().isoformat(),
            "packages": len(results),
            "successful": successful,
            "failed": failed,
            "duration": total_time
        })
        self.stats["history"] = self.stats["history"][-50:]
        
        self._save_stats()
    
    def print_stats(self):
        """Print build statistics"""
        print(f"\n{Color.BOLD}Build Statistics:{Color.RESET}\n")
        
        print(f"{Color.CYAN}Overall:{Color.RESET}")
        print(f"  Total builds: {self.stats['total_builds']}")
        print(f"  Successful:   {self.stats['successful_builds']} "
              f"({self.stats['successful_builds']/max(self.stats['total_builds'],1)*100:.1f}%)")
        print(f"  Failed:       {self.stats['failed_builds']}")
        print(f"  Total time:   {timedelta(seconds=int(self.stats['total_build_time']))}")
        
        if self.stats["package_stats"]:
            print(f"\n{Color.CYAN}Package Statistics:{Color.RESET}")
            sorted_packages = sorted(
                self.stats["package_stats"].items(),
                key=lambda x: x[1]["avg_time"],
                reverse=True
            )
            
            for pkg_name, pkg_stats in sorted_packages[:10]:
                success_rate = pkg_stats["successes"] / max(pkg_stats["builds"], 1) * 100
                print(f"  {pkg_name:20} "
                      f"builds: {pkg_stats['builds']:3} "
                      f"success: {success_rate:5.1f}% "
                      f"avg: {pkg_stats['avg_time']:6.1f}s")
        
        print()


# ==================== Benchmark Manager ====================
class BenchmarkManager:
    """Manage build benchmarks"""
    
    def __init__(self):
        self.benchmark_file = Path(BuildConfig.BENCHMARK_FILE)
        self.benchmarks = self._load_benchmarks()
    
    def _load_benchmarks(self) -> Dict:
        """Load benchmarks from disk"""
        if self.benchmark_file.exists():
            try:
                with open(self.benchmark_file, 'r') as f:
                    return json.load(f)
            except Exception as e:
                Logger.warn(f"Failed to load benchmarks: {e}")
        return {}
    
    def _save_benchmarks(self):
        """Save benchmarks to disk"""
        self.benchmark_file.parent.mkdir(parents=True, exist_ok=True)
        with open(self.benchmark_file, 'w') as f:
            json.dump(self.benchmarks, f, indent=2)
    
    def record(self, name: str, config: str, results: List[BuildResult]):
        """Record a benchmark"""
        total_time = sum(r.duration for r in results)
        
        self.benchmarks[name] = {
            "timestamp": datetime.now().isoformat(),
            "config": config,
            "packages": len(results),
            "total_time": total_time,
            "package_times": {r.package.name: r.duration for r in results}
        }
        
        self._save_benchmarks()
        Logger.success(f"Benchmark '{name}' recorded: {total_time:.1f}s")
    
    def compare(self, baseline: str, current: str):
        """Compare two benchmarks"""
        if baseline not in self.benchmarks:
            Logger.error(f"Baseline benchmark '{baseline}' not found")
            return
        
        if current not in self.benchmarks:
            Logger.error(f"Current benchmark '{current}' not found")
            return
        
        base = self.benchmarks[baseline]
        curr = self.benchmarks[current]
        
        print(f"\n{Color.BOLD}Benchmark Comparison:{Color.RESET}\n")
        print(f"Baseline: {baseline} ({base['config']}) - {base['total_time']:.1f}s")
        print(f"Current:  {current} ({curr['config']}) - {curr['total_time']:.1f}s")
        
        diff = curr['total_time'] - base['total_time']
        pct = (diff / base['total_time']) * 100
        
        color = Color.GREEN if diff < 0 else Color.RED
        print(f"\nDifference: {color}{diff:+.1f}s ({pct:+.1f}%){Color.RESET}\n")
        
        # Compare individual packages
        print(f"{Color.CYAN}Package Breakdown:{Color.RESET}")
        all_packages = set(base['package_times'].keys()) | set(curr['package_times'].keys())
        
        for pkg in sorted(all_packages):
            base_time = base['package_times'].get(pkg, 0)
            curr_time = curr['package_times'].get(pkg, 0)
            
            if base_time > 0:
                pkg_diff = curr_time - base_time
                pkg_pct = (pkg_diff / base_time) * 100
                pkg_color = Color.GREEN if pkg_diff < 0 else Color.RED
                print(f"  {pkg:20} {base_time:6.1f}s → {curr_time:6.1f}s "
                      f"{pkg_color}({pkg_pct:+5.1f}%){Color.RESET}")
        
        print()


# ==================== Incremental Build Tracker ====================
class IncrementalBuildTracker:
    """Track file changes for incremental builds"""
    
    def __init__(self):
        self.cache_file = Path(BuildConfig.INCREMENTAL_CACHE)
        self.cache = self._load_cache()
    
    def _load_cache(self) -> Dict:
        """Load cache from disk"""
        if self.cache_file.exists():
            try:
                with open(self.cache_file, 'r') as f:
                    return json.load(f)
            except Exception:
                pass
        return {}
    
    def _save_cache(self):
        """Save cache to disk"""
        self.cache_file.parent.mkdir(parents=True, exist_ok=True)
        with open(self.cache_file, 'w') as f:
            json.dump(self.cache, f, indent=2)
    
    def has_changes(self, package: Package) -> bool:
        """Check if package has changes"""
        source_dir = Path(package.source_directory)
        if not source_dir.exists():
            return True
        
        current_hash = self._compute_dir_hash(source_dir)
        cached_hash = self.cache.get(package.name)
        
        return current_hash != cached_hash
    
    def update(self, package: Package):
        """Update package hash"""
        source_dir = Path(package.source_directory)
        if source_dir.exists():
            self.cache[package.name] = self._compute_dir_hash(source_dir)
            self._save_cache()
    
    def _compute_dir_hash(self, directory: Path) -> str:
        """Compute hash of directory contents"""
        hasher = hashlib.sha256()
        
        try:
            for file_path in sorted(directory.rglob("*")):
                if file_path.is_file():
                    try:
                        with open(file_path, 'rb') as f:
                            hasher.update(f.read())
                    except (PermissionError, OSError):
                        pass
        except Exception:
            pass
        
        return hasher.hexdigest()[:16]


# ==================== Build Manager ====================
class BuildManager:
    """Manages the build process for all packages"""
    
    def __init__(self, packages: List[Package], parallel: bool = False,
                compiler_c: Optional[str] = None, compiler_cxx: Optional[str] = None):
        self.packages = packages
        self.parallel = parallel
        self.compiler_c = compiler_c
        self.compiler_cxx = compiler_cxx
        self.cache = self._load_cache()
        self.dep_graph = DependencyGraph(packages)
        self.stats_tracker = StatisticsTracker()
        self.benchmark_mgr = BenchmarkManager()
        self.incremental_tracker = IncrementalBuildTracker()
    
    def _load_cache(self) -> BuildCache:
        """Load build cache from disk"""
        cache_file = Path(BuildConfig.CACHE_FILE)
        if cache_file.exists():
            try:
                with open(cache_file, 'r') as f:
                    data = json.load(f)
                    return BuildCache(**data)
            except Exception as e:
                Logger.warn(f"Failed to load cache: {e}")
        return BuildCache()
    
    def _save_cache(self):
        """Save build cache to disk"""
        cache_file = Path(BuildConfig.CACHE_FILE)
        cache_file.parent.mkdir(parents=True, exist_ok=True)
        with open(cache_file, 'w') as f:
            json.dump(asdict(self.cache), f, indent=2)
    
    def _should_skip_package(self, package: Package, config: str, incremental: bool) -> bool:
        """Determine if package should be skipped"""
        if not incremental:
            return False
        
        # Check cache
        if not self.cache.is_package_cached(package, config):
            return False
        
        # Check for file changes
        if self.incremental_tracker.has_changes(package):
            return False
        
        # Check if build output exists
        build_dir = Path(package.build_directory)
        if not build_dir.exists():
            return False
        
        return True
    
    def build(
        self,
        config: str,
        specific_package: Optional[str] = None,
        profile: Optional[BuildProfile] = None,
        incremental: bool = False,
        benchmark_name: Optional[str] = None
    ) -> bool:
        """Build packages with optional profile and incremental support"""
        
        # Filter packages
        packages_to_build = self.packages
        
        if profile:
            packages_to_build = profile.filter_packages(packages_to_build)
            Logger.info(f"Using build profile: {profile.value}")
        
        if specific_package:
            packages_to_build = [p for p in packages_to_build if p.name == specific_package]
            if not packages_to_build:
                Logger.error(f"Package '{specific_package}' not found")
                return False
        
        # Get build order
        build_order = self.dep_graph.get_build_order()
        packages_to_build = sorted(
            packages_to_build,
            key=lambda p: build_order.index(p.name) if p.name in build_order else 999
        )
        
        # Filter for incremental build
        if incremental:
            original_count = len(packages_to_build)
            packages_to_build = [
                p for p in packages_to_build
                if not self._should_skip_package(p, config, incremental)
            ]
            skipped = original_count - len(packages_to_build)
            if skipped > 0:
                Logger.success(f"Incremental build: Skipping {skipped} unchanged packages")
        
        if not packages_to_build:
            Logger.success("All packages are up to date!")
            return True
        
        Logger.info(f"Building {len(packages_to_build)} package(s) in {config} mode")
        
        # Build packages
        start_time = time.time()
        
        if self.parallel and len(packages_to_build) > 1:
            results = self._build_parallel(packages_to_build, config)
        else:
            results = self._build_sequential(packages_to_build, config)
        
        total_time = time.time() - start_time
        
        # Update cache and incremental tracker
        for result in results:
            if result.success:
                self.cache.update_package(result.package, config)
                self.incremental_tracker.update(result.package)
        
        self._save_cache()
        
        # Record statistics
        self.stats_tracker.record_build(results)
        
        # Record benchmark if requested
        if benchmark_name:
            self.benchmark_mgr.record(benchmark_name, config, results)
        
        # Print summary
        self._print_summary(results, total_time)
        
        return all(r.success for r in results)
    
    def _build_sequential(self, packages: List[Package], config: str) -> List[BuildResult]:
        """Build packages sequentially"""
        results = []
        
        for i, package in enumerate(packages, 1):
            Logger.info(f"Building [{i}/{len(packages)}] {package.name}...")
            result = self._build_package(package, config)
            results.append(result)
            
            if not result.success:
                Logger.error(f"Build failed for {package.name}")
                if result.error_message:
                    Logger.error(result.error_message)
        
        return results
    
    def _build_parallel(self, packages: List[Package], config: str) -> List[BuildResult]:
        """Build packages in parallel (respecting dependencies)"""
        Logger.info("Building packages in parallel...")
        
        results = []
        build_order = self.dep_graph.get_build_order()
        completed = set()
        failed = set()
        
        # Build in waves based on dependencies
        while len(completed) + len(failed) < len(packages):
            # Find packages ready to build
            ready_packages = []
            for pkg in packages:
                if pkg.name in completed or pkg.name in failed:
                    continue
                
                deps_ready = all(dep in completed for dep in pkg.dependencies)
                if deps_ready:
                    ready_packages.append(pkg)
            
            if not ready_packages:
                break
            
            # Build ready packages in parallel
            with ThreadPoolExecutor(max_workers=os.cpu_count() or 4) as executor:
                futures = {
                    executor.submit(self._build_package, pkg, config): pkg
                    for pkg in ready_packages
                }
                
                for future in futures:
                    result = future.result()
                    results.append(result)
                    
                    if result.success:
                        completed.add(result.package.name)
                    else:
                        failed.add(result.package.name)
                        Logger.error(f"Build failed for {result.package.name}")
        
        return results
    
    def _build_package(self, package: Package, config: str) -> BuildResult:
        """Build a single package"""
        start_time = time.time()
        
        try:
            source_dir = package.source_directory
            build_dir = package.build_directory
            prefix_dir = package.prefix_directory
            
            if not os.path.exists(source_dir):
                return BuildResult(
                    package=package,
                    success=False,
                    duration=0,
                    error_message=f"Source directory not found: {source_dir}"
                )

            # Configure command
            configure_cmd = [
                "cmake",
                "-S", package.source_directory,
                "-B", package.build_directory,
                f"-DCMAKE_BUILD_TYPE={config}",
                f"-DCMAKE_INSTALL_PREFIX={package.prefix_directory}",
            ]
            
            # Add compiler settings
            if self.compiler_c:
                configure_cmd.append(f"-DCMAKE_C_COMPILER={self.compiler_c}")
            if self.compiler_cxx:
                configure_cmd.append(f"-DCMAKE_CXX_COMPILER={self.compiler_cxx}")
            
            # Add package options
            if package.options:
                configure_cmd.extend(package.options.split())
            
            Logger.debug(f"Configure: {' '.join(configure_cmd)}")
            
            result = subprocess.run(
                configure_cmd,
                capture_output=True,
                text=True
            )
            
            if result.returncode != 0:
                return BuildResult(
                    package=package,
                    success=False,
                    duration=time.time() - start_time,
                    error_message=result.stderr
                )
            
            # Build
            build_cmd = [
                "cmake",
                "--build", str(build_dir),
                "--config", config,
                "--parallel", str(os.cpu_count() or 4)
            ]
            
            Logger.debug(f"Build: {' '.join(build_cmd)}")
            
            result = subprocess.run(
                build_cmd,
                capture_output=True,
                text=True
            )
            
            if result.returncode != 0:
                return BuildResult(
                    package=package,
                    success=False,
                    duration=time.time() - start_time,
                    error_message=result.stderr
                )
            
            # Install
            install_cmd = [
                "cmake",
                "--install", str(build_dir),
                "--config", config
            ]
            
            Logger.debug(f"Install: {' '.join(install_cmd)}")
            
            result = subprocess.run(
                install_cmd,
                capture_output=True,
                text=True
            )
            
            duration = time.time() - start_time
            
            if result.returncode != 0:
                return BuildResult(
                    package=package,
                    success=False,
                    duration=duration,
                    error_message=result.stderr
                )
            
            Logger.success(f"Built {package.name} in {duration:.1f}s")
            
            return BuildResult(
                package=package,
                success=True,
                duration=duration
            )
            
        except Exception as e:
            return BuildResult(
                package=package,
                success=False,
                duration=time.time() - start_time,
                error_message=str(e)
            )
    
    def _print_summary(self, results: List[BuildResult], total_time: float):
        """Print build summary"""
        successful = sum(1 for r in results if r.success)
        failed = len(results) - successful
        
        print(f"\n{Color.BOLD}{'=' * 60}{Color.RESET}")
        print(f"{Color.BOLD}Build Summary:{Color.RESET}")
        print(f"  Total packages: {len(results)}")
        print(f"  {Color.GREEN}✓ Successful: {successful}{Color.RESET}")
        
        if failed > 0:
            print(f"  {Color.RED}✗ Failed: {failed}{Color.RESET}")
            
            print(f"\n{Color.RED}Failed packages:{Color.RESET}")
            for result in results:
                if not result.success:
                    print(f"  - {result.package.name}")
        
        print(f"  Total time: {timedelta(seconds=int(total_time))}")
        print(f"{Color.BOLD}{'=' * 60}{Color.RESET}\n")
    
    def clean(self, specific_package: Optional[str] = None):
        """Clean build artifacts"""
        packages = [p for p in self.packages if p.name == specific_package] if specific_package else self.packages
        
        for package in packages:
            build_dir = Path(package.build_directory)
            if build_dir.exists():
                Logger.info(f"Cleaning {package.name}...")
                shutil.rmtree(build_dir)
                Logger.success(f"Cleaned {package.name}")
        
        if not specific_package:
            # Clean cache files
            for cache_file in [BuildConfig.CACHE_FILE, BuildConfig.INCREMENTAL_CACHE]:
                path = Path(cache_file)
                if path.exists():
                    path.unlink()
                    Logger.success(f"Removed {cache_file}")
    
    def rebuild(
        self,
        config: str,
        specific_package: Optional[str] = None,
        profile: Optional[BuildProfile] = None,
        incremental: bool = False
    ) -> bool:
        """Clean and rebuild"""
        self.clean(specific_package)
        return self.build(config, specific_package, profile, incremental)
    
    def show_stats(self):
        """Show build statistics"""
        self.stats_tracker.print_stats()
    
    def compare_benchmarks(self, baseline: str, current: str):
        """Compare benchmarks"""
        self.benchmark_mgr.compare(baseline, current)


# ==================== Thread Pool for Parallel Builds ====================
from concurrent.futures import ThreadPoolExecutor


# ==================== Argument Parser ====================
def create_argument_parser() -> argparse.ArgumentParser:
    """Create argument parser with all commands"""
    parser = argparse.ArgumentParser(
        description="Motion Engine Enhanced Build System v" + BuildConfig.VERSION,
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Generate CMakePresets.json with proper compiler settings
  python Setup.py presets
  
  # Quick development build
  python Setup.py build --config Debug --profile quick --incremental --parallel
  
  # Full release build
  python Setup.py build --config Release --profile full
  
  # Build specific package
  python Setup.py build --config Debug --pkg imgui
  
  # Show statistics
  python Setup.py stats
  
  # Visualize dependencies
  python Setup.py graph --visualize
  
  # Setup version control system
  python Setup.py version-control
        """
    )
    
    parser.add_argument(
        "--version",
        action="version",
        version=f"Motion Build System v{BuildConfig.VERSION}"
    )
    
    subparsers = parser.add_subparsers(dest="command", help="Available commands")
    
    # Build command
    build_parser = subparsers.add_parser("build", help="Build packages")
    build_parser.add_argument(
        "--config",
        choices=BuildConfig.CONFIGS,
        required=True,
        help="Build configuration (Debug/Release/RelWithDebInfo/MinSizeRel)"
    )
    build_parser.add_argument(
        "--pkg",
        help="Specific package to build"
    )
    build_parser.add_argument(
        "--parallel",
        action="store_true",
        help="Build packages in parallel"
    )
    build_parser.add_argument(
        "--profile",
        choices=[p.value for p in BuildProfile],
        help="Use build profile (quick/full/minimal/dev/release)"
    )
    build_parser.add_argument(
        "--incremental",
        action="store_true",
        help="Incremental build - only rebuild changed packages"
    )
    build_parser.add_argument(
        "--benchmark",
        help="Record benchmark with given name"
    )

    build_parser.add_argument(
        "--c-compiler",
        type=str,
        help="Specify C compiler (e.g., gcc, clang, cl)"
    )
    build_parser.add_argument(
        "--cxx-compiler",
        type=str,
        help="Specify C++ compiler (e.g., g++, clang++, cl)"
    )
    build_parser.add_argument(
        "--compiler",
        type=str,
        help="Specify compiler (e.g., 'clang++' sets both clang and clang++)"
    )
    
    # Statistics command
    subparsers.add_parser("stats", help="Show build statistics")
    
    # Benchmark command
    benchmark_parser = subparsers.add_parser("benchmark", help="Benchmark operations")
    benchmark_parser.add_argument(
        "--compare",
        nargs=2,
        metavar=("BASELINE", "CURRENT"),
        help="Compare two benchmarks"
    )
    
    # Dependency graph command
    graph_parser = subparsers.add_parser("graph", help="Analyze dependencies")
    graph_parser.add_argument(
        "--visualize",
        action="store_true",
        help="Generate visual dependency graph"
    )
    graph_parser.add_argument(
        "--check-cycles",
        action="store_true",
        help="Check for circular dependencies"
    )
    graph_parser.add_argument(
        "--analyze",
        action="store_true",
        help="Print dependency analysis"
    )
    graph_parser.add_argument(
        "--output",
        type=Path,
        default=Path("build/dependency_graph.png"),
        help="Output path for graph visualization"
    )
    
    # Clean command
    clean_parser = subparsers.add_parser("clean", help="Clean build artifacts")
    clean_parser.add_argument(
        "--pkg",
        help="Specific package to clean"
    )
    
    # Rebuild command
    rebuild_parser = subparsers.add_parser("rebuild", help="Clean and rebuild")
    rebuild_parser.add_argument(
        "--config",
        choices=BuildConfig.CONFIGS,
        required=True,
        help="Build configuration"
    )
    rebuild_parser.add_argument(
        "--pkg",
        help="Specific package to rebuild"
    )
    rebuild_parser.add_argument(
        "--parallel",
        action="store_true",
        help="Build packages in parallel"
    )
    rebuild_parser.add_argument(
        "--profile",
        choices=[p.value for p in BuildProfile],
        help="Use build profile"
    )
    rebuild_parser.add_argument(
        "--incremental",
        action="store_true",
        help="Use incremental rebuild"
    )

    # Presets command
    presets_parser = subparsers.add_parser("presets", help="Generate CMakePresets.json with proper compiler settings")
    presets_parser.add_argument(
        "--c-compiler",
        type=str,
        help="Specify C compiler (e.g., gcc, clang, cl)"
    )
    presets_parser.add_argument(
        "--cxx-compiler",
        type=str,
        help="Specify C++ compiler (e.g., g++, clang++, cl)"
    )
    presets_parser.add_argument(
        "--compiler",
        type=str,
        help="Specify compiler (e.g., 'clang++' sets both clang and clang++)"
    )
    
    # List command
    subparsers.add_parser("list", help="List all packages")
    subparsers.add_parser("list-compilers", help="List available compilers")
    
    # Version control setup command
    subparsers.add_parser(
        "version-control",
        help="Setup version control system for Motion Engine"
    )
    
    # Global options
    parser.add_argument(
        "--verbose",
        action="store_true",
        help="Enable verbose logging"
    )
    parser.add_argument(
        "--log-file",
        type=Path,
        help="Write logs to file"
    )
    
    return parser


# ===================== Main Entry Point =====================
def main():
    """Main entry point"""
    parser = create_argument_parser()
    args = parser.parse_args()
    
    # Configure logging
    if hasattr(args, 'verbose') and args.verbose:
        Logger.set_level(LogLevel.DEBUG)
    if hasattr(args, 'log_file') and args.log_file:
        Logger.set_log_file(args.log_file)
    
    # Print banner
    print(f"\n{Color.BOLD}{Color.CYAN}{'=' * 60}{Color.RESET}")
    print(f"{Color.BOLD}Motion Engine Build System v{BuildConfig.VERSION}{Color.RESET}")
    print(f"{Color.BOLD}{Color.CYAN}{'=' * 60}{Color.RESET}\n")
    
    try:
        # Handle version-control command separately (doesn't need full validation)
        if args.command == "version-control":
            success = VersionControlSetup.setup()
            sys.exit(0 if success else 1)
        
        # System validation for other commands
        SystemValidator.check_cmake()
        SystemValidator.check_compiler()
        SystemValidator.check_disk_space()
        
        # Get package registry
        packages = get_package_registry()
        
        # Execute command
        if args.command == "build":
            # Parse compiler arguments
            c_compiler = None
            cxx_compiler = None
            
            if hasattr(args, 'c_compiler') and args.c_compiler:
                c_compiler = args.c_compiler
            if hasattr(args, 'cxx_compiler') and args.cxx_compiler:
                cxx_compiler = args.cxx_compiler
            
            # If --compiler is specified, derive both
            if hasattr(args, 'compiler') and args.compiler:
                compiler = args.compiler
                if not c_compiler:
                    # Derive C compiler from C++
                    if compiler == "g++":
                        c_compiler = "gcc"
                    elif compiler == "clang++":
                        c_compiler = "clang"
                    else:
                        c_compiler = compiler
                if not cxx_compiler:
                    cxx_compiler = compiler
            
            # Log compiler selection
            if c_compiler or cxx_compiler:
                Logger.info(f"Using compilers: C={c_compiler or 'default'}, C++={cxx_compiler or 'default'}")
            
            manager = BuildManager(packages, parallel=args.parallel,
                                 compiler_c=c_compiler, compiler_cxx=cxx_compiler)
            
            profile = None
            if hasattr(args, 'profile') and args.profile:
                profile = BuildProfile(args.profile)
            
            success = manager.build(
                args.config,
                args.pkg if hasattr(args, 'pkg') else None,
                profile=profile,
                incremental=getattr(args, 'incremental', False),
                benchmark_name=getattr(args, 'benchmark', None)
            )
            sys.exit(0 if success else 1)
        
        elif args.command == "stats":
            manager = BuildManager(packages)
            manager.show_stats()
        
        elif args.command == "benchmark":
            if args.compare:
                manager = BuildManager(packages)
                manager.compare_benchmarks(args.compare[0], args.compare[1])
        
        elif args.command == "graph":
            graph = DependencyGraph(packages)
            
            if args.analyze:
                graph.print_analysis()
            
            if args.check_cycles:
                cycles = graph.find_circular_dependencies()
                if cycles:
                    Logger.error(f"Found {len(cycles)} circular dependencies:")
                    for cycle in cycles:
                        Logger.error(f"  {' → '.join(cycle)}")
                    sys.exit(1)
                else:
                    Logger.success("No circular dependencies found")
            
            if args.visualize:
                graph.visualize(args.output)
        
        elif args.command == "rebuild":
            # Parse compiler arguments (same as build)
            c_compiler = None
            cxx_compiler = None
            
            if hasattr(args, 'c_compiler') and args.c_compiler:
                c_compiler = args.c_compiler
            if hasattr(args, 'cxx_compiler') and args.cxx_compiler:
                cxx_compiler = args.cxx_compiler
            
            if hasattr(args, 'compiler') and args.compiler:
                compiler = args.compiler
                if not c_compiler:
                    if compiler == "g++":
                        c_compiler = "gcc"
                    elif compiler == "clang++":
                        c_compiler = "clang"
                    else:
                        c_compiler = compiler
                if not cxx_compiler:
                    cxx_compiler = compiler
            
            # Log compiler selection
            if c_compiler or cxx_compiler:
                Logger.info(f"Using compilers: C={c_compiler or 'default'}, C++={cxx_compiler or 'default'}")
            
            manager = BuildManager(packages, parallel=args.parallel,
                                 compiler_c=c_compiler, compiler_cxx=cxx_compiler)
            
            profile = None
            if hasattr(args, 'profile') and args.profile:
                profile = BuildProfile(args.profile)
            
            success = manager.rebuild(
                args.config,
                args.pkg if hasattr(args, 'pkg') else None,
                profile=profile,
                incremental=getattr(args, 'incremental', False)
            )
            sys.exit(0 if success else 1)
        
        elif args.command == "presets":
            # Determine compilers from arguments
            c_compiler = None
            cxx_compiler = None
            
            if hasattr(args, 'c_compiler') and args.c_compiler:
                c_compiler = args.c_compiler
            if hasattr(args, 'cxx_compiler') and args.cxx_compiler:
                cxx_compiler = args.cxx_compiler
            
            # If --compiler is specified, derive both
            if hasattr(args, 'compiler') and args.compiler:
                compiler = args.compiler
                if not c_compiler:
                    # Derive C compiler from C++
                    if compiler == "g++":
                        c_compiler = "gcc"
                    elif compiler == "clang++":
                        c_compiler = "clang"
                    else:
                        c_compiler = compiler
                if not cxx_compiler:
                    cxx_compiler = compiler
            
            generator = PresetGenerator(packages, compiler_c=c_compiler, compiler_cxx=cxx_compiler)
            generator.generate(Path("."))
        
        elif args.command == "list":
            print(f"\n{Color.BOLD}Available Packages:{Color.RESET}\n")
            for pkg in packages:
                status = f"{Color.GREEN}✓{Color.RESET}" if pkg.enabled else f"{Color.RED}✗{Color.RESET}"
                deps = f" (deps: {', '.join(pkg.dependencies)})" if pkg.dependencies else ""
                print(f"  {status} {pkg.name:20} {deps}")
            print()
        
        elif args.command == "list-compilers":
            detect_and_list_compilers()
        
        else:
            parser.print_help()
    
    except BuildException as e:
        Logger.error(str(e))
        sys.exit(1)
    except KeyboardInterrupt:
        Logger.warn("\nBuild interrupted by user")
        sys.exit(130)
    except Exception as e:
        Logger.error(f"Unexpected error: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)


if __name__ == "__main__":
    main()