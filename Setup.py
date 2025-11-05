#!/usr/bin/env python3

"""
Motion Engine Build System
Version: 2.0.0

# Generate presets
---------------------------------
python Setup.py presets
---------------------------------

# Build everything (fast! but not every recommended way)
----------------------------------------------------------------
python Setup.py build --config <build_config> --parallel
----------------------------------------------------------------

# Build single package
--------------------------------------------------------------------
python Setup.py build --config <build_config> --pkg <package_name>
--------------------------------------------------------------------

# Clean rebuild
-------------------------------------------------------------------------
python Setup.py rebuild --config <build_config>
-------------------------------------------------------------------------

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
from pathlib import Path
from dataclasses import dataclass, field, asdict
from typing import List, Dict, Optional, Set
from datetime import datetime
from enum import Enum
from abc import ABC, abstractmethod


# ===================== Configuration =====================
class BuildConfig:
    """Central configuration for the build system"""
    VERSION = "2.0.0"
    CACHE_FILE = "build/.build_cache.json"
    STATE_FILE = "build/.build_state.json"
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


# ===================== System Checks =====================
class SystemValidator:
    """Validates system requirements and tools"""
    
    @staticmethod
    def check_cmake() -> tuple[int, int, int]:
        """Verify CMake installation and return version"""
        try:
            result = subprocess.run(
                ["cmake", "--version"],
                capture_output=True,
                text=True,
                check=True
            )
            
            # Parse version from output
            version_line = result.stdout.split('\n')[0]
            version_str = version_line.split()[2]
            version_parts = list(map(int, version_str.split('.')[:3]))
            while len(version_parts) < 3:
                version_parts.append(0)
            version_tuple = (version_parts[0], version_parts[1], version_parts[2])

            if version_tuple < BuildConfig.MIN_CMAKE_VERSION:
                raise ConfigurationException(
                    f"CMake {'.'.join(map(str, BuildConfig.MIN_CMAKE_VERSION))} or higher required, "
                    f"found {version_str}"
                )
            
            Logger.success(f"CMake version: {version_str}")
            return version_tuple
            
        except FileNotFoundError:
            raise ConfigurationException(
                "CMake not found. Install it and ensure it's in your PATH."
            )
    
    @staticmethod
    def check_compiler() -> Dict[str, str]:
        """Detect available compilers"""
        compilers = {}
        
        # Check for common compilers
        for compiler in ["gcc", "g++", "clang", "clang++", "cl"]:
            if shutil.which(compiler):
                compilers[compiler] = compiler
        
        if not compilers:
            Logger.warn("No compilers detected in PATH")
        else:
            Logger.info(f"Available compilers: {', '.join(compilers.keys())}")
        
        return compilers
    
    @staticmethod
    def check_disk_space(required_gb: float = 5.0) -> bool:
        """Check available disk space"""
        try:
            stat = shutil.disk_usage(".")
            available_gb = stat.free / (1024**3)
            
            if available_gb < required_gb:
                Logger.warn(
                    f"Low disk space: {available_gb:.1f}GB available "
                    f"(recommended: {required_gb:.1f}GB)"
                )
                return False
            
            Logger.debug(f"Disk space: {available_gb:.1f}GB available")
            return True
            
        except Exception as e:
            Logger.debug(f"Could not check disk space: {e}")
            return True


# ===================== File System Utilities =====================
class FileSystem:
    """Utilities for file system operations"""
    
    @staticmethod
    def ensure_dir(path: Path):
        """Create directory if it doesn't exist"""
        path.mkdir(parents=True, exist_ok=True)
    
    @staticmethod
    def write_json(path: Path, data: dict):
        """Write JSON data to file"""
        FileSystem.ensure_dir(path.parent)
        with open(path, "w", encoding="utf-8") as f:
            json.dump(data, f, indent=2)
        Logger.debug(f"Wrote {path}")
    
    @staticmethod
    def read_json(path: Path) -> Optional[dict]:
        """Read JSON data from file"""
        if not path.exists():
            return None
        
        try:
            with open(path, "r", encoding="utf-8") as f:
                return json.load(f)
        except Exception as e:
            Logger.warn(f"Failed to read {path}: {e}")
            return None
    
    @staticmethod
    def calculate_file_hash(path: Path) -> str:
        """Calculate SHA256 hash of a file"""
        sha256 = hashlib.sha256()
        with open(path, "rb") as f:
            for chunk in iter(lambda: f.read(8192), b""):
                sha256.update(chunk)
        return sha256.hexdigest()
    
    @staticmethod
    def clean_directory(path: Path, pattern: str = "*"):
        """Clean a directory matching pattern"""
        if not path.exists():
            return
        
        for item in path.glob(pattern):
            try:
                if item.is_dir():
                    shutil.rmtree(item)
                else:
                    item.unlink()
                Logger.debug(f"Removed {item}")
            except Exception as e:
                Logger.warn(f"Failed to remove {item}: {e}")


