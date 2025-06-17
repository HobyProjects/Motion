#pragma once

#include "Window.hpp"

namespace Motion::Core
{
    class UI
    {
        private:
            UI() = default;
            ~UI() = default;

            UI(const UI&) = delete;
            UI& operator=(const UI&) = delete;
            UI(UI&&) = delete;
            UI& operator=(UI&&) = delete;

        public:
            static void Init(WindowHandle whnd);
            static void Quit();

            static void UseColorDark();
            static void UseColorLight();
    };
}