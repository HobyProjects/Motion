#include "CorePCH.hpp"
#include "DialogBoxes.hpp"

#include <system_error>
#include <cassert>

#ifdef MOTION_PLATFORM_WINDOWS

#ifndef NOMINMAX
#  define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <knownfolders.h>   // for FOLDERID_Documents
#include <shlobj.h>         // SHGetKnownFolderPath
#include <shellapi.h>
#include <wrl/client.h>
#include <combaseapi.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <glfw/glfw3native.h>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shell32.lib")

#endif

namespace Motion 
{

#ifdef MOTION_PLATFORM_WINDOWS

    using Microsoft::WRL::ComPtr;

    namespace 
    {
        bool g_comInitialized = false;
        ComPtr<IFileOpenDialog> g_openDlg;
        ComPtr<IFileSaveDialog> g_saveDlg;

        static std::wstring ToWide(const std::string& s) 
        {
            if (s.empty()) return L"";
            int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
            std::wstring out(n ? n - 1 : 0, L'\0');
            if (n > 1) MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, out.data(), n);
            return out;
        }

        static std::string ToUTF8(const std::wstring& ws) 
        {
            if (ws.empty()) return {};
            int n = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, nullptr, 0, nullptr, nullptr);
            std::string out(n ? n - 1 : 0, '\0');
            if (n > 1) WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, out.data(), n, nullptr, nullptr);
            return out;
        }

        static std::filesystem::path FromShellPWSTR(PWSTR p)
        {
            if (!p) return {};
            size_t n = wcslen(p);
            std::wstring ws;
            ws.assign(p, p + n);

            const bool isUNC      = ws.rfind(L"\\\\", 0) == 0 && ws.rfind(L"\\\\?\\", 0) != 0;
            const bool hasPrefix  = ws.rfind(L"\\\\?\\", 0) == 0;
            if (!hasPrefix) 
            {
                if (isUNC) 
                {
                    std::wstring tail = ws.substr(2);
                    ws = L"\\\\?\\UNC\\" + tail;
                } else if (ws.size() >= 248 ) 
                {
                    ws = L"\\\\?\\" + ws;
                }
            }

            return std::filesystem::path(ws);
        }

        static void ApplyFilters(IFileDialog* dlg, const std::vector<FileFilter>& filters, const std::wstring& defExt) 
        {
            if (filters.empty()) 
            {
                if (!defExt.empty()) dlg->SetDefaultExtension(defExt.c_str());
                return;
            }

            std::vector<COMDLG_FILTERSPEC> specs;
            specs.reserve(filters.size());
            for (auto& f : filters) 
            {
                specs.push_back(COMDLG_FILTERSPEC{ f.Name.c_str(), f.Pattern.c_str() });
            }

            dlg->SetFileTypes(static_cast<UINT>(specs.size()), specs.data());
            dlg->SetFileTypeIndex(1); 
            if (!defExt.empty()) dlg->SetDefaultExtension(defExt.c_str());
        }

        static void ApplyCommonOptions(IFileDialog* dlg, const std::wstring& title, const std::filesystem::path& initialDir) 
        {
            if (!title.empty()) dlg->SetTitle(title.c_str());

            if (!initialDir.empty()) 
            {
                ComPtr<IShellItem> folder;
                std::wstring abs = initialDir.wstring();
                if (SUCCEEDED(SHCreateItemFromParsingName(abs.c_str(), nullptr, IID_PPV_ARGS(&folder)))) 
                {
                    dlg->SetFolder(folder.Get());   
                    dlg->SetDefaultFolder(folder.Get());
                }
            }
        }

        static HWND TryGetParentHwnd() 
        {
            auto& WM = WindowManager::GetInstance();
            auto wnd = WM.GetActiveWindow();
            if( wnd && wnd->IsFocused()) 
            {
                auto native = wnd->GetNativeWindow();
                if(native)
                {
                    return glfwGetWin32Window(static_cast<GLFWwindow*>(native));
                }
            }

            return nullptr;
        }

        static GUID GetFolderID(SystemFolder folder)
        {
            switch(folder)
            {
                case SystemFolder::Desktop:         return FOLDERID_Desktop;
                case SystemFolder::Documents:       return FOLDERID_Documents;
                case SystemFolder::Downloads:       return FOLDERID_LocalDownloads;
                case SystemFolder::UserFolder:      return FOLDERID_UsersFiles;
                default:                            return FOLDERID_Documents;
            };
        }
    } 

