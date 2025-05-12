#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include <glm/glm.hpp>
#include <filesystem>

namespace Motion::Core
{
    using json = nlohmann::json;

    class Model
    {

    };

    std::shared_ptr<Model> LoadModelFromFile(const std::string& modelName, const std::filesystem::path& modelPath);
}