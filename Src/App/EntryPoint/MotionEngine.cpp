#include "Application.hpp"


int main(int argc, char* argv[])
{
    auto& logger = Motion::Core::Loggers::GetInstance();
    logger.Initialize();

    auto& coreAPI = Motion::Core::CoreAPI::GetInstance();
    if (coreAPI.Init())
    {
        Motion::App::Application app;
        app.Start();
    }
    else
    {
        MOTION_CORE_ERROR("Failed to initialize Core API!");
        return -1;
    }

    coreAPI.Quit();
    MOTION_CORE_INFO("Application exited successfully.");
    return 0;
}