#endif 

    bool DialogBoxes::InitializeCOM() 
    {
#ifdef MOTION_PLATFORM_WINDOWS
        if (g_comInitialized) return true;
        HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
        g_comInitialized = SUCCEEDED(hr);
        return g_comInitialized;
#else
        return false;
#endif
    }

    void DialogBoxes::UninitializeCOM() 
    {
#ifdef MOTION_PLATFORM_WINDOWS
        if (g_comInitialized) 
        {
            g_openDlg.Reset();
            g_saveDlg.Reset();
            CoUninitialize();
            g_comInitialized = false;
        }
#endif
    }

#ifdef MOTION_PLATFORM_WINDOWS

    /**
     * @brief Ensures that a IFileDialog* instance for file opening is available.
     * This function should be called before attempting to open a file dialog.
     * @return true if the instance is available, false otherwise.
     */
    bool DialogBoxes::EnsureOpenDialog() 
    {
        if (g_openDlg) return true;
        HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(g_openDlg.ReleaseAndGetAddressOf()));
        return SUCCEEDED(hr);
    }

    /**
     * @brief Ensures that a IFileDialog* instance for file saving is available.
     * @return true if the instance is available, false otherwise.
     * @note This function is only applicable on Windows and is a no-op on other platforms.
     * @warning The function does not check if the instance is valid or if it has already been released.
     *         It is the responsibility of the caller to ensure that the instance is properly cleaned up.
     */
    bool DialogBoxes::EnsureSaveDialog() 
    {
        if (g_saveDlg) return true;
        HRESULT hr = CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(g_saveDlg.ReleaseAndGetAddressOf()));
        return SUCCEEDED(hr);
    }

