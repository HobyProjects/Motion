#!/usr/bin/env python3

"""
Motion Engine Build System - Universal Launcher
Automatically detects whether to run GUI or CLI mode
"""

import sys
import os

def main():
    # Check if GUI should be used
    use_gui = False
    
    # GUI mode if:
    # 1. No arguments provided
    # 2. --gui flag is present
    # 3. DISPLAY environment variable is set (Linux/Mac with X11)
    
    if len(sys.argv) == 1:
        # No arguments - try GUI if display available
        use_gui = has_display()
    elif "--gui" in sys.argv:
        # Explicit GUI request
        sys.argv.remove("--gui")
        use_gui = True
    
    if use_gui:
        try:
            import SetupGUI
            SetupGUI.main()
        except ImportError as e:
            print(f"Error: Could not load GUI module: {e}")
            print("Falling back to CLI mode...")
            import Setup
            Setup.main()
        except Exception as e:
            print(f"Error starting GUI: {e}")
            print("Falling back to CLI mode...")
            import Setup
            Setup.main()
    else:
        # Run CLI mode
        import Setup
        Setup.main()

def has_display():
    """Check if a display is available for GUI"""
    if sys.platform == "win32":
        # Windows always has display
        return True
    elif sys.platform == "darwin":
        # macOS always has display
        return True
    else:
        # Linux/Unix - check DISPLAY variable
        return "DISPLAY" in os.environ

if __name__ == "__main__":
    main()