#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>

#include <glm/glm.hpp>

namespace Motion::Core
{
    using ShaderProgramID   = uint32_t;
    using UniformLocation   = uint32_t;

    class IShader
    {
        public:
            IShader() = default;
            virtual ~IShader() = default;

            virtual void Bind() const = 0;
            virtual void Unbind() const = 0;

            virtual ShaderProgramID ProgramID() const = 0;
            virtual std::string& GetName() const = 0;
            virtual UniformLocation GetUniformLocation(const std::string& uniformName) const = 0;

            virtual void SetUniform(const std::string& uniformName, float value) = 0;
            virtual void SetUniform(const std::string& uniformName, int32_t value) = 0;
            virtual void SetUniform(const std::string& uniformName, uint32_t value) = 0;
            virtual void SetUniform(const std::string& uniformName, const glm::vec2& value) = 0;
            virtual void SetUniform(const std::string& uniformName, const glm::vec3& value) = 0;
            virtual void SetUniform(const std::string& uniformName, const glm::vec4& value) = 0;
            virtual void SetUniform(const std::string& uniformName, const glm::mat2& value) = 0;
            virtual void SetUniform(const std::string& uniformName, const glm::mat3& value) = 0;
            virtual void SetUniform(const std::string& uniformName, const glm::mat4& value) = 0;
    };
}

