#!/usr/bin/env python3

"""
Motion Engine Build System
Version: 2.2.0
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
from typing import List, Dict, Optional, Set
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
    VERSION = "2.2.0"
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


# ==================== NEW: Build Profiles ====================
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


# ==================== NEW: Version Control Setup ====================
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
    message(STATUS " Build Type : ${CMAKE_BUILD_TYPE}")
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

    "cmake/MotionVersion.hpp.in": 
    
r"""

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
    inline constexpr const char* BUILD_TYPE         = "@CMAKE_BUILD_TYPE@";
    
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

#define MOTION_VERSION Motion::Version::VERSION
#define MOTION_VERSION_MAJOR Motion::Version::MAJOR
#define MOTION_VERSION_MINOR Motion::Version::MINOR
#define MOTION_VERSION_PATCH Motion::Version::PATCH
#define MOTION_GIT_HASH Motion::Version::GIT_COMMIT_HASH
#define MOTION_BUILD_TYPE Motion::Version::BUILD_TYPE

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
            print(f"   {Color.CYAN}Motion::Version::GetFullVersion(){Color.RESET}\n")
            
            print(f"{Color.BOLD}Usage Example:{Color.RESET}")
            print(f"{Color.DIM}#include <MotionVersion.hpp>")
            print(f"#include <iostream>")
            print()
            print(f"int main() {{")
            print(f"    std::cout << Motion::Version::GetDetailedVersion();")
            print(f"    return 0;")
            print(f"}}{Color.RESET}\n")
            
            return True
            
        except Exception as e:
            Logger.error(f"Failed to setup version control: {e}")
            import traceback
            traceback.print_exc()
            return False



# ==================== System Validators ====================
class SystemValidator:
    """Validates system requirements"""
    
    @staticmethod
    def check_cmake() -> bool:
        """Check CMake installation and version"""
        try:
            result = subprocess.run(
                ["cmake", "--version"],
                capture_output=True,
                text=True,
                check=True
            )
            
            version_match = re.search(r'cmake version (\d+)\.(\d+)\.(\d+)', result.stdout)
            if version_match:
                version = tuple(map(int, version_match.groups()))
                if version >= BuildConfig.MIN_CMAKE_VERSION:
                    Logger.success(f"CMake {'.'.join(map(str, version))} found")
                    return True
                else:
                    raise ConfigurationException(
                        f"CMake {'.'.join(map(str, version))} found, but "
                        f"{'.'.join(map(str, BuildConfig.MIN_CMAKE_VERSION))} required"
                    )
            
            raise ConfigurationException("Could not determine CMake version")
            
        except FileNotFoundError:
            raise ConfigurationException("CMake not found. Please install CMake.")
        except subprocess.CalledProcessError:
            raise ConfigurationException("Failed to run CMake")
    
    @staticmethod
    def check_compiler() -> bool:
        """Check for available C++ compiler"""
        compilers = {
            "Windows": ["cl", "g++", "clang++"],
            "Linux": ["g++", "clang++"],
            "Darwin": ["clang++", "g++"]
        }
        
        system = platform.system()
        available = []
        
        for compiler in compilers.get(system, ["g++", "clang++"]):
            try:
                result = subprocess.run(
                    [compiler, "--version"],
                    capture_output=True,
                    text=True,
                    timeout=5
                )
                if result.returncode == 0:
                    available.append(compiler)
            except (FileNotFoundError, subprocess.TimeoutExpired):
                continue
        
        if available:
            Logger.success(f"Found compiler(s): {', '.join(available)}")
            return True
        else:
            raise ConfigurationException(
                f"No C++ compiler found. Please install a compiler for {system}"
            )
    
    @staticmethod
    def check_disk_space(min_gb: float = 5.0) -> bool:
        """Check available disk space"""
        try:
            stat = shutil.disk_usage(".")
            available_gb = stat.free / (1024 ** 3)
            
            if available_gb >= min_gb:
                Logger.debug(f"Available disk space: {available_gb:.2f} GB")
                return True
            else:
                Logger.warn(
                    f"Low disk space: {available_gb:.2f} GB available "
                    f"(recommended: {min_gb:.2f} GB)"
                )
                return True  # Warning, not error
                
        except Exception as e:
            Logger.warn(f"Could not check disk space: {e}")
            return True


