#pragma once

#include <imgui/imgui.h>
#include <imgui/imgui_impl_opengl3.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_internal.h>

#include "Window.hpp"

namespace Motion::Core
{
    class UI
    {
        private:
            UI() = default;
            ~UI() = default;


        public:
            static void Init(WindowHandle whnd);
            static void Quit();

            static void UseColorDark();
            static void UseColorLight();
    };
}