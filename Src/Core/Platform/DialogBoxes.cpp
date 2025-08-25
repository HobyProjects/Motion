#include "DialogBoxes.hpp"

#ifdef MOTION_PLATFORM_WINDOWS

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <windows.h>
#include <shobjidl.h> // For IFileDialog
#include <string>
#include <iostream>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shell32.lib")


#endif

#include <string>
#include <locale>
#include <codecvt>

namespace Motion
{

#ifdef MOTION_PLATFORM_WINDOWS

    std::string ToString(const std::wstring& wstr)
    {
        if (wstr.empty())
            return std::string();

        // Convert wide UTF-16 string to UTF-8 using Win32 API
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (size_needed <= 0)
            return std::string();

        std::string result(size_needed, '\0');
        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &result[0], size_needed, nullptr, nullptr);

        // Remove the trailing null added by WideCharToMultiByte
        if (!result.empty() && result.back() == '\0')
            result.pop_back();

        return result;
    }

    std::string DialogBoxes::OpenFileDialog()
    {
        try
        {
            std::wstring result;

            HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
            if (SUCCEEDED(hr))
            {
                IFileOpenDialog* pFileOpen = nullptr;

                // Create the FileOpenDialog object.
                hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER,
                    IID_PPV_ARGS(&pFileOpen));

                if (SUCCEEDED(hr))
                {
                    // Show the Open dialog box.
                    hr = pFileOpen->Show(nullptr);

                    // Get the file name from the dialog box.
                    if (SUCCEEDED(hr))
                    {
                        IShellItem* pItem = nullptr;
                        hr = pFileOpen->GetResult(&pItem);
                        if (SUCCEEDED(hr))
                        {
                            PWSTR pszFilePath = nullptr;
                            hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

                            // Save result
                            if (SUCCEEDED(hr))
                                result = pszFilePath;

                            CoTaskMemFree(pszFilePath);
                            pItem->Release();
                        }
                    }
                    pFileOpen->Release();
                }
                CoUninitialize();
            }

            return result.empty() ? std::string() : ToString(result);
        }
        catch (const std::exception& e)
        {
            MOTION_CORE_CRITICAL("Exception in OpenFileDialog: {}", e.what());
            return std::string();
        }
    }

    std::string DialogBoxes::SaveFileDialog()
    {
        try
        {
            std::wstring result;

            HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
            if (SUCCEEDED(hr))
            {
                IFileSaveDialog* pFileSave = nullptr;

                // Create the FileSaveDialog object.
                hr = CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_INPROC_SERVER,
                    IID_PPV_ARGS(&pFileSave));

                if (SUCCEEDED(hr))
                {
                    // Show the Save dialog box.
                    hr = pFileSave->Show(nullptr);

                    // Get the file name from the dialog box.
                    if (SUCCEEDED(hr))
                    {
                        IShellItem* pItem = nullptr;
                        hr = pFileSave->GetResult(&pItem);
                        if (SUCCEEDED(hr))
                        {
                            PWSTR pszFilePath = nullptr;
                            hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

                            // Save result
                            if (SUCCEEDED(hr))
                                result = pszFilePath;

                            CoTaskMemFree(pszFilePath);
                            pItem->Release();
                        }
                    }
                    pFileSave->Release();
                }
                CoUninitialize();
            }

            return result.empty() ? std::string() : ToString(result);
        }
        catch (const std::exception& e)
        {
            MOTION_CORE_CRITICAL("Exception in SaveFileDialog: {}", e.what());
            return std::string();
        }

    }

#else
#error "OpenFileDialog and SaveFileDialog are only implemented for Windows."
#endif

}