#pragma once

#include <memory>
#include <string>
#include <functional>
#include <unordered_map>

#include "Base.hpp"
#include "Event.hpp"
#include "Renderer.hpp"

namespace Motion::Core
{
    using WindowHandle = uint32_t;
    using NativeWindow = void*;

    enum class WindowState : uint32_t
    {
        FullScreen = Bits<0>::value,
        Minimized = Bits<1>::value,
        Maximized = Bits<2>::value,
        Normal = Bits<3>::value
    };

    inline uint32_t operator|(WindowState a, WindowState b) { return static_cast<uint32_t>(a) | static_cast<uint32_t>(b); }
    inline uint32_t operator&(WindowState a, WindowState b) { return static_cast<uint32_t>(a) & static_cast<uint32_t>(b); }
    inline uint32_t operator^(WindowState a, WindowState b) { return static_cast<uint32_t>(a) ^ static_cast<uint32_t>(b); }
    inline uint32_t operator~(WindowState a) { return ~static_cast<uint32_t>(a); }

    struct WindowColorBit
    {
        uint32_t RedBit{ 0 }, GreenBit{ 0 }, BlueBit{ 0 }, AlphaBit{ 0 };
        uint32_t DepthBit{ 0 }, DepthStencilBit{ 0 };
    };

    struct WindowProperties
    {
        std::string Title{ "" };
        uint32_t Width{ 0 };
        uint32_t Height{ 0 };
        WindowColorBit ColorBits;
        uint32_t RefreshRate{ 60 };
        uint32_t FixedWidth{ 0 };
        uint32_t FixedHeight{ 0 };
        uint32_t MinWidth{ 1280 };
        uint32_t MinHeight{ 720 };
        uint32_t PosX{ 0 };
        uint32_t PosY{ 0 };
        WindowHandle Handle{ 0 };
        int32_t PixelWidth{ 0 };
        int32_t PixelHeight{ 0 };
        WindowState State{ WindowState::Maximized };
        bool IsVSyncEnabled{ false };
        bool IsActive{ false };
        bool IsFocused{ false };
    };

    enum class BaseAPIs : uint32_t
    {
        GLFW = Bits<1>::value,
        Win32 = Bits<2>::value
    };

    inline uint32_t operator|(BaseAPIs a, BaseAPIs b) { return static_cast<uint32_t>(a) | static_cast<uint32_t>(b); }
    inline uint32_t operator&(BaseAPIs a, BaseAPIs b) { return static_cast<uint32_t>(a) & static_cast<uint32_t>(b); }
    inline uint32_t operator^(BaseAPIs a, BaseAPIs b) { return static_cast<uint32_t>(a) ^ static_cast<uint32_t>(b); }
    inline uint32_t operator~(BaseAPIs a) { return ~static_cast<uint32_t>(a); }
    
    inline uint32_t operator|(BaseAPIs a, RenderingAPI b) { return static_cast<uint32_t>(a) | static_cast<uint32_t>(b); }
    inline uint32_t operator&(BaseAPIs a, RenderingAPI b) { return static_cast<uint32_t>(a) & static_cast<uint32_t>(b); }
    inline uint32_t operator^(BaseAPIs a, RenderingAPI b) { return static_cast<uint32_t>(a) ^ static_cast<uint32_t>(b); }

    class IPlatformBaseAPI
    {
        public:
            IPlatformBaseAPI() = default;
            virtual ~IPlatformBaseAPI() = default;

            virtual bool Init() = 0;
            virtual void Quit() = 0;
            virtual BaseAPIs API() = 0;
            virtual bool IsInitialized() const = 0;
    };

    class IContext
    {
        public:
            IContext() = default;
            virtual ~IContext() = default;

            virtual void Attach(NativeWindow) = 0;
            virtual void Detach() = 0;

            virtual bool IsContextCreated() const = 0;
            virtual NativeWindow GetCurrentContext() const = 0;
            virtual void SwapBuffers(NativeWindow) = 0;
    };

    class IWindow
    {
        public:
            IWindow() = default;
            virtual ~IWindow() = default;

            virtual bool IsActive() const = 0;
            virtual bool IsFocused() const = 0;
            virtual bool IsVSyncEnabled() const = 0;
            virtual WindowHandle GetHandle() const = 0;
            virtual NativeWindow GetNativeWindow() const = 0;
            virtual WindowProperties& GetProperties() = 0;
            virtual void PollEvents() = 0;
            virtual void SwapBuffers() = 0;
            virtual void SetEventsCallbackFunc(const ApplicationCallbackFunction&) = 0;
            virtual void SetContext(const std::shared_ptr<IContext>& context) = 0;
            virtual std::shared_ptr<IContext> GetContext() const = 0;
    };

    class CoreAPI
    {
        private:
            CoreAPI() = default;
            ~CoreAPI() = default;

            CoreAPI(const CoreAPI&) = delete;
            CoreAPI& operator=(const CoreAPI&) = delete;
            CoreAPI(CoreAPI&&) = delete;
            CoreAPI& operator=(CoreAPI&&) = delete;

        public:
            static bool Init();
            static void Quit();

            static BaseAPIs API();
            static std::shared_ptr<IPlatformBaseAPI> GetBaseAPI();
            static std::shared_ptr<IContext> GetContext();
    };

    class WindowManager
    {
        private:
            WindowManager() = default;
            ~WindowManager() = default;

            WindowManager(const WindowManager&) = delete;
            WindowManager& operator=(const WindowManager&) = delete;
            WindowManager(WindowManager&&) = delete;
            WindowManager& operator=(WindowManager&&) = delete;

        public:
            static WindowHandle UniqueHandle();
            static std::shared_ptr<IWindow> Create(const std::string& title);
            static void Destroy(std::shared_ptr<IWindow>& window);
            static std::shared_ptr<IWindow> Get(WindowHandle handle);
    };
    
}