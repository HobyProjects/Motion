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

            virtual bool Init() override;
            virtual void Quit() override;
            virtual BaseAPIs API() override { return BaseAPIs::GLFW; }
            virtual bool IsInitialized() const override { return m_Initialized; }

        private:
            bool m_Initialized{ false };    
    };

    class GLFW_Window final : public IWindow
    {
        public:
            GLFW_Window(WindowHandle windowHandle, const std::string& title, const std::shared_ptr<IContext> context);
            virtual ~GLFW_Window();

            virtual bool IsActive() const override { return m_Properties.IsActive; }
            virtual bool IsFocused() const override { return m_Properties.IsFocused; }
            virtual bool IsVSyncEnabled() const override { return m_Properties.IsVSyncEnabled; }
            virtual WindowHandle GetHandle() const override { return m_Properties.Handle; }
            
            virtual NativeWindow GetNativeWindow() const override { return m_Window; }
            virtual WindowProperties& GetProperties() override { return m_Properties; }
            virtual GraphicSettings& GetGraphicSettings() override { return m_Graphic->GetSettings(); }
            virtual void PollEvents() override;
            virtual void SwapBuffers() override;
            virtual void SetEventsCallbackFunc(const ApplicationCallbackFunction&) override;
            virtual void SetContext(const std::shared_ptr<IContext>& context) override;
            virtual std::shared_ptr<IContext> GetContext() const override;

        private:
            void SetEventsCallBacks();
            void RegisterEventsCallBacks();

        private:
            WindowProperties m_Properties{};
            GLFWwindow* m_Window{ nullptr };
            std::shared_ptr<IGraphic> m_Graphic{ nullptr };
            std::shared_ptr<IContext> m_Context{ nullptr };
            ApplicationCallbackFunction m_CallbackFunc{ nullptr };
    };
}