#pragma once

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "Window.hpp"

namespace Motion::Core
{
    class GLFW_BaseAPI final : public IPlatformBaseAPI
    {
    public:
        GLFW_BaseAPI() = default;
        virtual ~GLFW_BaseAPI() = default;

        [[nodiscard]] virtual bool Init() noexcept override;
        [[nodiscard]] virtual PlatformBaseAPIs API() const noexcept override { return PlatformBaseAPIs::GLFW; }
        [[nodiscard]] virtual bool IsInitialized() const noexcept override { return m_Initialized; }

        virtual void Quit() noexcept override;

    private:
        bool m_Initialized{ false };
    };

    class GLFW_Window final : public IWindow
    {
    public:
        GLFW_Window(WindowHandle windowHandle, const std::string& title);
        virtual ~GLFW_Window();

        [[nodiscard]] virtual bool IsActive() const noexcept override { return m_Properties.IsActive; }
        [[nodiscard]] virtual bool IsFocused() const noexcept override { return m_Properties.IsFocused; }
        [[nodiscard]] virtual bool IsVSyncEnabled() const noexcept override { return m_Properties.IsVSyncEnabled; }
        [[nodiscard]] virtual WindowHandle GetHandle() const noexcept override { return m_Properties.Handle; }
        [[nodiscard]] virtual NativeWindow GetNativeWindow() const noexcept override { return m_Window; }
        [[nodiscard]] virtual WindowProperties& GetProperties() noexcept override { return m_Properties; }
        [[nodiscard]] virtual std::shared_ptr<IContext> GetContext() const noexcept override;

        virtual void PollEvents() noexcept override;
        virtual void SwapBuffers() noexcept override;
        virtual void SetEventsCallbackFunc(const EventProcessingFunction&) noexcept override;
        virtual void SetContext(const std::shared_ptr<IContext>& context) noexcept override;

    private:
        void SetEventsCallBacks();
        void RegisterEventsCallBacks();

    private:
        WindowProperties m_Properties{};
        GLFWwindow* m_Window{ nullptr };
        std::shared_ptr<IContext> m_Context{ nullptr };
        EventProcessingFunction m_CallbackFunc{ nullptr };
    };
}