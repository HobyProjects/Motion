#include "DialogBoxes.hpp"

#ifdef MOTION_PLATFORM_WINDOWS

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <windows.h>
#include <commdlg.h>
#include <algorithm>
#include <string>

#endif

namespace Motion
{

#ifdef MOTION_PLATFORM_WINDOWS

    /**
     * @brief Converts a wide string (std::wstring) to a UTF-8 encoded std::string.
     *
     * This function uses the Windows API WideCharToMultiByte to perform the conversion.
     * After conversion, all backslashes ('\\') in the resulting string are replaced with forward slashes ('/').
     *
     * @param wstr The wide string to convert.
     * @return A UTF-8 encoded std::string representation of the input wide string.
     */
    static std::string WideToUtf8(const std::wstring& wstr)
    {
        if (wstr.empty()) return {};

        int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), nullptr, 0, nullptr, nullptr);
        std::string result(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), result.data(), size_needed, nullptr, nullptr);

        std::replace(result.begin(), result.end(), '\\', '/');
        return result;
    }

    /**
     * @brief Initializes an OPENFILENAMEW structure for use with file dialog boxes.
     *
     * This function sets up the OPENFILENAMEW structure with the provided parameters,
     * preparing it for use with Windows file dialog APIs. It zeroes out the file buffer
     * and the structure, sets the owner window, file buffer, filter, and title, and
     * applies standard flags to ensure valid file and path selection.
     *
     * @param ofn Reference to the OPENFILENAMEW structure to initialize.
     * @param szFile Pointer to a buffer that will receive the selected file path.
     * @param bufferSize Size of the szFile buffer, in characters.
     * @param filter File type filter string (pairs of description and pattern, separated by '\0').
     * @param title Title of the dialog box.
     */
    static void InitializeFileDialog(OPENFILENAMEW& ofn, wchar_t* szFile, DWORD bufferSize, const std::wstring& filter, const std::wstring& title)
    {
        ZeroMemory(szFile, bufferSize);
        ZeroMemory(&ofn, sizeof(ofn));

        auto& windowManager = WindowManager::GetInstance();
        std::shared_ptr<IWindow> window = windowManager.GetActiveWindow();

        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = glfwGetWin32Window((GLFWwindow*)window->GetNativeWindow());
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = bufferSize;
        ofn.lpstrFilter = filter.c_str();
        ofn.nFilterIndex = 1;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;
        ofn.lpstrTitle = title.c_str();
        std::wstring initDirStr = std::filesystem::current_path().wstring();
        ofn.lpstrInitialDir = initDirStr.c_str();
        ofn.lpstrDefExt = L"";
        ofn.lpstrFileTitle = nullptr;
    }


    /**
     * @brief Opens a file dialog for the user to select a file.
     *
     * Displays a standard Windows "Open File" dialog box with the specified filter and title.
     * If the user selects a file, the function returns the file path as a UTF-8 encoded string.
     * If the user cancels the dialog or an error occurs, returns std::nullopt.
     * Logs critical errors if the dialog fails or an exception is thrown.
     *
     * @param filter The file type filter for the dialog (e.g., L"Text Files (*.txt)\0*.txt\0").
     * @param title The title of the dialog window.
     * @return std::optional<std::string> The selected file path in UTF-8 encoding, or std::nullopt if no file was selected or an error occurred.
     */
    std::optional<std::string> DialogBoxes::OpenFileDialog(const std::wstring& filter, const std::wstring& title)
    {
        try
        {
            wchar_t szFile[MAX_PATH];
            OPENFILENAMEW ofn;
            InitializeFileDialog(ofn, szFile, MAX_PATH, filter, title);

            if (GetOpenFileNameW(&ofn))
            {
                return WideToUtf8(ofn.lpstrFile);
            }
            else
            {
                DWORD err = CommDlgExtendedError();
                if (err != 0)
                {
                    MOTION_CORE_CRITICAL("Dialog Error: {}", err);
                }
                return std::nullopt;
            }
        }
        catch (const std::exception& e)
        {
            MOTION_CORE_CRITICAL("Exception in OpenFileDialog: {}", e.what());
            return std::nullopt;
        }
    }


    /**
     * @brief Displays a Save File dialog box and returns the selected file path.
     *
     * This function shows a standard Windows Save File dialog using the specified filter and title.
     * If the user selects a file and confirms, the file path is returned as a UTF-8 encoded string.
     * If the user cancels or an error occurs, std::nullopt is returned.
     * Any errors encountered during the dialog operation are logged.
     *
     * @param filter The file type filter string (e.g., L"Text Files (*.txt)\0*.txt\0").
     * @param title The title of the dialog window.
     * @return std::optional<std::string> The selected file path in UTF-8 encoding, or std::nullopt if cancelled or an error occurs.
     */
    std::optional<std::string> DialogBoxes::SaveFileDialog(const std::wstring& filter, const std::wstring& title)
    {
        try
        {
            wchar_t szFile[MAX_PATH];
            OPENFILENAMEW ofn;
            InitializeFileDialog(ofn, szFile, MAX_PATH, filter, title);

            ofn.Flags |= OFN_OVERWRITEPROMPT;

            if (GetSaveFileNameW(&ofn))
            {
                return WideToUtf8(ofn.lpstrFile);
            }
            else
            {
                DWORD err = CommDlgExtendedError();
                if (err != 0)
                {
                    MOTION_CORE_CRITICAL("Dialog Error: {}", err);
                }

                return std::nullopt;
            }
        }
        catch (const std::exception& e)
        {
            MOTION_CORE_CRITICAL("Exception in SaveFileDialog: {}", e.what());
            return std::nullopt;
        }

    }

#else
#error "OpenFileDialog and SaveFileDialog are only implemented for Windows."
#endif

}