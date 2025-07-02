#include "DialogBoxes.hpp"

#ifdef MOTION_PLATFORM_WINDOWS
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#endif

#ifdef MOTION_PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <commdlg.h>
#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")
#endif

namespace Motion::Core
{
    std::filesystem::path DialogBoxes::OpenFileDialog(NativeWindow window, const std::string& caption, const std::string& filter, const std::filesystem::path& defaultPath)
    {
        #ifdef MOTION_PLATFORM_WINDOWS

        char szFile[MAX_PATH] = {};

        OPENFILENAMEA ofn;
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = glfwGetWin32Window((GLFWwindow*)window);
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);

        // Filter must be properly formatted with '\0'
        std::string formattedFilter = filter; // Ensure it's "Text Files\0*.txt\0All Files\0*.*\0\0"
        ofn.lpstrFilter = formattedFilter.c_str();

        ofn.nFilterIndex = 1;
        ofn.lpstrFileTitle = nullptr;
        ofn.nMaxFileTitle = 0;

        std::string initDirStr = defaultPath.string();
        ofn.lpstrInitialDir = initDirStr.c_str();

        ofn.lpstrTitle = caption.c_str();
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;

        if (GetOpenFileNameA(&ofn)) {
            return std::filesystem::path(ofn.lpstrFile);
        }

        // If the user cancels the dialog, return an empty path
        return std::filesystem::path();

        #else

        #error "OpenFileDialog is not implemented for this platform."
        
        #endif
    }

    std::filesystem::path DialogBoxes::SaveFileDialog(NativeWindow window, const std::string& caption, const std::string& filter, const std::filesystem::path& defaultPath)
    {
        #ifdef MOTION_PLATFORM_WINDOWS

        char szFile[MAX_PATH] = {};

        OPENFILENAMEA ofn;
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = glfwGetWin32Window((GLFWwindow*)window);
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);

        // IMPORTANT: The filter string must be double-null terminated
        std::string formattedFilter = filter; // Must be like: "Text Files\0*.txt\0All Files\0*.*\0\0"
        ofn.lpstrFilter = formattedFilter.c_str();
        ofn.nFilterIndex = 1;

        ofn.lpstrFileTitle = nullptr;
        ofn.nMaxFileTitle = 0;

        std::string initDirStr = defaultPath.string();
        ofn.lpstrInitialDir = initDirStr.c_str();

        ofn.lpstrTitle = caption.c_str();

        ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_EXPLORER;

        if (GetSaveFileNameA(&ofn)) {
            return std::filesystem::path(ofn.lpstrFile);
        }

        // If the user cancels the dialog, return an empty path
        return std::filesystem::path();

        #else

        #error "SaveFileDialog is not implemented for this platform."
        
        #endif
    }
}