#include "Application.hpp"

using namespace Motion;

int main(int argc, char* argv[])
{
    auto& logger = Loggers::GetInstance();
    logger.Initialize();

    auto& coreAPI = CoreAPI::GetInstance();
    if (coreAPI.Init())
    {
        Application app;
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