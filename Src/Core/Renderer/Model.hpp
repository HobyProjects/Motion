#pragma once

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <filesystem>
#include <nlohmann/json.hpp>

#include "Mesh.hpp"

namespace Motion::Core
{
    class Model
    {
        public:
            Model(const std::string& name, const std::shared_ptr<IShader>& shader): Shader(shader), Name(name){};
            ~Model() = default;

            void Render(const glm::mat4& modelTransForm);
            bool LoadFrom_glTF(const std::filesystem::path& modelPath);

        private:
            std::shared_ptr<IShader> Shader{ nullptr };
            std::vector<std::shared_ptr<SubMesh>> SubMeshes{};
            std::string Name{ "" };
    };
}