#pragma once

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <filesystem>

#include "Mesh.hpp"

namespace Motion::Core
{
    class Model
    {
        public:
            Model(std::shared_ptr<IShader> shader): Shader(shader){};
            ~Model() = default;

            void Render();

        public:
            glm::mat4 ModelTransform{ 1.0f };
            std::shared_ptr<IShader> Shader{ nullptr };
            std::vector<std::shared_ptr<Mesh>> Meshes{};
    };

    std::shared_ptr<Model> ImportModelFromFile(const std::string& modelName, const std::filesystem::path& modelPath);
}