# ==================== Package Registry ====================
def get_package_registry() -> List[Package]:
    """Central registry of all packages to build"""
    
    packages = [
        Package(
            name="glfw",
            source_directory="vendor/glfw",
            build_directory="build/glfw",
            prefix_directory="build/install",
            options="-DGLFW_BUILD_EXAMPLES=OFF -DGLFW_BUILD_TESTS=OFF -DGLFW_BUILD_DOCS=OFF"
        ),
        Package(
            name="glad",
            source_directory="vendor/glad",
            build_directory="build/glad",
            prefix_directory="build/install"
        ),
        Package(
            name="glm",
            source_directory="vendor/glm",
            build_directory="build/glm",
            prefix_directory="build/install",
            options="-DGLM_BUILD_TESTS=OFF"
        ),
        Package(
            name="imgui",
            source_directory="vendor/imgui",
            build_directory="build/imgui",
            prefix_directory="build/install",
            dependencies=["glfw", "glad"]
        ),
        Package(
            name="entt",
            source_directory="vendor/entt",
            build_directory="build/entt",
            prefix_directory="build/install"
        ),
        Package(
            name="stb",
            source_directory="vendor/stb",
            build_directory="build/stb",
            prefix_directory="build/install"
        ),
        Package(
            name="ReactPhysics3D",
            source_directory="vendor/reactphysics3d",
            build_directory="build/reactphysics3d",
            prefix_directory="build/install",
            options="-DRP3D_COMPILE_TESTS=OFF"
        ),
        Package(
            name="spdlog",
            source_directory="vendor/spdlog",
            build_directory="build/spdlog",
            prefix_directory="build/install"
        ),
        Package(
            name="assimp",
            source_directory="vendor/assimp",
            build_directory="build/assimp",
            prefix_directory="build/install",
            options="-DASSIMP_BUILD_TESTS=OFF -DASSIMP_BUILD_ASSIMP_TOOLS=OFF"
        )
    ]
    
    return packages


# ==================== Incremental Build System ====================
class IncrementalBuildTracker:
    """Tracks file changes for incremental builds"""
    
    def __init__(self, cache_file: Path = Path(BuildConfig.INCREMENTAL_CACHE)):
        self.cache_file = cache_file
        self.cache: Dict[str, Dict[str, str]] = self._load_cache()
    
    def _load_cache(self) -> Dict[str, Dict[str, str]]:
        """Load cache from disk"""
        if self.cache_file.exists():
            try:
                with open(self.cache_file, 'r') as f:
                    return json.load(f)
            except Exception as e:
                Logger.warn(f"Failed to load incremental cache: {e}")
        return {}
    
    def _save_cache(self):
        """Save cache to disk"""
        self.cache_file.parent.mkdir(parents=True, exist_ok=True)
        with open(self.cache_file, 'w') as f:
            json.dump(self.cache, f, indent=2)
    
    def _hash_directory(self, directory: Path) -> Dict[str, str]:
        """Calculate hashes for all source files in directory"""
        hashes = {}
        
        if not directory.exists():
            return hashes
        
        for ext in ['.cpp', '.hpp', '.h', '.c', '.cc', '.cxx']:
            for file_path in directory.rglob(f'*{ext}'):
                try:
                    with open(file_path, 'rb') as f:
                        file_hash = hashlib.md5(f.read()).hexdigest()
                        relative_path = str(file_path.relative_to(directory))
                        hashes[relative_path] = file_hash
                except Exception as e:
                    Logger.debug(f"Failed to hash {file_path}: {e}")
        
        return hashes
    
    def has_changes(self, package: Package) -> bool:
        """Check if package has changes since last build"""
        source_dir = Path(package.source_directory)
        current_hashes = self._hash_directory(source_dir)
        cached_hashes = self.cache.get(package.name, {})
        
        # If no cache exists, assume changes
        if not cached_hashes:
            Logger.debug(f"{package.name}: No cache found, rebuilding")
            return True
        
        # Compare hashes
        if current_hashes != cached_hashes:
            # Find what changed
            added = set(current_hashes.keys()) - set(cached_hashes.keys())
            removed = set(cached_hashes.keys()) - set(current_hashes.keys())
            modified = {
                f for f in current_hashes.keys() & cached_hashes.keys()
                if current_hashes[f] != cached_hashes[f]
            }
            
            if added:
                Logger.debug(f"{package.name}: Added files: {added}")
            if removed:
                Logger.debug(f"{package.name}: Removed files: {removed}")
            if modified:
                Logger.debug(f"{package.name}: Modified files: {modified}")
            
            return True
        
        Logger.debug(f"{package.name}: No changes detected")
        return False
    
    def update(self, package: Package):
        """Update cache for package"""
        source_dir = Path(package.source_directory)
        self.cache[package.name] = self._hash_directory(source_dir)
        self._save_cache()


