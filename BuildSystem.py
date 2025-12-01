#!/usr/bin/env python3

"""
Motion Engine Build System - Universal Launcher
Automatically detects whether to run GUI or CLI mode
"""

import sys
import os

def main():
    use_gui = False

    if len(sys.argv) == 1:
        use_gui = has_display()
    elif "--gui" in sys.argv:
        sys.argv.remove("--gui")
        use_gui = True
    
    if use_gui:
        try:
            import SetupGUI
            SetupGUI.main()
        except ImportError as e:
            print(f"Error: Could not load GUI module: {e}")
            print("Falling back to CLI mode...")
            import SetupCLI
            SetupCLI.main()
        except Exception as e:
            print(f"Error starting GUI: {e}")
            print("Falling back to CLI mode...")
            import SetupCLI
            SetupCLI.main()
    else:
        import SetupCLI
        SetupCLI.main()

def has_display():
    """Check if a display is available for GUI"""
    if sys.platform == "win32":
        return True
    elif sys.platform == "darwin":
        return True
    else:
        return "DISPLAY" in os.environ

if __name__ == "__main__":
    main()