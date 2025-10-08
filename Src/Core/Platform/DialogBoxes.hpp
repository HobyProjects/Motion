#pragma once
#include <string>
#include <vector>
#include <optional>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#include <shobjidl.h>
#endif

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

    class DialogBoxes 
    {
        public:
            static bool InitializeCOM();
            static void UninitializeCOM();

            static std::filesystem::path GetDocumentsFolder();
            static std::filesystem::path OpenFileDialog(const OpenDialogOptions& opt);
            static std::optional<std::filesystem::path> SaveFileDialog(const SaveDialogOptions& opt);

        private:
#ifdef _WIN32
            static bool EnsureOpenDialog();
            static bool EnsureSaveDialog();
#endif

    };

} 
