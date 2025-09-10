#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>

#include <glm/glm.hpp>

#include "Asset.hpp"
#include "UUID.hpp"

#define INVALID_UNIFORM_LOCATION  -1

namespace Motion
{
    using ShaderID          = std::int32_t;
    using ShaderProgramID   = std::int32_t;
    using UniformLocation   = std::int32_t;

    enum class ShaderType : std::int32_t
    {
        None                    = 0,
        Vertex                  = 1,
        Fragment                = 2,
        Geometry                = 3,
        Compute                 = 4,
        TessellationControl     = 5,
        TessellationEvaluation  = 6
    };

    inline std::int32_t operator|(ShaderType a, ShaderType b) { return static_cast<std::int32_t>(a) | static_cast<std::int32_t>(b); }
    inline std::int32_t operator&(ShaderType a, ShaderType b) { return static_cast<std::int32_t>(a) & static_cast<std::int32_t>(b); }

    class IShader : public IAsset
    {
    public:
        IShader()           = default;
        virtual ~IShader()  = default;

        virtual void Bind()     const = 0;
        virtual void Unbind()   const = 0;

        virtual void SetUniform(const std::string_view uniformName,             float value) = 0;
        virtual void SetUniform(const std::string_view uniformName,      std::int32_t value) = 0;
        virtual void SetUniform(const std::string_view uniformName,  const glm::vec2& value) = 0;
        virtual void SetUniform(const std::string_view uniformName,  const glm::vec3& value) = 0;
        virtual void SetUniform(const std::string_view uniformName,  const glm::vec4& value) = 0;
        virtual void SetUniform(const std::string_view uniformName,  const glm::mat2& value) = 0;
        virtual void SetUniform(const std::string_view uniformName,  const glm::mat3& value) = 0;
        virtual void SetUniform(const std::string_view uniformName,  const glm::mat4& value) = 0;
        virtual void SetUniform(const std::string_view uniformName,  std::int32_t size, std::uint32_t* values) = 0;

        [[nodiscard]] virtual ShaderProgramID   ProgramID() const = 0;
        [[nodiscard]] virtual std::string       GetName()   const = 0;
        [[nodiscard]] virtual UniformLocation   GetUniformLocation(const std::string_view uniformName) = 0;

        static void AttachShaderProgram(ShaderID shaderID, ShaderProgramID programID);
        static void LinkShaderProgram(ShaderProgramID programID);
        static void ValidateShaderProgram(ShaderProgramID programID);
        static void DeleteShaderProgram(ShaderProgramID programID);

        [[nodiscard]] static ShaderProgramID    CreateShaderProgram();
        [[nodiscard]] static std::string        ReadShaderFile(const std::filesystem::path& filePath);
        [[nodiscard]] static ShaderID           CompileShader(ShaderType shaderType, const std::string& sourceCode);
        
        [[nodiscard]] static std::unordered_map<ShaderType, std::string> ReadFullShaderFile(const std::filesystem::path& filePath);
        [[nodiscard]] static std::unordered_map<ShaderType, std::string> ReadShaderFiles(const std::filesystem::path& vertexPath, const std::filesystem::path& fragmentPath);
    };

    enum class ShaderFeatureMask : std::uint32_t
    {
        USE_NONE                            = 0,
        USE_BASECOLOR_MAP                   = BIT(0),
        USE_NORMAL_MAP                      = BIT(1),
        USE_OCCLUSION_MAP                   = BIT(2),
        USE_ROUGHNESS_MAP                   = BIT(3),
        USE_METALLIC_MAP                    = BIT(4),
        USE_EMISSIVE_MAP                    = BIT(5),
        USE_ORM_MAP                         = BIT(6),
    };

    inline std::uint32_t operator|(ShaderFeatureMask a, ShaderFeatureMask b) { return static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b); }
    inline std::uint32_t operator&(ShaderFeatureMask a, ShaderFeatureMask b) { return static_cast<std::uint32_t>(a) & static_cast<std::uint32_t>(b); }
    inline std::uint32_t operator|=(ShaderFeatureMask& a, ShaderFeatureMask b) { a = static_cast<ShaderFeatureMask>(a | b); return static_cast<std::uint32_t>(a); }
    inline std::uint32_t operator&=(ShaderFeatureMask& a, ShaderFeatureMask b) { a = static_cast<ShaderFeatureMask>(a & b); return static_cast<std::uint32_t>(a); }

    struct ShaderVariantKey
    {
        UUID                    VariantID;
        ShaderFeatureMask       Features;
        std::filesystem::path   SourceFiles;
    };

    struct ShaderVariantHashCode
    {
        std::size_t operator()(const ShaderVariantKey& key) const
        {
            std::size_t hash    = std::hash<UUID>()(key.VariantID);
            hash               ^= std::hash<ShaderFeatureMask>()(key.Features);
            hash               ^= std::hash<std::filesystem::path>()(key.SourceFiles);
            return hash;
        }
    };

    class ShaderVariant
    {
    private:
        ShaderVariant()     = default;
        ~ShaderVariant()    = default;

        ShaderVariant(const ShaderVariant&)             = delete;
        ShaderVariant& operator=(const ShaderVariant&)  = delete;
        ShaderVariant(ShaderVariant&&)                  = delete;
        ShaderVariant& operator=(ShaderVariant&&)       = delete;

    public:
        static ShaderVariant& GetInstance()
        {
            static ShaderVariant instance;
            return instance;
        }

    public:
        std::shared_ptr<IShader> GetVariant(const std::filesystem::path& sourceFile, ShaderFeatureMask features);
        void VariantWrite(const std::string& name, ShaderType type, const std::string& source, const std::filesystem::path& location);

    private:
        std::unordered_map<std::size_t, std::shared_ptr<IShader>> m_ShaderCache;
    };
}