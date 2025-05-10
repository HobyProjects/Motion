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
        SDL = Bits<2>::value,
        Win32 = Bits<3>::value
    };

    class IBaseAPI
    {
        public:
            IBaseAPI() = default;
            virtual ~IBaseAPI() = default;

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
            virtual WindowProperties GetProperties() const = 0;
            virtual void PollEvents() = 0;
            virtual void SwapBuffers() = 0;
            virtual void SetEventsCallbackFunc(const ApplicationCallbackFunction&) = 0;
            virtual void SetContext(const std::shared_ptr<IContext>& context) = 0;
            virtual std::shared_ptr<IContext> GetContext() const = 0;
    };
    
}