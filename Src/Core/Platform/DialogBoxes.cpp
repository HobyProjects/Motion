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
    static const char* ALL_FILES_FILTER = "All Files\0*.*\0\0";
    static const char* TEXT_FILES_FILTER = "Text Files\0*.txt\0\0";
    static const char* IMAGE_FILES_FILTER = "Image Files\0*.png;*.jpg;*.jpeg;*.tga;*.bmp;*.dds\0\0";
    static const char* TEXTURE_FILES_FILTER = "Texture Files\0*.png;*.jpg;*.jpeg;*.tga;*.bmp;*.dds\0\0";
    static const char* MODEL_FILES_FILTER = "StaticMesh Files\0*.fbx;*.obj;*.gltf;*.glb;*.dae;*.stl;*.ply;\0\0";
    static const char* SHADER_FILES_FILTER = "Shader Files\0*.glsl;*.hlsl\0\0";
    static const char* FONT_FILES_FILTER = "Font Files\0*.ttf;*.otf\0\0";
    static const char* AUDIO_FILES_FILTER = "Audio Files\0*.wav;*.mp3\0\0";
    static const char* VIDEO_FILES_FILTER = "Video Files\0*.mp4;*.mkv;*.avi\0\0";

    std::filesystem::path DialogBoxes::OpenFileDialog(NativeWindow window, const std::string& caption, FileType fileType, const std::filesystem::path& defaultPath)
    {
#ifdef MOTION_PLATFORM_WINDOWS

        char szFile[MAX_PATH] = {};

        OPENFILENAMEA ofn;
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = glfwGetWin32Window((GLFWwindow*)window);
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);

        switch (fileType)
        {
        case FileType::AllFiles: ofn.lpstrFilter = ALL_FILES_FILTER; break;
        case FileType::TextFile: ofn.lpstrFilter = TEXT_FILES_FILTER; break;
        case FileType::TextureFile: ofn.lpstrFilter = TEXTURE_FILES_FILTER; break;
        case FileType::ModelFile: ofn.lpstrFilter = MODEL_FILES_FILTER; break;
        case FileType::ShaderFile: ofn.lpstrFilter = SHADER_FILES_FILTER; break;
        case FileType::FontFile: ofn.lpstrFilter = FONT_FILES_FILTER; break;
        case FileType::AudioFile: ofn.lpstrFilter = AUDIO_FILES_FILTER; break;
        case FileType::VideoFile: ofn.lpstrFilter = VIDEO_FILES_FILTER; break;
        case FileType::ImageFile: ofn.lpstrFilter = IMAGE_FILES_FILTER; break;
        };

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

    std::filesystem::path DialogBoxes::SaveFileDialog(NativeWindow window, const std::string& caption, FileType fileType, const std::filesystem::path& defaultPath)
    {
#ifdef MOTION_PLATFORM_WINDOWS

        char szFile[MAX_PATH] = {};

        OPENFILENAMEA ofn;
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = glfwGetWin32Window((GLFWwindow*)window);
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);

        switch (fileType)
        {
        case FileType::AllFiles: ofn.lpstrFilter = ALL_FILES_FILTER; break;
        case FileType::TextFile: ofn.lpstrFilter = TEXT_FILES_FILTER; break;
        case FileType::TextureFile: ofn.lpstrFilter = TEXTURE_FILES_FILTER; break;
        case FileType::ModelFile: ofn.lpstrFilter = MODEL_FILES_FILTER; break;
        case FileType::ShaderFile: ofn.lpstrFilter = SHADER_FILES_FILTER; break;
        case FileType::FontFile: ofn.lpstrFilter = FONT_FILES_FILTER; break;
        case FileType::AudioFile: ofn.lpstrFilter = AUDIO_FILES_FILTER; break;
        case FileType::VideoFile: ofn.lpstrFilter = VIDEO_FILES_FILTER; break;
        case FileType::ImageFile: ofn.lpstrFilter = IMAGE_FILES_FILTER; break;
        };

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