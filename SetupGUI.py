#!/usr/bin/env python3

"""
Motion Engine Build System - GUI Mode
Version: 2.3.0 - GUI Edition
"""

import tkinter as tk
from tkinter import ttk, scrolledtext, messagebox, filedialog
import threading
import queue
import sys
import os
from pathlib import Path
from datetime import datetime
import json

# Import the original Setup module
import Setup
from Setup import (
    BuildConfig, Logger, LogLevel, BuildManager, BuildProfile,
    get_package_registry, SystemValidator, PresetGenerator,
    DependencyGraph, Color, BuildException
)


class BuildGUI:
    """Modern GUI for Motion Engine Build System"""
    
    def __init__(self, root):
        self.root = root
        self.root.title(f"Motion Engine Build System v{BuildConfig.VERSION}")
        self.root.geometry("1200x800")
        self.root.minsize(900, 600)
        
        # Build state
        self.packages = []
        self.build_thread = None
        self.log_queue = queue.Queue()
        self.is_building = False
        
        # Configure style
        self.setup_styles()
        
        # Create UI
        self.create_ui()
        
        # Start log processor
        self.process_logs()
        
        # Load packages
        self.load_packages()
        
        # Redirect logger to GUI
        self.setup_logging_redirect()
        
    def setup_styles(self):
        """Configure ttk styles for modern look"""
        style = ttk.Style()
        
        # Try to use a modern theme
        available_themes = style.theme_names()
        if 'clam' in available_themes:
            style.theme_use('clam')
        elif 'alt' in available_themes:
            style.theme_use('alt')
            
        # Customize colors
        style.configure('Header.TLabel', font=('Segoe UI', 12, 'bold'))
        style.configure('Title.TLabel', font=('Segoe UI', 16, 'bold'))
        style.configure('Build.TButton', font=('Segoe UI', 10, 'bold'))
        style.configure('Success.TLabel', foreground='#2ecc71')
        style.configure('Error.TLabel', foreground='#e74c3c')
        style.configure('Warning.TLabel', foreground='#f39c12')
        
    def create_ui(self):
        """Create the main UI layout"""
        # Main container
        main_container = ttk.Frame(self.root, padding="10")
        main_container.grid(row=0, column=0, sticky="nsew")
        self.root.grid_rowconfigure(0, weight=1)
        self.root.grid_columnconfigure(0, weight=1)
        
        # Title
        title_frame = ttk.Frame(main_container)
        title_frame.grid(row=0, column=0, columnspan=2, sticky="ew", pady=(0, 10))
        
        ttk.Label(
            title_frame, 
            text="🏗️ Motion Engine Build System",
            style='Title.TLabel'
        ).pack(side=tk.LEFT)
        
        self.status_label = ttk.Label(title_frame, text="Ready", style='Header.TLabel')
        self.status_label.pack(side=tk.RIGHT)
        
        # Left panel - Configuration
        left_panel = ttk.LabelFrame(main_container, text="Build Configuration", padding="10")
        left_panel.grid(row=1, column=0, sticky="nsew", padx=(0, 5))
        main_container.grid_rowconfigure(1, weight=1)
        main_container.grid_columnconfigure(0, weight=1)
        
        self.create_config_panel(left_panel)
        
        # Right panel - Package selection
        right_panel = ttk.LabelFrame(main_container, text="Packages", padding="10")
        right_panel.grid(row=1, column=1, sticky="nsew", padx=(5, 0))
        main_container.grid_columnconfigure(1, weight=1)
        
        self.create_package_panel(right_panel)
        
        # Bottom panel - Log output
        bottom_panel = ttk.LabelFrame(main_container, text="Build Output", padding="10")
        bottom_panel.grid(row=2, column=0, columnspan=2, sticky="nsew", pady=(10, 0))
        main_container.grid_rowconfigure(2, weight=2)
        
        self.create_log_panel(bottom_panel)
        
        # Control buttons
        control_frame = ttk.Frame(main_container)
        control_frame.grid(row=3, column=0, columnspan=2, sticky="ew", pady=(10, 0))
        
        self.create_control_buttons(control_frame)
        
    def create_config_panel(self, parent):
        """Create configuration options panel"""
        row = 0
        
        # Build configuration
        ttk.Label(parent, text="Configuration:", style='Header.TLabel').grid(
            row=row, column=0, sticky="w", pady=(0, 5)
        )
        row += 1
        
        self.config_var = tk.StringVar(value="Release")
        config_frame = ttk.Frame(parent)
        config_frame.grid(row=row, column=0, sticky="ew", pady=(0, 10))
        
        for config in BuildConfig.CONFIGS:
            ttk.Radiobutton(
                config_frame, 
                text=config, 
                variable=self.config_var, 
                value=config
            ).pack(side=tk.LEFT, padx=5)
        row += 1
        
        # Build profile
        ttk.Label(parent, text="Build Profile:", style='Header.TLabel').grid(
            row=row, column=0, sticky="w", pady=(10, 5)
        )
        row += 1
        
        self.profile_var = tk.StringVar(value="full")
        profile_combo = ttk.Combobox(
            parent, 
            textvariable=self.profile_var,
            values=["full", "quick", "minimal", "dev", "release"],
            state="readonly",
            width=30
        )
        profile_combo.grid(row=row, column=0, sticky="ew", pady=(0, 5))
        row += 1
        
        # Profile descriptions
        profile_desc = {
            "full": "All packages (complete build)",
            "quick": "Essential packages only",
            "minimal": "Minimum required packages",
            "dev": "Development with debug tools",
            "release": "Optimized release build"
        }
        
        self.profile_desc_label = ttk.Label(
            parent, 
            text=profile_desc["full"],
            foreground="gray",
            wraplength=280
        )
        self.profile_desc_label.grid(row=row, column=0, sticky="w", pady=(0, 10))
        row += 1
        
        def update_profile_desc(*args):
            self.profile_desc_label.config(text=profile_desc.get(self.profile_var.get(), ""))
        
        self.profile_var.trace_add("write", update_profile_desc)
        
        # Compiler options
        ttk.Label(parent, text="Compiler:", style='Header.TLabel').grid(
            row=row, column=0, sticky="w", pady=(10, 5)
        )
        row += 1
        
        compiler_frame = ttk.Frame(parent)
        compiler_frame.grid(row=row, column=0, sticky="ew", pady=(0, 10))
        
        ttk.Label(compiler_frame, text="C++:").grid(row=0, column=0, sticky="w")
        self.cxx_compiler_var = tk.StringVar(value="default")
        cxx_entry = ttk.Entry(compiler_frame, textvariable=self.cxx_compiler_var, width=20)
        cxx_entry.grid(row=0, column=1, sticky="ew", padx=5)
        
        ttk.Label(compiler_frame, text="C:").grid(row=1, column=0, sticky="w", pady=(5, 0))
        self.c_compiler_var = tk.StringVar(value="default")
        c_entry = ttk.Entry(compiler_frame, textvariable=self.c_compiler_var, width=20)
        c_entry.grid(row=1, column=1, sticky="ew", padx=5, pady=(5, 0))
        
        compiler_frame.grid_columnconfigure(1, weight=1)
        row += 1
        
        # Build options
        ttk.Separator(parent, orient='horizontal').grid(
            row=row, column=0, sticky="ew", pady=10
        )
        row += 1
        
        ttk.Label(parent, text="Options:", style='Header.TLabel').grid(
            row=row, column=0, sticky="w", pady=(0, 5)
        )
        row += 1
        
        self.parallel_var = tk.BooleanVar(value=True)
        ttk.Checkbutton(
            parent, 
            text="Parallel build (faster)", 
            variable=self.parallel_var
        ).grid(row=row, column=0, sticky="w", pady=2)
        row += 1
        
        self.incremental_var = tk.BooleanVar(value=True)
        ttk.Checkbutton(
            parent, 
            text="Incremental build (skip cached)", 
            variable=self.incremental_var
        ).grid(row=row, column=0, sticky="w", pady=2)
        row += 1
        
        self.verbose_var = tk.BooleanVar(value=False)
        ttk.Checkbutton(
            parent, 
            text="Verbose output", 
            variable=self.verbose_var
        ).grid(row=row, column=0, sticky="w", pady=2)
        row += 1
        
        # System info
        ttk.Separator(parent, orient='horizontal').grid(
            row=row, column=0, sticky="ew", pady=10
        )
        row += 1
        
        info_frame = ttk.Frame(parent)
        info_frame.grid(row=row, column=0, sticky="ew")
        
        ttk.Label(info_frame, text="System Info", style='Header.TLabel').pack(anchor="w")
        
        self.system_info_text = tk.Text(
            info_frame, 
            height=8, 
            width=30,
            state='disabled',
            background='#f5f5f5',
            font=('Courier', 9)
        )
        self.system_info_text.pack(fill=tk.BOTH, expand=True, pady=5)
        
        self.update_system_info()
        
    def create_package_panel(self, parent):
        """Create package selection panel"""
        # Search bar
        search_frame = ttk.Frame(parent)
        search_frame.pack(fill=tk.X, pady=(0, 5))
        
        ttk.Label(search_frame, text="🔍").pack(side=tk.LEFT)
        self.search_var = tk.StringVar()
        self.search_var.trace_add("write", self.filter_packages)
        
        search_entry = ttk.Entry(search_frame, textvariable=self.search_var)
        search_entry.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=5)
        
        # Select all/none buttons
        btn_frame = ttk.Frame(parent)
        btn_frame.pack(fill=tk.X, pady=(0, 5))
        
        ttk.Button(btn_frame, text="Select All", command=self.select_all_packages).pack(
            side=tk.LEFT, padx=2
        )
        ttk.Button(btn_frame, text="Select None", command=self.select_no_packages).pack(
            side=tk.LEFT, padx=2
        )
        ttk.Button(btn_frame, text="Reset", command=self.reset_package_selection).pack(
            side=tk.LEFT, padx=2
        )
        
        # Package list with scrollbar
        list_frame = ttk.Frame(parent)
        list_frame.pack(fill=tk.BOTH, expand=True)
        
        scrollbar = ttk.Scrollbar(list_frame)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        
        # Create Treeview for packages
        self.package_tree = ttk.Treeview(
            list_frame,
            columns=("status", "name", "deps"),
            show="tree headings",
            yscrollcommand=scrollbar.set,
            selectmode="extended"
        )
        
        self.package_tree.heading("status", text="✓")
        self.package_tree.heading("name", text="Package")
        self.package_tree.heading("deps", text="Dependencies")
        
        self.package_tree.column("#0", width=0, stretch=False)
        self.package_tree.column("status", width=30, anchor="center")
        self.package_tree.column("name", width=150)
        self.package_tree.column("deps", width=200)
        
        self.package_tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.config(command=self.package_tree.yview)
        
        # Bind double-click to toggle
        self.package_tree.bind("<Double-1>", self.toggle_package)
        
        # Package info
        info_frame = ttk.LabelFrame(parent, text="Package Info", padding="5")
        info_frame.pack(fill=tk.X, pady=(10, 0))
        
        self.package_info_label = ttk.Label(
            info_frame,
            text="Select a package to see details",
            wraplength=350,
            justify=tk.LEFT
        )
        self.package_info_label.pack()
        
        self.package_tree.bind("<<TreeviewSelect>>", self.show_package_info)
        
    def create_log_panel(self, parent):
        """Create log output panel"""
        # Toolbar
        toolbar = ttk.Frame(parent)
        toolbar.pack(fill=tk.X, pady=(0, 5))
        
        ttk.Button(toolbar, text="Clear", command=self.clear_log).pack(side=tk.LEFT, padx=2)
        ttk.Button(toolbar, text="Save Log", command=self.save_log).pack(side=tk.LEFT, padx=2)
        
        self.auto_scroll_var = tk.BooleanVar(value=True)
        ttk.Checkbutton(
            toolbar, 
            text="Auto-scroll", 
            variable=self.auto_scroll_var
        ).pack(side=tk.LEFT, padx=10)
        
        # Log text widget
        self.log_text = scrolledtext.ScrolledText(
            parent,
            wrap=tk.WORD,
            height=15,
            font=('Consolas', 9),
            background='#1e1e1e',
            foreground='#d4d4d4',
            insertbackground='white'
        )
        self.log_text.pack(fill=tk.BOTH, expand=True)
        
        # Configure tags for colored output
        self.log_text.tag_config("ERROR", foreground="#f48771")
        self.log_text.tag_config("SUCCESS", foreground="#a8cc8c")
        self.log_text.tag_config("WARN", foreground="#e5c07b")
        self.log_text.tag_config("INFO", foreground="#61afef")
        self.log_text.tag_config("DEBUG", foreground="#c678dd")
        self.log_text.tag_config("TIMESTAMP", foreground="#5c6370")
        
    def create_control_buttons(self, parent):
        """Create main control buttons"""
        # Progress bar
        self.progress_var = tk.DoubleVar()
        self.progress_bar = ttk.Progressbar(
            parent,
            mode='indeterminate',
            variable=self.progress_var
        )
        self.progress_bar.pack(fill=tk.X, pady=(0, 10))
        
        # Buttons
        btn_frame = ttk.Frame(parent)
        btn_frame.pack(fill=tk.X)
        
        self.build_btn = ttk.Button(
            btn_frame,
            text="🔨 Build",
            style='Build.TButton',
            command=self.start_build
        )
        self.build_btn.pack(side=tk.LEFT, padx=5, ipadx=20, ipady=5)
        
        self.rebuild_btn = ttk.Button(
            btn_frame,
            text="🔄 Rebuild All",
            command=self.start_rebuild
        )
        self.rebuild_btn.pack(side=tk.LEFT, padx=5, ipadx=20, ipady=5)
        
        self.stop_btn = ttk.Button(
            btn_frame,
            text="⛔ Stop",
            command=self.stop_build,
            state='disabled'
        )
        self.stop_btn.pack(side=tk.LEFT, padx=5, ipadx=20, ipady=5)
        
        ttk.Separator(btn_frame, orient='vertical').pack(side=tk.LEFT, fill=tk.Y, padx=10)
        
        ttk.Button(
            btn_frame,
            text="📊 Statistics",
            command=self.show_statistics
        ).pack(side=tk.LEFT, padx=5)
        
        ttk.Button(
            btn_frame,
            text="🌐 Dependency Graph",
            command=self.show_dependency_graph
        ).pack(side=tk.LEFT, padx=5)
        
        ttk.Button(
            btn_frame,
            text="⚙️ Generate Presets",
            command=self.generate_presets
        ).pack(side=tk.LEFT, padx=5)
        
    def load_packages(self):
        """Load package registry"""
        try:
            self.packages = get_package_registry()
            self.update_package_tree()
            self.log_message("INFO", f"Loaded {len(self.packages)} packages")
        except Exception as e:
            messagebox.showerror("Error", f"Failed to load packages: {e}")
            
    def update_package_tree(self):
        """Update package tree view"""
        self.package_tree.delete(*self.package_tree.get_children())
        
        search_term = self.search_var.get().lower()
        
        for pkg in self.packages:
            if search_term and search_term not in pkg.name.lower():
                continue
                
            status = "✓" if pkg.enabled else "✗"
            deps = ", ".join(pkg.dependencies) if pkg.dependencies else "None"
            
            item_id = self.package_tree.insert(
                "",
                tk.END,
                values=(status, pkg.name, deps),
                tags=("enabled" if pkg.enabled else "disabled",)
            )
            
        # Configure row colors
        self.package_tree.tag_configure("enabled", foreground="black")
        self.package_tree.tag_configure("disabled", foreground="gray")
        
    def filter_packages(self, *args):
        """Filter packages based on search term"""
        self.update_package_tree()
        
    def toggle_package(self, event):
        """Toggle package enabled state"""
        selection = self.package_tree.selection()
        if not selection:
            return
            
        for item_id in selection:
            values = self.package_tree.item(item_id)["values"]
            pkg_name = values[1]
            
            # Find package and toggle
            for pkg in self.packages:
                if pkg.name == pkg_name:
                    pkg.enabled = not pkg.enabled
                    break
                    
        self.update_package_tree()
        
    def select_all_packages(self):
        """Enable all packages"""
        for pkg in self.packages:
            pkg.enabled = True
        self.update_package_tree()
        
    def select_no_packages(self):
        """Disable all packages"""
        for pkg in self.packages:
            pkg.enabled = False
        self.update_package_tree()
        
    def reset_package_selection(self):
        """Reset package selection to defaults"""
        self.load_packages()
        
    def show_package_info(self, event):
        """Show selected package information"""
        selection = self.package_tree.selection()
        if not selection:
            self.package_info_label.config(text="Select a package to see details")
            return
            
        item_id = selection[0]
        values = self.package_tree.item(item_id)["values"]
        pkg_name = values[1]
        
        # Find package details
        for pkg in self.packages:
            if pkg.name == pkg_name:
                info = f"📦 {pkg.name}\n\n"
                info += f"Source: {pkg.source_directory}\n"
                info += f"Build Dir: {pkg.build_directory}\n"
                info += f"Dependencies: {', '.join(pkg.dependencies) if pkg.dependencies else 'None'}\n"
                if pkg.options:
                    info += f"Options: {pkg.options}\n"
                    
                self.package_info_label.config(text=info)
                break
                
    def update_system_info(self):
        """Update system information display"""
        self.system_info_text.config(state='normal')
        self.system_info_text.delete(1.0, tk.END)
        
        info = []
        info.append(f"Platform: {sys.platform}")
        info.append(f"Python: {sys.version.split()[0]}")
        
        try:
            import subprocess
            cmake_ver = subprocess.check_output(
                ["cmake", "--version"], 
                stderr=subprocess.DEVNULL
            ).decode().split('\n')[0]
            info.append(f"CMake: {cmake_ver.split()[-1]}")
        except:
            info.append("CMake: Not found")
            
        try:
            import shutil
            compiler = shutil.which("g++") or shutil.which("clang++") or shutil.which("cl")
            if compiler:
                info.append(f"Compiler: {Path(compiler).name}")
            else:
                info.append("Compiler: Not detected")
        except:
            info.append("Compiler: Unknown")
            
        # Disk space
        try:
            import shutil
            total, used, free = shutil.disk_usage(".")
            free_gb = free // (2**30)
            info.append(f"Free space: {free_gb} GB")
        except:
            pass
            
        self.system_info_text.insert(tk.END, "\n".join(info))
        self.system_info_text.config(state='disabled')
        
    def setup_logging_redirect(self):
        """Redirect logger output to GUI"""
        class GUILogHandler:
            def __init__(self, queue):
                self.queue = queue
                
            def log(self, level, msg):
                self.queue.put((level, msg))
                
        self.log_handler = GUILogHandler(self.log_queue)
        
    def log_message(self, level, msg):
        """Add message to log queue"""
        self.log_queue.put((level, msg))
        
    def process_logs(self):
        """Process log messages from queue"""
        try:
            while True:
                level, msg = self.log_queue.get_nowait()
                
                timestamp = datetime.now().strftime("%H:%M:%S")
                
                self.log_text.insert(tk.END, f"[{timestamp}] ", "TIMESTAMP")
                self.log_text.insert(tk.END, f"[{level}] ", level)
                self.log_text.insert(tk.END, f"{msg}\n")
                
                if self.auto_scroll_var.get():
                    self.log_text.see(tk.END)
                    
        except queue.Empty:
            pass
        finally:
            self.root.after(100, self.process_logs)
            
    def clear_log(self):
        """Clear log output"""
        self.log_text.delete(1.0, tk.END)
        
    def save_log(self):
        """Save log to file"""
        filename = filedialog.asksaveasfilename(
            defaultextension=".log",
            filetypes=[("Log files", "*.log"), ("Text files", "*.txt"), ("All files", "*.*")]
        )
        
        if filename:
            try:
                with open(filename, 'w') as f:
                    f.write(self.log_text.get(1.0, tk.END))
                messagebox.showinfo("Success", f"Log saved to {filename}")
            except Exception as e:
                messagebox.showerror("Error", f"Failed to save log: {e}")
                
    def start_build(self):
        """Start build process"""
        if self.is_building:
            messagebox.showwarning("Warning", "Build already in progress")
            return
            
        # Get selected packages
        enabled_packages = [pkg for pkg in self.packages if pkg.enabled]
        if not enabled_packages:
            messagebox.showwarning("Warning", "No packages selected")
            return
            
        self.is_building = True
        self.update_ui_state(building=True)
        
        # Start build in separate thread
        self.build_thread = threading.Thread(target=self.run_build, daemon=True)
        self.build_thread.start()
        
        self.progress_bar.start(10)
        
    def run_build(self):
        """Run build in background thread"""
        try:
            config = self.config_var.get()
            profile = BuildProfile(self.profile_var.get()) if self.profile_var.get() != "full" else None
            
            # Get compiler settings
            cxx_compiler = None if self.cxx_compiler_var.get() == "default" else self.cxx_compiler_var.get()
            c_compiler = None if self.c_compiler_var.get() == "default" else self.c_compiler_var.get()
            
            # Set log level
            if self.verbose_var.get():
                Logger.set_level(LogLevel.DEBUG)
            else:
                Logger.set_level(LogLevel.INFO)
                
            self.log_message("INFO", f"Starting {config} build...")
            self.log_message("INFO", f"Profile: {self.profile_var.get()}")
            
            # Create custom logger that redirects to GUI
            original_log = Logger._log
            
            def gui_log(level, prefix, color, msg):
                self.log_message(level.name, msg)
                
            Logger._log = gui_log
            
            # Create build manager
            manager = BuildManager(
                self.packages,
                parallel=self.parallel_var.get(),
                compiler_c=c_compiler,
                compiler_cxx=cxx_compiler
            )
            
            # Run build
            success = manager.build(
                config,
                profile=profile,
                incremental=self.incremental_var.get()
            )
            
            # Restore original logger
            Logger._log = original_log
            
            if success:
                self.log_message("SUCCESS", "Build completed successfully! ✓")
                self.root.after(0, lambda: self.status_label.config(
                    text="Build Complete ✓", 
                    style='Success.TLabel'
                ))
            else:
                self.log_message("ERROR", "Build failed! ✗")
                self.root.after(0, lambda: self.status_label.config(
                    text="Build Failed ✗", 
                    style='Error.TLabel'
                ))
                
        except Exception as e:
            self.log_message("ERROR", f"Build error: {e}")
            self.root.after(0, lambda: self.status_label.config(
                text="Build Error ✗", 
                style='Error.TLabel'
            ))
        finally:
            self.is_building = False
            self.root.after(0, lambda: self.update_ui_state(building=False))
            self.root.after(0, self.progress_bar.stop)
            
    def start_rebuild(self):
        """Start rebuild all"""
        result = messagebox.askyesno(
            "Confirm Rebuild",
            "This will clean and rebuild all packages.\nAre you sure?"
        )
        
        if not result:
            return
            
        # Clear cache
        try:
            cache_file = Path(BuildConfig.CACHE_FILE)
            if cache_file.exists():
                cache_file.unlink()
        except:
            pass
            
        self.start_build()
        
    def stop_build(self):
        """Stop build process"""
        # TODO: Implement graceful build stopping
        self.log_message("WARN", "Build stop requested (not implemented yet)")
        
    def update_ui_state(self, building):
        """Update UI state based on build status"""
        if building:
            self.build_btn.config(state='disabled')
            self.rebuild_btn.config(state='disabled')
            self.stop_btn.config(state='normal')
            self.status_label.config(text="Building...", style='Warning.TLabel')
        else:
            self.build_btn.config(state='normal')
            self.rebuild_btn.config(state='normal')
            self.stop_btn.config(state='disabled')
            
    def show_statistics(self):
        """Show build statistics"""
        try:
            manager = BuildManager(self.packages)
            
            # Capture stats output
            stats_window = tk.Toplevel(self.root)
            stats_window.title("Build Statistics")
            stats_window.geometry("600x400")
            
            text = scrolledtext.ScrolledText(stats_window, wrap=tk.WORD, font=('Consolas', 9))
            text.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)
            
            # Redirect stats output
            import io
            from contextlib import redirect_stdout
            
            f = io.StringIO()
            with redirect_stdout(f):
                manager.show_stats()
            
            stats_output = f.getvalue()
            text.insert(tk.END, stats_output)
            text.config(state='disabled')
            
        except Exception as e:
            messagebox.showerror("Error", f"Failed to load statistics: {e}")
            
    def show_dependency_graph(self):
        """Show dependency graph visualization"""
        try:
            graph = DependencyGraph(self.packages)
            
            # Create window
            graph_window = tk.Toplevel(self.root)
            graph_window.title("Dependency Graph")
            graph_window.geometry("600x400")
            
            text = scrolledtext.ScrolledText(graph_window, wrap=tk.WORD, font=('Consolas', 9))
            text.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)
            
            # Show analysis
            import io
            from contextlib import redirect_stdout
            
            f = io.StringIO()
            with redirect_stdout(f):
                graph.print_analysis()
            
            output = f.getvalue()
            text.insert(tk.END, output)
            text.config(state='disabled')
            
            # Add visualize button if matplotlib available
            if Setup.HAS_MATPLOTLIB:
                btn_frame = ttk.Frame(graph_window)
                btn_frame.pack(fill=tk.X, padx=10, pady=(0, 10))
                
                ttk.Button(
                    btn_frame,
                    text="Generate Visual Graph",
                    command=lambda: self.generate_visual_graph(graph)
                ).pack()
                
        except Exception as e:
            messagebox.showerror("Error", f"Failed to analyze dependencies: {e}")
            
    def generate_visual_graph(self, graph):
        """Generate visual dependency graph"""
        try:
            filename = filedialog.asksaveasfilename(
                defaultextension=".png",
                filetypes=[("PNG files", "*.png"), ("All files", "*.*")]
            )
            
            if filename:
                graph.visualize(Path(filename))
                messagebox.showinfo("Success", f"Graph saved to {filename}")
                
        except Exception as e:
            messagebox.showerror("Error", f"Failed to generate graph: {e}")
            
    def generate_presets(self):
        """Generate CMake presets"""
        try:
            # Get compiler settings
            cxx_compiler = None if self.cxx_compiler_var.get() == "default" else self.cxx_compiler_var.get()
            c_compiler = None if self.c_compiler_var.get() == "default" else self.c_compiler_var.get()
            
            generator = PresetGenerator(
                self.packages,
                compiler_c=c_compiler,
                compiler_cxx=cxx_compiler
            )
            
            generator.generate(Path("."))
            
            messagebox.showinfo("Success", "CMakePresets.json generated successfully!")
            self.log_message("SUCCESS", "CMakePresets.json generated")
            
        except Exception as e:
            messagebox.showerror("Error", f"Failed to generate presets: {e}")


def main():
    """Main entry point for GUI"""
    root = tk.Tk()
    app = BuildGUI(root)
    root.mainloop()


if __name__ == "__main__":
    # Check if we should run GUI or CLI
    if len(sys.argv) > 1 and sys.argv[1] != "--gui":
        # Run CLI version
        Setup.main()
    else:
        # Run GUI version
        main()