#pragma once

#include "Material.hpp"
#include "UUID.hpp"

#include <unordered_map>
#include <memory>
#include <string>

namespace Motion::Core
{
    class MaterialManager 
    {
        private:
            MaterialManager() = default;
            ~MaterialManager() = default;

            MaterialManager(const MaterialManager&) = delete;
            MaterialManager& operator=(const MaterialManager&) = delete; 
            MaterialManager(MaterialManager&&) = delete;
            MaterialManager& operator=(MaterialManager&&) = delete;

        public:
            static void Rgister(const std::string& name, const std::shared_ptr<Material>& material);
            static std::shared_ptr<Material> Get(const UUID& id);
            static std::shared_ptr<Material> Get(const std::string& name);
            static bool HasMaterial(const std::string& name);
            static bool HasMaterial(const UUID& name);
            static void Unregister(const UUID& id);
            static void Unregister(const std::string& name);

            static void Clear();
    };
}