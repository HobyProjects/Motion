#pragma once

#include <yaml-cpp/yaml.h>
#include <glm/glm.hpp>
#include <filesystem>

#include "UUID.hpp"
#include "Material.hpp"

namespace YAML
{
    template<>
    struct convert<glm::vec3>
    {
        static Node encode(const glm::vec3& rhs)
        {
            Node node;
            node.push_back(rhs.x);
            node.push_back(rhs.y);
            node.push_back(rhs.z);
            return node;
        }

        static bool decode(const Node& node, glm::vec3& rhs)
        {
            if (!node.IsSequence() || node.size() != 3)
                return false;    

            rhs.x = node[0].as<float>();    
            rhs.y = node[1].as<float>();    
            rhs.z = node[2].as<float>();    
            return true;
        }
    };

    template<>
    struct convert<glm::vec4>
    {
        static Node encode(const glm::vec4& rhs)
        {
            Node node;
            node.push_back(rhs.x);
            node.push_back(rhs.y);
            node.push_back(rhs.z);
            node.push_back(rhs.w);
            return node;
        }

        static bool decode(const Node& node, glm::vec4& rhs)
        {
            if (!node.IsSequence() || node.size() != 4)
                return false;

            rhs.x = node[0].as<float>();
            rhs.y = node[1].as<float>();
            rhs.z = node[2].as<float>();
            rhs.w = node[3].as<float>();
            return true;
        }
    };

    template<>
    struct convert<glm::mat4>
    {
        static Node encode(const glm::mat4& rhs)
        {
            Node node;
            node.push_back(rhs[0]);
            node.push_back(rhs[1]);
            node.push_back(rhs[2]);
            node.push_back(rhs[3]);
            return node;
        }

        static bool decode(const Node& node, glm::mat4& rhs)
        {
            if (!node.IsSequence() || node.size() != 4)
                return false;

            rhs[0] = node[0].as<glm::vec4>();
            rhs[1] = node[1].as<glm::vec4>();
            rhs[2] = node[2].as<glm::vec4>();
            rhs[3] = node[3].as<glm::vec4>();
            return true;
        }
    };

    template<>
    struct convert<glm::mat3>
    {
        static Node encode(const glm::mat3& rhs)
        {
            Node node;
            node.push_back(rhs[0]);
            node.push_back(rhs[1]);
            node.push_back(rhs[2]);
            return node;
        }

        static bool decode(const Node& node, glm::mat3& rhs)
        {
            if (!node.IsSequence() || node.size() != 3)
                return false;

            rhs[0] = node[0].as<glm::vec3>();
            rhs[1] = node[1].as<glm::vec3>();
            rhs[2] = node[2].as<glm::vec3>();
            return true;
        }
    };

    template<>
    struct convert<std::filesystem::path>
    {
        static Node encode(const std::filesystem::path& rhs)
        {
            return Node(rhs.string());
        }

        static bool decode(const Node& node, std::filesystem::path& rhs)
        {
            rhs = node.as<std::string>();
            return true;
        }

    };

    template<>
    struct convert<Motion::Core::Material::ShadingMethod>
    {
        static Node encode(const Motion::Core::Material::ShadingMethod& rhs)
        {
            return Node((int)rhs);
        }

        static bool decode(const Node& node, Motion::Core::Material::ShadingMethod& rhs)
        {
            rhs = (Motion::Core::Material::ShadingMethod)node.as<int>();
            return true;
        }
    };

}

namespace Motion::Core
{
    class MaterialSerializer
    {
        private:
            MaterialSerializer() = default;
            ~MaterialSerializer() = default;

            MaterialSerializer(MaterialSerializer const&) = delete;
            MaterialSerializer& operator=(MaterialSerializer const&) = delete;
            MaterialSerializer(MaterialSerializer&&) = delete;
            MaterialSerializer& operator=(MaterialSerializer&&) = delete;
            
        public:
            static bool Serialize(const std::filesystem::path& filePath, const std::shared_ptr<Material>& material);
            static std::shared_ptr<Material> Deserialize(const std::filesystem::path& filePath);
    };
}