# ===================== CMake Operations =====================
class CMakeCommand:
    """Abstraction for CMake operations"""
    
    def __init__(self, package: Package, config: str, prefix_path: str):
        self.package = package
        self.config = config
        self.prefix_path = prefix_path
    
    def _build_configure_args(self) -> List[str]:
        """Build CMake configuration arguments"""
        return [
            "cmake",
            "--fresh",
            f"-DCMAKE_BUILD_TYPE={self.config}",
            f"-DCMAKE_SYSTEM_NAME={platform.system()}",
            f"-DCMAKE_PREFIX_PATH={self.prefix_path}",
            f"-DCMAKE_INSTALL_PREFIX={self.prefix_path}",
            f"-DCMAKE_CXX_STANDARD={BuildConfig.CXX_STANDARD}",
            f"-DCMAKE_C_STANDARD={BuildConfig.C_STANDARD}",
            "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
            *self.package.options.split(),
            f"-S{self.package.source_directory}",
            f"-B{self.package.build_directory}"
        ]
    
    def configure(self) -> bool:
        """Configure package with CMake"""
        args = self._build_configure_args()
        Logger.command(" ".join(args))
        
        try:
            result = subprocess.run(
                args,
                capture_output=True,
                text=True,
                check=True
            )
            Logger.debug(result.stdout)
            return True
            
        except subprocess.CalledProcessError as e:
            Logger.error(f"Configuration failed: {e.stderr}")
            return False
    
    def build(self) -> bool:
        """Build package"""
        args = [
            "cmake",
            "--build", self.package.build_directory,
            "--config", self.config,
            "-j", str(os.cpu_count() or 1)
        ]
        Logger.command(" ".join(args))
        
        try:
            result = subprocess.run(
                args,
                capture_output=True,
                text=True,
                check=True
            )
            Logger.debug(result.stdout)
            return True
            
        except subprocess.CalledProcessError as e:
            Logger.error(f"Build failed: {e.stderr}")
            return False
    
    def install(self) -> bool:
        """Install package"""
        args = [
            "cmake",
            "--install", self.package.build_directory,
            "--config", self.config,
            "--prefix", self.package.prefix_directory
        ]
        Logger.command(" ".join(args))
        
        try:
            result = subprocess.run(
                args,
                capture_output=True,
                text=True,
                check=True
            )
            Logger.debug(result.stdout)
            return True
            
        except subprocess.CalledProcessError as e:
            Logger.error(f"Installation failed: {e.stderr}")
            return False


# ===================== Build Strategy =====================
class BuildStrategy(ABC):
    """Abstract base class for build strategies"""
    
    @abstractmethod
    def build(self, packages: List[Package], config: str, cache: BuildCache) -> List[BuildResult]:
        """Execute build strategy"""
        pass


