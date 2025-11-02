#pragma once

#include <string>
#include <vector>
#include <optional>
#include <filesystem>

namespace Motion 
{

    struct FileFilter 
    {
        std::wstring Name;
        std::wstring Pattern;
    };

    struct OpenDialogOptions 
    {
        std::wstring Title;                        
        std::wstring DefaultExtension;             
        std::filesystem::path InitialDirectory;    
        std::vector<FileFilter> Filters;            
        bool AllowMultiSelect{false};
        bool PickFolders{false};                    
    };

    struct SaveDialogOptions 
    {
        std::wstring Title;                      
        std::wstring DefaultExtension;            
        std::filesystem::path InitialDirectory;
        std::vector<FileFilter> Filters;
        bool OverwritePrompt{true};               
    };

    enum class SystemFolder
    {
        Desktop,
        Documents,
        Downloads,
        UserFolder
    };

    class DialogBoxes 
    {
        public:
            static bool InitializeCOM();
            static void UninitializeCOM();

            static std::filesystem::path GetSystemFolder(SystemFolder folder = SystemFolder::Documents);
            static std::filesystem::path OpenFileDialog(const OpenDialogOptions& opt);
            static std::optional<std::filesystem::path> SaveFileDialog(const SaveDialogOptions& opt);
            static std::filesystem::path SelectFolderDialog(const std::wstring& title = L"Select a folder", const std::filesystem::path& initialDir = {});

        private:

#ifdef MOTION_PLATFORM_WINDOWS

            static bool EnsureOpenDialog();
            static bool EnsureSaveDialog();
#endif

    };

} 
