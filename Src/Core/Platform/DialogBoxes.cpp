#include "CorePCH.hpp"
#include <system_error>
#include <cassert>

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#include <glfw/glfw3native.h>
#include <wrl/client.h>
#include <shellapi.h>
#include <commdlg.h>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shell32.lib")
#endif

namespace Motion 
{
    #ifdef _WIN32
    using Microsoft::WRL::ComPtr;

    namespace 
    {
        bool g_comInitialized = false;
        ComPtr<IFileOpenDialog> g_openDlg;
        ComPtr<IFileSaveDialog> g_saveDlg;

        inline std::wstring ToWide(const std::string& s) 
        {
            if (s.empty()) return L"";
            int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
            std::wstring out(n ? n - 1 : 0, L'\0');
            if (n > 1) MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, out.data(), n);
            return out;
        }

        inline std::string ToUTF8(const std::wstring& ws) 
        {
            if (ws.empty()) return {};
            int n = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, nullptr, 0, nullptr, nullptr);
            std::string out(n ? n - 1 : 0, '\0');
            if (n > 1) WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, out.data(), n, nullptr, nullptr);
            return out;
        }

        inline void ApplyFilters(IFileDialog* dlg, const std::vector<FileFilter>& filters, const std::wstring& defExt) {
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
            dlg->SetFileTypeIndex(1); // 1-based index
            if (!defExt.empty()) dlg->SetDefaultExtension(defExt.c_str());
        }

        inline void ApplyCommonOptions(IFileDialog* dlg, const std::wstring& title, const std::filesystem::path& initialDir) 
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

        inline HWND TryGetParentHwnd() 
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

    } 

    #endif // _WIN32

    bool DialogBoxes::InitializeCOM() 
    {
#ifdef _WIN32
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
#ifdef _WIN32
        if (g_comInitialized) 
        {
            g_openDlg.Reset();
            g_saveDlg.Reset();
            CoUninitialize();
            g_comInitialized = false;
        }
#endif
    }

#ifdef _WIN32

    bool DialogBoxes::EnsureOpenDialog() 
    {
        if (g_openDlg) return true;
        HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(g_openDlg.ReleaseAndGetAddressOf()));
        return SUCCEEDED(hr);
    }

    bool DialogBoxes::EnsureSaveDialog() 
    {
        if (g_saveDlg) return true;
        HRESULT hr = CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(g_saveDlg.ReleaseAndGetAddressOf()));
        return SUCCEEDED(hr);
    }

#endif

    std::filesystem::path DialogBoxes::OpenFileDialog(const OpenDialogOptions& opt) 
    {
#ifdef _WIN32
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
                            results = std::filesystem::path(p);
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
                    results = std::filesystem::path(p);
                    CoTaskMemFree(p);
                }
            }
        }

        return results;
#else
        return {};
#endif

    }

    std::optional<std::filesystem::path> DialogBoxes::SaveFileDialog(const SaveDialogOptions& opt) 
    {
#ifdef _WIN32

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
                std::filesystem::path chosen(p);
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
} // namespace Motion