class SequentialBuildStrategy(BuildStrategy):
    """Build packages sequentially"""
    
    def build(self, packages: List[Package], config: str, cache: BuildCache) -> List[BuildResult]:
        results = []
        prefix_path = self._get_prefix_path(packages)
        
        for package in packages:
            if not package.enabled:
                Logger.info(f"Skipping disabled package: {package.name}")
                continue
            
            result = self._build_package(package, config, prefix_path, cache)
            results.append(result)
            
            if not result.success:
                Logger.error(f"Failed to build {package.name}, stopping build")
                break
        
        return results
    
    def _build_package(self, package: Package, config: str, 
                      prefix_path: str, cache: BuildCache) -> BuildResult:
        """Build a single package"""
        start_time = datetime.now()
        
        # Check cache
        if cache.is_package_cached(package, config):
            Logger.info(f"⚡ {package.name} is cached, skipping")
            return BuildResult(
                package=package,
                success=True,
                duration=0.0
            )
        
        Logger.info(f"🔨 Building {Color.BOLD}{package.name}{Color.RESET}")
        
        cmake = CMakeCommand(package, config, prefix_path)
        
        # Configure
        if not cmake.configure():
            duration = (datetime.now() - start_time).total_seconds()
            return BuildResult(
                package=package,
                success=False,
                duration=duration,
                error_message="Configuration failed"
            )
        
        # Build
        if not cmake.build():
            duration = (datetime.now() - start_time).total_seconds()
            return BuildResult(
                package=package,
                success=False,
                duration=duration,
                error_message="Build failed"
            )
        
        # Install
        if not cmake.install():
            duration = (datetime.now() - start_time).total_seconds()
            return BuildResult(
                package=package,
                success=False,
                duration=duration,
                error_message="Installation failed"
            )
        
        # Update cache
        cache.update_package(package, config)
        
        duration = (datetime.now() - start_time).total_seconds()
        Logger.success(f"✓ {package.name} built in {duration:.1f}s")
        
        return BuildResult(
            package=package,
            success=True,
            duration=duration
        )
    
    @staticmethod
    def _get_prefix_path(packages: List[Package]) -> str:
        """Get aggregated prefix path"""
        sep = ";" if platform.system() == "Windows" else ":"
        return sep.join(os.path.abspath(pkg.prefix_directory) for pkg in packages)


class ParallelBuildStrategy(BuildStrategy):
    """Build packages in parallel where possible"""
    
    def __init__(self, max_workers: int = 0):
        self.max_workers = max_workers or min(os.cpu_count() or 1, 4)
    
    def build(self, packages: List[Package], config: str, cache: BuildCache) -> List[BuildResult]:
        # Resolve dependency order
        sorted_packages = self._resolve_dependencies(packages)
        
        # Build in batches based on dependencies
        results = []
        built_packages = set()
        
        while len(built_packages) < len(sorted_packages):
            # Find packages ready to build (dependencies satisfied)
            ready = [
                pkg for pkg in sorted_packages
                if pkg.name not in built_packages
                and all(dep in built_packages for dep in pkg.dependencies)
            ]
            
            if not ready:
                Logger.error("Circular dependency detected or no packages ready")
                break
            
            # Build ready packages in parallel
            batch_results = self._build_batch(ready, config, cache)
            results.extend(batch_results)
            
            # Update built set
            for result in batch_results:
                if result.success:
                    built_packages.add(result.package.name)
                else:
                    Logger.error(f"Build failed for {result.package.name}")
                    return results
        
        return results
    
    def _resolve_dependencies(self, packages: List[Package]) -> List[Package]:
        """Topologically sort packages by dependencies"""
        # Simple dependency resolution (can be enhanced with proper graph algorithms)
        sorted_pkgs = []
        remaining = packages.copy()
        
        while remaining:
            # Find packages with no unresolved dependencies
            ready = [
                pkg for pkg in remaining
                if all(
                    dep in [p.name for p in sorted_pkgs]
                    for dep in pkg.dependencies
                )
            ]
            
            if not ready:
                raise DependencyException(
                    f"Circular dependency or missing package: "
                    f"{[pkg.name for pkg in remaining]}"
                )
            
            sorted_pkgs.extend(ready)
            for pkg in ready:
                remaining.remove(pkg)
        
        return sorted_pkgs
    
    def _build_batch(self, packages: List[Package], config: str, 
                    cache: BuildCache) -> List[BuildResult]:
        """Build a batch of packages in parallel"""
        results = []
        threads = []
        result_queue = queue.Queue()
        prefix_path = SequentialBuildStrategy._get_prefix_path(packages)
        
        def worker(pkg: Package):
            builder = SequentialBuildStrategy()
            result = builder._build_package(pkg, config, prefix_path, cache)
            result_queue.put(result)
        
        # Start threads
        for package in packages[:self.max_workers]:
            if not package.enabled:
                continue
            
            thread = threading.Thread(target=worker, args=(package,))
            thread.start()
            threads.append(thread)
        
        # Wait for completion
        for thread in threads:
            thread.join()
        
        # Collect results
        while not result_queue.empty():
            results.append(result_queue.get())
        
        return results


