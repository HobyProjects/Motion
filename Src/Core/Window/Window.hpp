#pragma once

#include <memory>
#include <string>
#include <functional>
#include <unordered_map>

#include "Base.hpp"
#include "Event.hpp"
#include "Renderer.hpp"
#include "UUID.hpp"

namespace Motion::Core
{
    using WindowHandle = UUID;
    using NativeWindow = void*;

    enum class WindowState : std::uint32_t
    {
        FullScreen = Bits<0>::value,
        Minimized = Bits<1>::value,
        Maximized = Bits<2>::value,
        Normal = Bits<3>::value
    };

    struct WindowColorBit
    {
        std::uint32_t RedBit{ 0 }, GreenBit{ 0 }, BlueBit{ 0 }, AlphaBit{ 0 };
        std::uint32_t DepthBit{ 0 }, DepthStencilBit{ 0 };
    };

    struct WindowProperties
    {
        std::string Title{ "" };
        std::uint32_t Width{ 0 };
        std::uint32_t Height{ 0 };
        WindowColorBit ColorBits;
        std::uint32_t RefreshRate{ 60 };
        std::uint32_t FixedWidth{ 0 };
        std::uint32_t FixedHeight{ 0 };
        std::uint32_t MinWidth{ 1280 };
        std::uint32_t MinHeight{ 720 };
        std::uint32_t PosX{ 0 };
        std::uint32_t PosY{ 0 };
        WindowHandle Handle{ 0 };
        int32_t PixelWidth{ 0 };
        int32_t PixelHeight{ 0 };
        WindowState State{ WindowState::Maximized };
        bool IsVSyncEnabled{ false };
        bool IsActive{ false };
        bool IsFocused{ false };
    };

    enum class PlatformBaseAPIs : std::uint32_t
    {
        GLFW = Bits<1>::value,
        Win32 = Bits<2>::value
    };

    inline std::uint32_t operator|(WindowState a, WindowState b) { return static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b); }
    inline std::uint32_t operator&(WindowState a, WindowState b) { return static_cast<std::uint32_t>(a) & static_cast<std::uint32_t>(b); }

    inline std::uint32_t operator|(PlatformBaseAPIs a, PlatformBaseAPIs b) { return static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b); }
    inline std::uint32_t operator&(PlatformBaseAPIs a, PlatformBaseAPIs b) { return static_cast<std::uint32_t>(a) & static_cast<std::uint32_t>(b); }

    inline std::uint32_t operator|(PlatformBaseAPIs a, RenderingAPI b) { return static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b); }
    inline std::uint32_t operator&(PlatformBaseAPIs a, RenderingAPI b) { return static_cast<std::uint32_t>(a) & static_cast<std::uint32_t>(b); }
    inline std::uint32_t operator|(RenderingAPI a, PlatformBaseAPIs b) { return static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b); }
    inline std::uint32_t operator&(RenderingAPI a, PlatformBaseAPIs b) { return static_cast<std::uint32_t>(a) & static_cast<std::uint32_t>(b); }

    class IPlatformBaseAPI
    {
    public:
        IPlatformBaseAPI() = default;
        virtual ~IPlatformBaseAPI() = default;

        [[nodiscard]] virtual bool Init() noexcept = 0;
        [[nodiscard]] virtual PlatformBaseAPIs API() const noexcept = 0;
        [[nodiscard]] virtual bool IsInitialized() const noexcept = 0;

        virtual void Quit() noexcept = 0;
    };

    class IContext
    {
    public:
        IContext() = default;
        virtual ~IContext() = default;

        virtual bool Activate() noexcept = 0;
        virtual void Attach(NativeWindow) noexcept = 0;
        virtual void Detach() noexcept = 0;
        virtual void SwapBuffers(NativeWindow) noexcept = 0;

        [[nodiscard]] virtual bool IsContextCreated() const noexcept = 0;
        [[nodiscard]] virtual NativeWindow GetCurrentContext() const noexcept = 0;
    };

    class IWindow
    {
    public:
        IWindow() = default;
        virtual ~IWindow() = default;

        [[nodiscard]] virtual bool IsActive() const noexcept = 0;
        [[nodiscard]] virtual bool IsFocused() const noexcept = 0;
        [[nodiscard]] virtual bool IsVSyncEnabled() const noexcept = 0;
        [[nodiscard]] virtual WindowHandle GetHandle() const noexcept = 0;
        [[nodiscard]] virtual NativeWindow GetNativeWindow() const noexcept = 0;
        [[nodiscard]] virtual WindowProperties& GetProperties() noexcept = 0;
        [[nodiscard]] virtual std::shared_ptr<IContext> GetContext() const noexcept = 0;

        virtual void PollEvents() = 0;
        virtual void SwapBuffers() = 0;
        virtual void SetEventsCallbackFunc(const EventProcessingFunction&) = 0;
        virtual void SetContext(const std::shared_ptr<IContext>& context) = 0;
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
        /**
         * @brief Returns the singleton instance of CoreAPI.
         *
         * This method ensures that only one instance of CoreAPI exists throughout the application.
         * It initializes the instance if it does not already exist.
         *
         * @return Reference to the singleton CoreAPI instance.
         */
        [[nodiscard]] static CoreAPI& GetInstance() noexcept
        {
            static CoreAPI instance;
            return instance;
        }

    public:
        [[nodiscard]] bool Init() noexcept;
        [[nodiscard]] PlatformBaseAPIs API() const noexcept;
        [[nodiscard]] std::shared_ptr<IPlatformBaseAPI> GetBaseAPI() const noexcept;

        void Quit() noexcept;

    private:
        std::shared_ptr<IPlatformBaseAPI> m_PlatformBaseAPIService{ nullptr };
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
        /**
         * @brief Returns the singleton instance of WindowManager.
         *
         * This method ensures that only one instance of WindowManager exists throughout the application.
         * It initializes the instance if it does not already exist.
         *
         * @return Reference to the singleton WindowManager instance.
         */
        [[nodiscard]] static WindowManager& GetInstance() noexcept
        {
            static WindowManager instance;
            return instance;
        }

    public:
        [[nodiscard]] std::shared_ptr<IWindow> Create(const std::string& title) noexcept;
        [[nodiscard]] std::shared_ptr<IWindow> GetWindow(WindowHandle handle) const noexcept;
        [[nodiscard]] std::shared_ptr<IWindow> GetActiveWindow() const noexcept;

        void Destroy(WindowHandle windowHandle) noexcept;

    private:
        std::unordered_map<WindowHandle, std::shared_ptr<IWindow>> m_WindowManagementService;
    };

}