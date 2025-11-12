#pragma once

#include "Asserts.hpp"
#include "SceneSerializer.hpp"
#include "YAML_ConvertExternal.hpp"

namespace Motion
{
    class YAMLSerializationStrategy : public ISerializationStrategy
    {
    public:
        YAMLSerializationStrategy() = default;
        ~YAMLSerializationStrategy() override = default;

        SerializationResult Serialize(const SerializedScene& sceneData, const std::filesystem::path& path) override
        {
            try
            {
                if (path.empty())
                {
                    return SerializationResult::Fail("Empty file path");
                }

                YAML::Node root = YAML::convert<SerializedScene>::encode(sceneData);
                std::filesystem::path absPath = std::filesystem::absolute(path);
                
                if (absPath.has_parent_path())
                {
                    std::error_code ec;
                    std::filesystem::create_directories(absPath.parent_path(), ec);
                    if (ec)
                    {
                        return SerializationResult::Fail(
                            std::format("Failed to create directories: {}", ec.message())
                        );
                    }
                }

                std::ofstream file(absPath, std::ios::out | std::ios::trunc);
                if (!file.is_open())
                {
                    return SerializationResult::Fail(
                        std::format("Failed to open file for writing: {}", absPath.string())
                    );
                }

                file << root << std::endl;
                file.close();

                MOTION_CORE_INFO("Scene serialized to: {}", absPath.string());
                return SerializationResult::Ok(absPath.string());
            }
            catch (const YAML::Exception& e)
            {
                return SerializationResult::Fail(
                    std::format("YAML error: {}", e.what())
                );
            }
            catch (const std::exception& e)
            {
                return SerializationResult::Fail(
                    std::format("Serialization error: {}", e.what())
                );
            }
        }

        std::optional<SerializedScene> Deserialize(
            const std::filesystem::path& path
        ) override
        {
            try
            {
                if (!std::filesystem::exists(path))
                {
                    MOTION_CORE_ERROR("File not found: {}", path.string());
                    return std::nullopt;
                }

                YAML::Node root = YAML::LoadFile(path.string());
                
                if (!root["Scene"].IsDefined())
                {
                    MOTION_CORE_ERROR("Invalid scene file format");
                    return std::nullopt;
                }

                SerializedScene scene;
                if (!YAML::convert<SerializedScene>::decode(root, scene))
                {
                    MOTION_CORE_ERROR("Failed to decode scene data");
                    return std::nullopt;
                }

                MOTION_CORE_INFO("Scene deserialized from: {}", path.string());
                return scene;
            }
            catch (const YAML::Exception& e)
            {
                MOTION_CORE_ERROR("YAML error: {}", e.what());
                return std::nullopt;
            }
            catch (const std::exception& e)
            {
                MOTION_CORE_ERROR("Deserialization error: {}", e.what());
                return std::nullopt;
            }
        }
    };

} 