#endif

    /**
     * @brief Returns the path of the specified system folder.
     * @param folder The system folder to get the path of.
     * @return The path of the specified system folder, or an empty path if the function fails.
     * @note This function is currently only implemented for Windows.
     */
    std::filesystem::path DialogBoxes::GetSystemFolder(SystemFolder folder)
    {
#ifdef MOTION_PLATFORM_WINDOWS
        PWSTR path = nullptr;
        if (SUCCEEDED(SHGetKnownFolderPath(GetFolderID(folder), 0, nullptr, &path))) 
        {
            std::filesystem::path p = FromShellPWSTR(path);
            CoTaskMemFree(path);
            return p;
        }
#endif
        return {};
    }

    /**
     * @brief Opens a file dialog box for selecting one or more files.
     *
     * On Windows, this function will use the IFileOpenDialog COM interface to open a file dialog box.
     * On other platforms, this function will return an empty std::filesystem::path.
     *
     * @param opt The options for the file dialog box.
     * @return The path of the selected file(s) or an empty path if the dialog was cancelled.
     */
    std::filesystem::path DialogBoxes::OpenFileDialog(const OpenDialogOptions& opt) 
    {
#ifdef MOTION_PLATFORM_WINDOWS

        std::filesystem::path results{};
        if (!g_comInitialized) InitializeCOM();
        if (!EnsureOpenDialog()) return results;

        g_openDlg->ClearClientData();
        DWORD opts = 0;
        g_openDlg->GetOptions(&opts);

        opts |= FOS_FORCEFILESYSTEM;
        if (opt.AllowMultiSelect) opts |= FOS_ALLOWMULTISELECT;
        if (opt.PickFolders)      opts |= FOS_PICKFOLDERS;

        g_openDlg->SetOptions(opts);

        ApplyFilters(g_openDlg.Get(), opt.Filters, opt.DefaultExtension);
        ApplyCommonOptions(g_openDlg.Get(), opt.Title, opt.InitialDirectory);

        HRESULT hr = g_openDlg->Show(TryGetParentHwnd());
        if (FAILED(hr)) return results; 

        if (opt.AllowMultiSelect) 
        {
            ComPtr<IShellItemArray> items;
            if (SUCCEEDED(g_openDlg->GetResults(&items)) && items) 
            {
                DWORD count = 0;
                items->GetCount(&count);
                for (DWORD i = 0; i < count; ++i) 
                {
                    ComPtr<IShellItem> it;
                    if (SUCCEEDED(items->GetItemAt(i, &it)) && it) 
                    {
                        PWSTR p = nullptr;
                        if (SUCCEEDED(it->GetDisplayName(SIGDN_FILESYSPATH, &p)) && p) 
                        {
                            results = FromShellPWSTR(p);
                            CoTaskMemFree(p);
                        }
                    }
                }
            }
        } 
        else 
        {
            ComPtr<IShellItem> it;
            if (SUCCEEDED(g_openDlg->GetResult(&it)) && it) 
            {
                PWSTR p = nullptr;
                if (SUCCEEDED(it->GetDisplayName(SIGDN_FILESYSPATH, &p)) && p) 
                {
                    results = FromShellPWSTR(p);
                    CoTaskMemFree(p);
                }
            }
        }

        return results;
#else
        return {};
#endif

    }

    /**
     * @brief Show a save file dialog box
     * @param opt Options for the save dialog box
     * @return A std::optional containing the path of the chosen file, or std::nullopt if the dialog was cancelled
     * @remarks
     * On Windows, this function uses the IFileSaveDialog COM interface to show the dialog box.
     * On other platforms, this function always returns std::nullopt.
     */
    std::optional<std::filesystem::path> DialogBoxes::SaveFileDialog(const SaveDialogOptions& opt) 
    {
#ifdef MOTION_PLATFORM_WINDOWS

        if (!g_comInitialized) InitializeCOM();
        if (!EnsureSaveDialog()) return std::nullopt;

        g_saveDlg->ClearClientData();
        DWORD opts = 0;
        g_saveDlg->GetOptions(&opts);
        opts |= FOS_FORCEFILESYSTEM;
        if (opt.OverwritePrompt) opts |= FOS_OVERWRITEPROMPT;
        g_saveDlg->SetOptions(opts);

        ApplyFilters(g_saveDlg.Get(), opt.Filters, opt.DefaultExtension);
        ApplyCommonOptions(g_saveDlg.Get(), opt.Title, opt.InitialDirectory);

        HRESULT hr = g_saveDlg->Show(TryGetParentHwnd());
        if (FAILED(hr)) return std::nullopt;

        ComPtr<IShellItem> it;
        if (SUCCEEDED(g_saveDlg->GetResult(&it)) && it) 
        {
            PWSTR p = nullptr;
            if (SUCCEEDED(it->GetDisplayName(SIGDN_FILESYSPATH, &p)) && p) 
            {
                std::filesystem::path chosen = FromShellPWSTR(p);
                CoTaskMemFree(p);
                if (!opt.DefaultExtension.empty() && chosen.has_extension() == false) 
                {
                    std::filesystem::path withExt = chosen;
                    withExt += L"." + opt.DefaultExtension;
                    return withExt;
                }

                return chosen;
            }
        }

        return std::nullopt;
#else
        return std::nullopt;
#endif

    }

    /**
     * @brief Opens a folder dialog.
     * @param title The title of the dialog.
     * @param initialDir The initial directory of the dialog.
     * @return The path of the selected folder.
     * @note This function is only available on Windows.
     */
    std::filesystem::path DialogBoxes::SelectFolderDialog(const std::wstring & title, const std::filesystem::path & initialDir)
    {
#ifdef MOTION_PLATFORM_WINDOWS
        OpenDialogOptions opt{};
        opt.Title            = title;
        opt.InitialDirectory = initialDir;
        opt.PickFolders      = true;  
        return OpenFileDialog(opt);
#else
        return {};
#endif
    }
} 
