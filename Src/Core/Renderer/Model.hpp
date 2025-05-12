#pragma once

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <filesystem>

namespace Motion::Core
{
    class Model
    {

    };

    std::shared_ptr<Model> LoadModelFromFile(const std::string& modelName, const std::filesystem::path& modelPath);
}