# ===================== Preset Generator =====================
class PresetGenerator:
    """Generates CMakePresets.json"""
    
    def __init__(self, packages: List[Package]):
        self.packages = packages
    
    def generate(self, output_dir: Path):
        """Generate CMakePresets.json"""
        preset_data = self._build_preset_data()
        output_file = output_dir / "CMakePresets.json"
        
        # Check if update needed
        if output_file.exists():
            existing = FileSystem.read_json(output_file)
            if existing and self._hash_dict(existing) == self._hash_dict(preset_data):
                Logger.info("CMakePresets.json is up-to-date")
                return
        
        FileSystem.write_json(output_file, preset_data)
        Logger.success("Generated CMakePresets.json")
    
    def _build_preset_data(self) -> dict:
        """Build preset data structure"""
        sep = ";" if platform.system() == "Windows" else ":"
        prefix_path = sep.join(
            os.path.abspath(pkg.prefix_directory) for pkg in self.packages
        )
        
        base_vars = {
            "CMAKE_CXX_STANDARD": BuildConfig.CXX_STANDARD,
            "CMAKE_CXX_STANDARD_REQUIRED": "ON",
            "CMAKE_CXX_EXTENSIONS": "OFF",
            "CMAKE_C_STANDARD": BuildConfig.C_STANDARD,
            "CMAKE_C_STANDARD_REQUIRED": "ON",
            "CMAKE_C_EXTENSIONS": "OFF",
            "CMAKE_PREFIX_PATH": prefix_path,
            "CMAKE_INSTALL_PREFIX": prefix_path,
            "CMAKE_EXPORT_COMPILE_COMMANDS": "ON"
        }
        
        configure_presets = [
            {
                "name": "base",
                "hidden": True,
                "binaryDir": "${sourceDir}/build",
                "cacheVariables": base_vars
            }
        ]
        
        for config in BuildConfig.CONFIGS:
            configure_presets.append({
                "name": f"{config}-x64",
                "inherits": "base",
                "displayName": f"{config} x64",
                "description": f"{config} configuration for x64",
                "architecture": {"value": "x86_64", "strategy": "external"},
                "binaryDir": f"${{sourceDir}}/build/{config}-x64"
            })
        
        build_presets = [
            {
                "name": "base",
                "hidden": True,
                "configurePreset": "base",
                "jobs": os.cpu_count() or 1,
                "cleanFirst": True
            }
        ]
        
        for config in BuildConfig.CONFIGS:
            build_presets.append({
                "name": f"{config}-x64",
                "inherits": "base",
                "configurePreset": f"{config}-x64"
            })
        
        test_presets = [
            {
                "name": "base",
                "hidden": True,
                "configurePreset": "base",
                "execution": {"noTestsAction": "error", "stopOnFailure": False},
                "output": {"outputOnFailure": True}
            }
        ]
        
        for config in BuildConfig.CONFIGS:
            test_presets.append({
                "name": f"{config}-x64",
                "inherits": "base",
                "configurePreset": f"{config}-x64"
            })
        
        return {
            "version": 3,
            "cmakeMinimumRequired": {
                "major": BuildConfig.MIN_CMAKE_VERSION[0],
                "minor": BuildConfig.MIN_CMAKE_VERSION[1],
                "patch": BuildConfig.MIN_CMAKE_VERSION[2]
            },
            "configurePresets": configure_presets,
            "buildPresets": build_presets,
            "testPresets": test_presets
        }
    
    @staticmethod
    def _hash_dict(data: dict) -> str:
        """Generate hash of dictionary"""
        json_str = json.dumps(data, sort_keys=True)
        return hashlib.md5(json_str.encode()).hexdigest()