# ==================== Build Statistics ====================
@dataclass
class BuildStatistics:
    """Statistics for build operations"""
    total_builds: int = 0
    successful_builds: int = 0
    failed_builds: int = 0
    total_time: float = 0.0
    avg_time: float = 0.0
    package_times: Dict[str, float] = field(default_factory=dict)
    last_build: Optional[str] = None
    build_history: List[Dict] = field(default_factory=list)


class StatisticsTracker:
    """Tracks and persists build statistics"""
    
    def __init__(self, stats_file: Path = Path(BuildConfig.STATS_FILE)):
        self.stats_file = stats_file
        self.stats = self._load_stats()
    
    def _load_stats(self) -> BuildStatistics:
        """Load statistics from disk"""
        if self.stats_file.exists():
            try:
                with open(self.stats_file, 'r') as f:
                    data = json.load(f)
                    return BuildStatistics(**data)
            except Exception as e:
                Logger.warn(f"Failed to load statistics: {e}")
        return BuildStatistics()
    
    def _save_stats(self):
        """Save statistics to disk"""
        self.stats_file.parent.mkdir(parents=True, exist_ok=True)
        with open(self.stats_file, 'w') as f:
            json.dump(asdict(self.stats), f, indent=2)
    
    def record_build(self, results: List[BuildResult]):
        """Record build results"""
        self.stats.total_builds += 1
        
        successful = sum(1 for r in results if r.success)
        failed = len(results) - successful
        
        self.stats.successful_builds += successful
        self.stats.failed_builds += failed
        
        total_time = sum(r.duration for r in results)
        self.stats.total_time += total_time
        
        if self.stats.total_builds > 0:
            self.stats.avg_time = self.stats.total_time / self.stats.total_builds
        
        # Update package times
        for result in results:
            pkg_name = result.package.name
            if pkg_name not in self.stats.package_times:
                self.stats.package_times[pkg_name] = result.duration
            else:
                # Running average
                self.stats.package_times[pkg_name] = (
                    self.stats.package_times[pkg_name] + result.duration
                ) / 2
        
        self.stats.last_build = datetime.now().isoformat()
        
        # Add to history (keep last 100 builds)
        self.stats.build_history.append({
            'timestamp': datetime.now().isoformat(),
            'duration': total_time,
            'successful': successful,
            'failed': failed,
            'packages': [r.package.name for r in results]
        })
        
        if len(self.stats.build_history) > 100:
            self.stats.build_history = self.stats.build_history[-100:]
        
        self._save_stats()
    
    def display(self):
        """Display statistics"""
        print(f"\n{Color.BOLD}Build Statistics:{Color.RESET}\n")
        print(f"  Total Builds:       {self.stats.total_builds}")
        print(f"  Successful:         {Color.GREEN}{self.stats.successful_builds}{Color.RESET}")
        print(f"  Failed:             {Color.RED}{self.stats.failed_builds}{Color.RESET}")
        print(f"  Success Rate:       {self._success_rate():.1f}%")
        print(f"  Total Build Time:   {self._format_duration(self.stats.total_time)}")
        print(f"  Average Build Time: {self._format_duration(self.stats.avg_time)}")
        
        if self.stats.last_build:
            last_build = datetime.fromisoformat(self.stats.last_build)
            print(f"  Last Build:         {last_build.strftime('%Y-%m-%d %H:%M:%S')}")
        
        if self.stats.package_times:
            print(f"\n{Color.BOLD}Package Build Times:{Color.RESET}")
            sorted_packages = sorted(
                self.stats.package_times.items(),
                key=lambda x: x[1],
                reverse=True
            )
            for pkg, time in sorted_packages:
                print(f"  {pkg:20} {self._format_duration(time)}")
        
        print()
    
    def _success_rate(self) -> float:
        """Calculate success rate"""
        total = self.stats.successful_builds + self.stats.failed_builds
        if total == 0:
            return 0.0
        return (self.stats.successful_builds / total) * 100
    
    @staticmethod
    def _format_duration(seconds: float) -> str:
        """Format duration in human-readable format"""
        if seconds < 60:
            return f"{seconds:.2f}s"
        elif seconds < 3600:
            minutes = int(seconds // 60)
            secs = seconds % 60
            return f"{minutes}m {secs:.0f}s"
        else:
            hours = int(seconds // 3600)
            minutes = int((seconds % 3600) // 60)
            return f"{hours}h {minutes}m"


# ==================== Benchmark System ====================
@dataclass
class Benchmark:
    """Benchmark data for a build"""
    name: str
    timestamp: str
    config: str
    total_duration: float
    package_durations: Dict[str, float]
    system_info: Dict[str, str]


class BenchmarkManager:
    """Manages build benchmarks"""
    
    def __init__(self, benchmark_file: Path = Path(BuildConfig.BENCHMARK_FILE)):
        self.benchmark_file = benchmark_file
        self.benchmarks: Dict[str, Benchmark] = self._load_benchmarks()
    
    def _load_benchmarks(self) -> Dict[str, Benchmark]:
        """Load benchmarks from disk"""
        if self.benchmark_file.exists():
            try:
                with open(self.benchmark_file, 'r') as f:
                    data = json.load(f)
                    return {
                        name: Benchmark(**bench_data)
                        for name, bench_data in data.items()
                    }
            except Exception as e:
                Logger.warn(f"Failed to load benchmarks: {e}")
        return {}
    
    def _save_benchmarks(self):
        """Save benchmarks to disk"""
        self.benchmark_file.parent.mkdir(parents=True, exist_ok=True)
        with open(self.benchmark_file, 'w') as f:
            data = {
                name: asdict(bench)
                for name, bench in self.benchmarks.items()
            }
            json.dump(data, f, indent=2)
    
    def record(self, name: str, config: str, results: List[BuildResult]):
        """Record a benchmark"""
        package_durations = {
            r.package.name: r.duration
            for r in results
        }
        
        benchmark = Benchmark(
            name=name,
            timestamp=datetime.now().isoformat(),
            config=config,
            total_duration=sum(package_durations.values()),
            package_durations=package_durations,
            system_info={
                'platform': platform.system(),
                'processor': platform.processor(),
                'python_version': platform.python_version()
            }
        )
        
        self.benchmarks[name] = benchmark
        self._save_benchmarks()
        Logger.success(f"Recorded benchmark '{name}'")
    
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
        
        print(f"\n{Color.BOLD}Benchmark Comparison:{Color.RESET}")
        print(f"  Baseline: {baseline} ({base.timestamp})")
        print(f"  Current:  {current} ({curr.timestamp})")
        print()
        
        # Total time comparison
        total_diff = curr.total_duration - base.total_duration
        total_pct = (total_diff / base.total_duration) * 100
        
        color = Color.GREEN if total_diff < 0 else Color.RED
        print(f"  Total Time:")
        print(f"    Baseline: {base.total_duration:.2f}s")
        print(f"    Current:  {curr.total_duration:.2f}s")
        print(f"    Change:   {color}{total_diff:+.2f}s ({total_pct:+.1f}%){Color.RESET}")
        print()
        
        # Package-by-package comparison
        print(f"  {Color.BOLD}Package Breakdown:{Color.RESET}")
        all_packages = set(base.package_durations.keys()) | set(curr.package_durations.keys())
        
        for pkg in sorted(all_packages):
            base_time = base.package_durations.get(pkg, 0)
            curr_time = curr.package_durations.get(pkg, 0)
            
            if base_time == 0:
                print(f"    {pkg:20} NEW: {curr_time:.2f}s")
            elif curr_time == 0:
                print(f"    {pkg:20} REMOVED")
            else:
                diff = curr_time - base_time
                pct = (diff / base_time) * 100
                color = Color.GREEN if diff < 0 else Color.RED
                print(f"    {pkg:20} {base_time:6.2f}s → {curr_time:6.2f}s "
                      f"{color}({diff:+.2f}s, {pct:+.1f}%){Color.RESET}")
        
        print()


# ==================== Dependency Graph ====================
class DependencyGraph:
    """Analyzes and visualizes package dependencies"""
    
    def __init__(self, packages: List[Package]):
        self.packages = packages
        self.graph = self._build_graph()
    
    def _build_graph(self) -> Dict[str, Set[str]]:
        """Build adjacency list representation"""
        graph = {pkg.name: set(pkg.dependencies) for pkg in self.packages}
        return graph
    
    def get_build_order(self) -> List[str]:
        """Topological sort for build order"""
        visited = set()
        stack = []
        
        def visit(node: str):
            if node in visited:
                return
            visited.add(node)
            for dep in self.graph.get(node, []):
                visit(dep)
            stack.append(node)
        
        for pkg in self.graph:
            visit(pkg)
        
        return stack
    
    def find_circular_dependencies(self) -> List[List[str]]:
        """Detect circular dependencies"""
        def dfs(node: str, visited: Set[str], rec_stack: Set[str], path: List[str]) -> Optional[List[str]]:
            visited.add(node)
            rec_stack.add(node)
            path.append(node)
            
            for neighbor in self.graph.get(node, []):
                if neighbor not in visited:
                    cycle = dfs(neighbor, visited, rec_stack, path[:])
                    if cycle:
                        return cycle
                elif neighbor in rec_stack:
                    # Found cycle
                    cycle_start = path.index(neighbor)
                    return path[cycle_start:] + [neighbor]
            
            rec_stack.remove(node)
            return None
        
        visited = set()
        cycles = []
        
        for node in self.graph:
            if node not in visited:
                cycle = dfs(node, visited, set(), [])
                if cycle:
                    cycles.append(cycle)
        
        return cycles
    
    def visualize(self, output_path: Path):
        """Generate dependency graph visualization"""
        if not HAS_NETWORKX or not HAS_MATPLOTLIB:
            Logger.error("Visualization requires networkx and matplotlib")
            Logger.info("Install with: pip install networkx matplotlib")
            return
        
        # Create directed graph
        G = nx.DiGraph()
        
        for pkg, deps in self.graph.items():
            G.add_node(pkg)
            for dep in deps:
                G.add_edge(pkg, dep)
        
        # Layout and draw
        plt.figure(figsize=(12, 8))
        pos = nx.spring_layout(G, k=2, iterations=50)
        
        # Draw nodes
        nx.draw_networkx_nodes(
            G, pos,
            node_color='lightblue',
            node_size=3000,
            alpha=0.9
        )
        
        # Draw edges
        nx.draw_networkx_edges(
            G, pos,
            edge_color='gray',
            arrows=True,
            arrowsize=20,
            arrowstyle='->',
            connectionstyle='arc3,rad=0.1'
        )
        
        # Draw labels
        nx.draw_networkx_labels(
            G, pos,
            font_size=10,
            font_weight='bold'
        )
        
        plt.title("Motion Engine Package Dependencies", fontsize=16, fontweight='bold')
        plt.axis('off')
        plt.tight_layout()
        
        output_path.parent.mkdir(parents=True, exist_ok=True)
        plt.savefig(output_path, dpi=300, bbox_inches='tight')
        Logger.success(f"Dependency graph saved to {output_path}")
    
    def print_analysis(self):
        """Print detailed dependency analysis"""
        print(f"\n{Color.BOLD}Dependency Analysis:{Color.RESET}\n")
        
        # Build order
        build_order = self.get_build_order()
        print(f"  {Color.BOLD}Recommended Build Order:{Color.RESET}")
        for i, pkg in enumerate(build_order, 1):
            deps = self.graph[pkg]
            dep_str = f" (depends on: {', '.join(deps)})" if deps else " (no dependencies)"
            print(f"    {i}. {pkg}{dep_str}")
        
        # Statistics
        print(f"\n  {Color.BOLD}Statistics:{Color.RESET}")
        print(f"    Total Packages:     {len(self.packages)}")
        
        independent = sum(1 for deps in self.graph.values() if not deps)
        print(f"    Independent:        {independent}")
        
        max_deps = max(len(deps) for deps in self.graph.values())
        most_dependent = [pkg for pkg, deps in self.graph.items() if len(deps) == max_deps]
        print(f"    Max Dependencies:   {max_deps} ({', '.join(most_dependent)})")
        
        # Reverse dependencies
        reverse_deps = {pkg: [] for pkg in self.graph}
        for pkg, deps in self.graph.items():
            for dep in deps:
                reverse_deps[dep].append(pkg)
        
        max_rdeps = max(len(rdeps) for rdeps in reverse_deps.values())
        most_depended = [pkg for pkg, rdeps in reverse_deps.items() if len(rdeps) == max_rdeps]
        print(f"    Most Depended On:   {', '.join(most_depended)} ({max_rdeps} packages)")
        
        print()


# ==================== Build Manager ====================
class BuildManager:
    """Manages the build process for all packages"""
    
    def __init__(self, packages: List[Package], parallel: bool = False):
        self.packages = packages
        self.parallel = parallel
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
                Logger.error(f"Failed to build {package.name}")
                if result.error_message:
                    Logger.error(result.error_message)
        
        return results
    
    def _build_parallel(self, packages: List[Package], config: str) -> List[BuildResult]:
        """Build packages in parallel"""
        Logger.info("Building packages in parallel...")
        
        result_queue: queue.Queue = queue.Queue()
        threads = []
        
        def worker(pkg: Package):
            result = self._build_package(pkg, config)
            result_queue.put(result)
        
        # Start threads
        for package in packages:
            thread = threading.Thread(target=worker, args=(package,))
            thread.start()
            threads.append(thread)
        
        # Wait for completion
        for thread in threads:
            thread.join()
        
        # Collect results
        results = []
        while not result_queue.empty():
            results.append(result_queue.get())
        
        return results
    
    def _build_package(self, package: Package, config: str) -> BuildResult:
        """Build a single package"""
        start_time = time.time()
        
        try:
            # Create directories
            Path(package.build_directory).mkdir(parents=True, exist_ok=True)
            Path(package.prefix_directory).mkdir(parents=True, exist_ok=True)
            
            # Configure
            configure_cmd = [
                "cmake",
                f"-S{package.source_directory}",
                f"-B{package.build_directory}",
                f"-DCMAKE_BUILD_TYPE={config}",
                f"-DCMAKE_INSTALL_PREFIX={package.prefix_directory}",
                f"-DCMAKE_CXX_STANDARD={BuildConfig.CXX_STANDARD}",
                f"-DCMAKE_C_STANDARD={BuildConfig.C_STANDARD}"
            ]
            
            if package.options:
                configure_cmd.extend(package.options.split())
            
            Logger.debug(f"Configure: {' '.join(configure_cmd)}")
            
            result = subprocess.run(
                configure_cmd,
                capture_output=True,
                text=True,
                timeout=300
            )
            
            if result.returncode != 0:
                return BuildResult(
                    package=package,
                    success=False,
                    duration=time.time() - start_time,
                    error_message=f"Configuration failed:\n{result.stderr}"
                )
            
            # Build
            build_cmd = [
                "cmake",
                "--build",
                package.build_directory,
                "--config",
                config,
                "--parallel"
            ]
            
            Logger.debug(f"Build: {' '.join(build_cmd)}")
            
            result = subprocess.run(
                build_cmd,
                capture_output=True,
                text=True,
                timeout=600
            )
            
            if result.returncode != 0:
                return BuildResult(
                    package=package,
                    success=False,
                    duration=time.time() - start_time,
                    error_message=f"Build failed:\n{result.stderr}"
                )
            
            # Install
            install_cmd = [
                "cmake",
                "--install",
                package.build_directory,
                "--config",
                config
            ]
            
            Logger.debug(f"Install: {' '.join(install_cmd)}")
            
            result = subprocess.run(
                install_cmd,
                capture_output=True,
                text=True,
                timeout=300
            )
            
            if result.returncode != 0:
                return BuildResult(
                    package=package,
                    success=False,
                    duration=time.time() - start_time,
                    error_message=f"Installation failed:\n{result.stderr}"
                )
            
            duration = time.time() - start_time
            Logger.success(f"Built {package.name} in {duration:.2f}s")
            
            return BuildResult(
                package=package,
                success=True,
                duration=duration
            )
            
        except subprocess.TimeoutExpired:
            return BuildResult(
                package=package,
                success=False,
                duration=time.time() - start_time,
                error_message="Build timeout"
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
        successful = [r for r in results if r.success]
        failed = [r for r in results if not r.success]
        
        print(f"\n{Color.BOLD}{'=' * 60}{Color.RESET}")
        print(f"{Color.BOLD}Build Summary{Color.RESET}")
        print(f"{Color.BOLD}{'=' * 60}{Color.RESET}\n")
        
        print(f"  Total Time:     {total_time:.2f}s")
        print(f"  Successful:     {Color.GREEN}{len(successful)}{Color.RESET}")
        print(f"  Failed:         {Color.RED}{len(failed)}{Color.RESET}")
        
        if successful:
            print(f"\n  {Color.GREEN}✓{Color.RESET} {Color.BOLD}Successful Packages:{Color.RESET}")
            for result in successful:
                print(f"    • {result.package.name:20} ({result.duration:.2f}s)")
        
        if failed:
            print(f"\n  {Color.RED}✗{Color.RESET} {Color.BOLD}Failed Packages:{Color.RESET}")
            for result in failed:
                print(f"    • {result.package.name}")
        
        print()
    
    def clean(self, specific_package: Optional[str] = None):
        """Clean build artifacts"""
        packages = self.packages
        if specific_package:
            packages = [p for p in packages if p.name == specific_package]
        
        Logger.info(f"Cleaning {len(packages)} package(s)...")
        
        for package in packages:
            build_dir = Path(package.build_directory)
            if build_dir.exists():
                shutil.rmtree(build_dir)
                Logger.success(f"Cleaned {package.name}")
        
        # Clear cache
        self.cache = BuildCache()
        self._save_cache()
        
        # Clear incremental cache
        cache_file = Path(BuildConfig.INCREMENTAL_CACHE)
        if cache_file.exists():
            cache_file.unlink()
        
        Logger.success("Build artifacts cleaned")
    
    def rebuild(
        self,
        config: str,
        specific_package: Optional[str] = None,
        profile: Optional[BuildProfile] = None,
        incremental: bool = False
    ) -> bool:
        """Clean and rebuild packages"""
        self.clean(specific_package)
        return self.build(config, specific_package, profile, incremental)
    
    def show_stats(self):
        """Display build statistics"""
        self.stats_tracker.display()
    
    def compare_benchmarks(self, baseline: str, current: str):
        """Compare two benchmarks"""
        self.benchmark_mgr.compare(baseline, current)


# ==================== CMake Presets Generator ====================
class PresetGenerator:
    """Generates CMakePresets.json for easier configuration"""
    
    def __init__(self, packages: List[Package]):
        self.packages = packages
    
    def generate(self, project_root: Path):
        """Generate CMakePresets.json"""
        presets = {
            "version": 3,
            "cmakeMinimumRequired": {
                "major": BuildConfig.MIN_CMAKE_VERSION[0],
                "minor": BuildConfig.MIN_CMAKE_VERSION[1],
                "patch": BuildConfig.MIN_CMAKE_VERSION[2]
            },
            "configurePresets": self._generate_configure_presets(),
            "buildPresets": self._generate_build_presets()
        }
        
        output_file = project_root / "CMakePresets.json"
        with open(output_file, 'w') as f:
            json.dump(presets, f, indent=2)
        
        Logger.success(f"Generated {output_file}")
    
    def _generate_configure_presets(self) -> List[Dict]:
        """Generate configure presets"""
        presets = []
        
        for config in BuildConfig.CONFIGS:
            presets.append({
                "name": config.lower(),
                "displayName": f"{config} Build",
                "description": f"Configure for {config}",
                "binaryDir": "${sourceDir}/build",
                "cacheVariables": {
                    "CMAKE_BUILD_TYPE": config,
                    "CMAKE_CXX_STANDARD": BuildConfig.CXX_STANDARD,
                    "CMAKE_C_STANDARD": BuildConfig.C_STANDARD,
                    "CMAKE_EXPORT_COMPILE_COMMANDS": "ON"
                }
            })
        
        return presets
    
    def _generate_build_presets(self) -> List[Dict]:
        """Generate build presets"""
        presets = []
        
        for config in BuildConfig.CONFIGS:
            presets.append({
                "name": config.lower(),
                "configurePreset": config.lower(),
                "configuration": config
            })
        
        return presets


# ==================== Argument Parser ====================
def create_argument_parser() -> argparse.ArgumentParser:
    """Create argument parser with all commands"""
    parser = argparse.ArgumentParser(
        description="Motion Engine Enhanced Build System",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
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
  
  # Setup version control
  python Setup.py version-control
        """
    )
    
    parser.add_argument(
        "--version",
        action="version",
        version=f"Motion Build System v{BuildConfig.VERSION}"
    )
    
    subparsers = parser.add_subparsers(dest="command", help="Command to execute")
    
    # Enhanced build command
    build_parser = subparsers.add_parser("build", help="Build packages")
    build_parser.add_argument(
        "--config",
        choices=BuildConfig.CONFIGS,
        required=True,
        help="Build configuration"
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
    subparsers.add_parser("presets", help="Generate CMakePresets.json")
    
    # List command
    subparsers.add_parser("list", help="List all packages")
    
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
    """Enhanced main entry point"""
    parser = create_argument_parser()
    args = parser.parse_args()
    
    # Configure logging
    if hasattr(args, 'verbose') and args.verbose:
        Logger.set_level(LogLevel.DEBUG)
    if hasattr(args, 'log_file') and args.log_file:
        Logger.set_log_file(args.log_file)
    
    # Print enhanced banner
    print(f"\n{Color.BOLD}{Color.CYAN}{'=' * 60}{Color.RESET}")
    print(f"{Color.BOLD}Motion Engine Build System v{BuildConfig.VERSION} (Enhanced){Color.RESET}")
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
            manager = BuildManager(packages, parallel=args.parallel)
            
            # Parse profile
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
        
        elif args.command == "clean":
            manager = BuildManager(packages)
            manager.clean(args.pkg if hasattr(args, 'pkg') else None)
        
        elif args.command == "rebuild":
            manager = BuildManager(packages, parallel=args.parallel)
            
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
            generator = PresetGenerator(packages)
            generator.generate(Path("."))
        
        elif args.command == "list":
            print(f"\n{Color.BOLD}Available Packages:{Color.RESET}\n")
            for pkg in packages:
                status = f"{Color.GREEN}✓{Color.RESET}" if pkg.enabled else f"{Color.RED}✗{Color.RESET}"
                deps = f" (deps: {', '.join(pkg.dependencies)})" if pkg.dependencies else ""
                print(f"  {status} {pkg.name:20} {deps}")
            print()
        
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