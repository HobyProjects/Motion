#include <iostream>
#include "Application.hpp"


int main(int argc, char* argv[]) 
{
    Motion::App::Application* app = new Motion::App::Application();
    app->Start();
    delete app;

    std::cin.get();
    return 0;
}