# ===================== Build Manager =====================
class BuildManager:
    """Main build orchestrator"""
    
    def __init__(self, packages: List[Package], parallel: bool = False):
        self.packages = packages
        self.cache = self._load_cache()
        self.strategy: BuildStrategy = (
            ParallelBuildStrategy() if parallel 
            else SequentialBuildStrategy()
        )
    
    def build(self, config: str, package_filter: Optional[str] = None) -> bool:
        """Execute build process"""
        # Filter packages
        packages_to_build = self._filter_packages(package_filter)
        
        if not packages_to_build:
            Logger.error(f"No packages match filter: {package_filter}")
            return False
        
        Logger.info(f"Building {len(packages_to_build)} package(s)")
        
        # Execute build
        results = self.strategy.build(packages_to_build, config, self.cache)
        
        # Save cache
        self._save_cache()
        
        # Generate report
        self._print_report(results)
        
        # Return success status
        return all(r.success for r in results)
    
    def clean(self, package_filter: Optional[str] = None):
        """Clean build artifacts"""
        packages = self._filter_packages(package_filter)
        
        for package in packages:
            Logger.info(f"Cleaning {package.name}")
            FileSystem.clean_directory(Path(package.build_directory))
            FileSystem.clean_directory(Path(package.prefix_directory))
        
        Logger.success("Clean complete")
    
    def rebuild(self, config: str, package_filter: Optional[str] = None):
        """Clean and rebuild packages"""
        self.clean(package_filter)
        self.cache = BuildCache()  # Reset cache
        return self.build(config, package_filter)
    
    def _filter_packages(self, package_filter: Optional[str]) -> List[Package]:
        """Filter packages by name"""
        if not package_filter:
            return self.packages
        
        filtered = [
            pkg for pkg in self.packages
            if pkg.name.lower() == package_filter.lower()
        ]
        
        return filtered
    
    def _load_cache(self) -> BuildCache:
        """Load build cache from disk"""
        cache_path = Path(BuildConfig.CACHE_FILE)
        data = FileSystem.read_json(cache_path)
        
        if data:
            return BuildCache(**data)
        
        return BuildCache()
    
    def _save_cache(self):
        """Save build cache to disk"""
        cache_path = Path(BuildConfig.CACHE_FILE)
        FileSystem.write_json(cache_path, asdict(self.cache))
    
    def _print_report(self, results: List[BuildResult]):
        """Print build report"""
        print("\n" + "=" * 60)
        print(f"{Color.BOLD}Build Report{Color.RESET}")
        print("=" * 60)
        
        total_time = sum(r.duration for r in results)
        success_count = sum(1 for r in results if r.success)
        
        for result in results:
            status = f"{Color.GREEN}✓ PASS{Color.RESET}" if result.success else f"{Color.RED}✗ FAIL{Color.RESET}"
            print(f"{status} {result.package.name:20s} ({result.duration:.1f}s)")
            if result.error_message:
                print(f"      {Color.RED}Error: {result.error_message}{Color.RESET}")
        
        print("=" * 60)
        print(f"Total: {success_count}/{len(results)} packages")
        print(f"Time: {total_time:.1f}s")
        print("=" * 60 + "\n")


# ===================== Package Registry =====================
def get_package_registry() -> List[Package]:
    """Define all packages for the Motion engine"""
    return [
        Package(
            name="glfw",
            source_directory="libs/glfw",
            build_directory="build/vendors/glfw",
            prefix_directory="build/packages/glfw",
            options="-DGLFW_BUILD_EXAMPLES=OFF -DGLFW_BUILD_TESTS=OFF -DGLFW_BUILD_DOCS=OFF"
        ),
        Package(
            name="spdlog",
            source_directory="libs/spdlog",
            build_directory="build/vendors/spdlog",
            prefix_directory="build/packages/spdlog",
            options="-DSPDLOG_BUILD_EXAMPLE=OFF"
        ),
        Package(
            name="glad",
            source_directory="libs/glad",
            build_directory="build/vendors/glad",
            prefix_directory="build/packages/glad"
        ),
        Package(
            name="glm",
            source_directory="libs/glm",
            build_directory="build/vendors/glm",
            prefix_directory="build/packages/glm",
            options="-DGLM_BUILD_TESTS=OFF"
        ),
        Package(
            name="imgui",
            source_directory="libs/imgui_docking",
            build_directory="build/vendors/imgui",
            prefix_directory="build/packages/imgui",
            dependencies=["glfw", "glad"]
        ),
        Package(
            name="entt",
            source_directory="libs/entt",
            build_directory="build/vendors/entt",
            prefix_directory="build/packages/entt",
            options="-DENTT_INCLUDE_HEADERS=ON -DENTT_INCLUDE_NATVIS=ON -DENTT_INSTALL=ON"
        ),
        Package(
            name="assimp",
            source_directory="libs/assimp",
            build_directory="build/vendors/assimp",
            prefix_directory="build/packages/assimp",
            options="-DASSIMP_BUILD_TESTS=OFF"
        ),
        Package(
            name="stb_image",
            source_directory="libs/stb_image",
            build_directory="build/vendors/stb_image",
            prefix_directory="build/packages/stb_image"
        ),
        Package(
            name="yaml-cpp",
            source_directory="libs/yaml-cpp",
            build_directory="build/vendors/yaml-cpp",
            prefix_directory="build/packages/yaml-cpp",
            options="-DYAML_BUILD_SHARED_LIBS=OFF"
        ),
        Package(
            name="MikkTSpace",
            source_directory="libs/MikkTSpace",
            build_directory="build/vendors/MikkTSpace",
            prefix_directory="build/packages/MikkTSpace"
        ),
        Package(
            name="imguizmo",
            source_directory="libs/imguizmo",
            build_directory="build/vendors/imguizmo",
            prefix_directory="build/packages/imguizmo",
            dependencies=["imgui"]
        ),
        Package(
            name="ReactPhysics3D",
            source_directory="libs/reactphysics3d",
            build_directory="build/vendors/reactphysics3d",
            prefix_directory="build/packages/ReactPhysics3D",
            options=(
                "-DRP3D_COMPILE_LIBRARY=ON "
                "-DRP3D_COMPILE_TESTBED=OFF "
                "-DRP3D_COMPILE_TESTS=OFF "
                "-DRP3D_PROFILING_ENABLED=OFF "
                "-DRP3D_GENERATE_DOCUMENTATION=OFF "
                "-DRP3D_CODE_COVERAGE_ENABLED=OFF "
                "-DRP3D_DOUBLE_PRECISION_ENABLED=OFF"
            )
        ),
    ]


# ===================== CLI =====================
def create_argument_parser() -> argparse.ArgumentParser:
    """Create command-line argument parser"""
    parser = argparse.ArgumentParser(
        description="Motion Engine Build System",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s build --config Release                    Build all packages
  %(prog)s build --config Debug --pkg glfw           Build specific package
  %(prog)s build --config Release --parallel         Build in parallel
  %(prog)s clean --pkg imgui                         Clean specific package
  %(prog)s rebuild --config Debug                    Clean and rebuild all
  %(prog)s presets                                   Generate CMake presets
        """
    )
    
    parser.add_argument(
        "--version",
        action="version",
        version=f"Motion Build System v{BuildConfig.VERSION}"
    )
    
    subparsers = parser.add_subparsers(dest="command", help="Command to execute")
    
    # Build command
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
    
    # Presets command
    subparsers.add_parser("presets", help="Generate CMakePresets.json")
    
    # List command
    subparsers.add_parser("list", help="List all packages")
    
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
    if args.verbose:
        Logger.set_level(LogLevel.DEBUG)
    if hasattr(args, 'log_file') and args.log_file:
        Logger.set_log_file(args.log_file)
    
    # Print banner
    print(f"\n{Color.BOLD}{Color.CYAN}{'=' * 60}{Color.RESET}")
    print(f"{Color.BOLD}Motion Engine Build System v{BuildConfig.VERSION}{Color.RESET}")
    print(f"{Color.BOLD}{Color.CYAN}{'=' * 60}{Color.RESET}\n")
    
    try:
        # System validation
        SystemValidator.check_cmake()
        SystemValidator.check_compiler()
        SystemValidator.check_disk_space()
        
        # Get package registry
        packages = get_package_registry()
        
        # Execute command
        if args.command == "build":
            manager = BuildManager(packages, parallel=args.parallel)
            success = manager.build(args.config, args.pkg)
            sys.exit(0 if success else 1)
        
        elif args.command == "clean":
            manager = BuildManager(packages)
            manager.clean(args.pkg)
        
        elif args.command == "rebuild":
            manager = BuildManager(packages, parallel=args.parallel)
            success = manager.rebuild(args.config, args.pkg)
            sys.exit(0 if success else 1)
        
        elif args.command == "presets":
            generator = PresetGenerator(packages)
            generator.generate(Path("."))
        
        elif args.command == "list":
            print(f"\n{Color.BOLD}Available Packages:{Color.RESET}\n")
            for pkg in packages:
                status = f"{Color.GREEN}✓{Color.RESET}" if pkg.enabled else f"{Color.RED}✗{Color.RESET}"
                deps = f" (deps: {', '.join(pkg.dependencies)})" if pkg.dependencies else ""
                print(f"  {status} {pkg.name}{deps}")
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