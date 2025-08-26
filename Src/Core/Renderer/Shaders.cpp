#include "CorePCH.hpp"

namespace Motion
{
    void IShader::DeleteShaderProgram(ShaderProgramID programID)
    {
        switch (Renderer::GetAPI())
        {
            case RenderingAPI::OpenGL:         GL_Shader::DeleteShaderProgram(programID);                   break;
            case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!");      break;
            case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!");     break;
            default:                           MOTION_ASSERT(false, "Unknown rendering API!");              break;
        };
    }

    ShaderProgramID IShader::CreateShaderProgram()
    {
        switch (Renderer::GetAPI())
        {
            case RenderingAPI::OpenGL:         return GL_Shader::CreateShaderProgram();
            case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!");      return 0;
            case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!");     return 0;
            default:                           MOTION_ASSERT(false, "Unknown rendering API!");              return 0;
        };
    }

    void IShader::AttachShaderProgram(ShaderID shaderID, ShaderProgramID programID)
    {
        switch (Renderer::GetAPI())
        {
            case RenderingAPI::OpenGL:         GL_Shader::AttachShaderProgram(shaderID, programID);         break;
            case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!");      break;
            case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!");     break;
            default:                           MOTION_ASSERT(false, "Unknown rendering API!");              break;
        };
    }

    ShaderID IShader::CompileShader(ShaderType shaderType, const std::string& sourceCode)
    {
        switch (Renderer::GetAPI())
        {
            case RenderingAPI::OpenGL:         return GL_Shader::CompileShader(shaderType, sourceCode);
            case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!");      return 0;
            case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!");     return 0;
            default:                           MOTION_ASSERT(false, "Unknown rendering API!");              return 0;
        };
    }

    void IShader::LinkShaderProgram(ShaderProgramID programID)
    {
        switch (Renderer::GetAPI())
        {
            case RenderingAPI::OpenGL:         GL_Shader::LinkShaderProgram(programID);                 break;
            case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!");  break;
            case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); break;
            default:                           MOTION_ASSERT(false, "Unknown rendering API!");          break;
        };
    }

    void IShader::ValidateShaderProgram(ShaderProgramID programID)
    {
        switch (Renderer::GetAPI())
        {
            case RenderingAPI::OpenGL:         GL_Shader::ValidateShaderProgram(programID);             break;
            case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!");  break;
            case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); break;
            default:                           MOTION_ASSERT(false, "Unknown rendering API!");          break;
        };
    }

    static ShaderType GetShaderTypeFromString(const std::string& typeStr)
    {
        if (typeStr == "vertex")                      return ShaderType::Vertex;
        if (typeStr == "fragment")                    return ShaderType::Fragment;
        if (typeStr == "geometry")                    return ShaderType::Geometry;
        if (typeStr == "compute")                     return ShaderType::Compute;
        if (typeStr == "tessellation_control")        return ShaderType::TessellationControl;
        if (typeStr == "tessellation_evaluation")     return ShaderType::TessellationEvaluation;

        return ShaderType::None;
    }

    std::string IShader::ReadShaderFile(const std::filesystem::path& filePath)
    {
        if (!std::filesystem::exists(filePath))
        {
            MOTION_ASSERT(false, "Shader file does not exist: {0}", filePath.string());
            return {};
        }

        std::ifstream file(filePath);
        if (!file.is_open())
        {
            MOTION_ASSERT(false, "Failed to open shader file: {0}", filePath.string());
            return {};
        }

        std::string sourceCode((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();
        return sourceCode;
    }

    std::unordered_map<ShaderType, std::string> IShader::ReadFullShaderFile(const std::filesystem::path& filePath)
    {
        std::string source = ReadShaderFile(filePath);
        if (source.empty())
            return {};

        std::unordered_map<ShaderType, std::string> shaderSources;
        std::regex typeRegex(R"(#type\s+(\w+))");
        std::sregex_iterator it(source.begin(), source.end(), typeRegex);
        std::sregex_iterator end;

        std::vector<std::pair<size_t, ShaderType>> shaderPositions;
        for (; it != end; ++it)
        {
            std::string typeStr = (*it)[1];
            ShaderType shaderType = GetShaderTypeFromString(typeStr);
            if (shaderType == ShaderType::None)
            {
                MOTION_CORE_WARN("Unknown shader type: {0}", typeStr);
                continue;
            }
            shaderPositions.emplace_back(it->position(), shaderType);
        }

        for (size_t i = 0; i < shaderPositions.size(); ++i)
        {
            size_t begin = source.find('\n', shaderPositions[i].first) + 1;
            size_t end = (i + 1 < shaderPositions.size()) ? shaderPositions[i + 1].first : source.size();
            shaderSources[shaderPositions[i].second] = source.substr(begin, end - begin);
        }

        return shaderSources;
    }

    std::unordered_map<ShaderType, std::string> IShader::ReadShaderFiles(const std::filesystem::path& vertexPath, const std::filesystem::path& fragmentPath)
    {
        if (!std::filesystem::exists(vertexPath) || !std::filesystem::exists(fragmentPath))
        {
            MOTION_ASSERT(false, "One or both shader files do not exist: {0}, {1}", vertexPath.string(), fragmentPath.string());
            return {};
        }

        std::unordered_map<ShaderType, std::string> shaderSources;
        shaderSources[ShaderType::Vertex]       = ReadShaderFile(vertexPath);
        shaderSources[ShaderType::Fragment]     = ReadShaderFile(fragmentPath);

        if (shaderSources[ShaderType::Vertex].empty() || shaderSources[ShaderType::Fragment].empty())
        {
            MOTION_ASSERT(false, "Failed to read shader files: {0}, {1}", vertexPath.string(), fragmentPath.string());
            return {};
        }

        return shaderSources;
    }

    static std::string CreateDefinitions(ShaderFeatureMask features)
    {
        using SFM = ShaderFeatureMask;
        std::ostringstream ss;

        if (features & SFM::USE_SH9)                        ss << "#define USE_SH9 1\n";
        if (features & SFM::USE_ORM_MAP)                    ss << "#define USE_ORM_MAP 1\n";
        if (features & SFM::USE_OPACITY_MAP)                ss << "#define USE_OPACITY_MAP 1\n";
        if (features & SFM::USE_ALPHA_MODE_OPAQUE)          ss << "#define USE_ALPHA_MODE_OPAQUE 1\n";
        if (features & SFM::USE_ALPHA_MODE_MASK)            ss << "#define USE_ALPHA_MODE_MASK 1\n";
        if (features & SFM::USE_ALPHA_MODE_BLEND)           ss << "#define USE_ALPHA_MODE_BLEND 1\n";
        if (features & SFM::USE_ALPHA_BLEND_PREMULTIPLIED)  ss << "#define USE_ALPHA_BLEND_PREMULTIPLIED 1\n";

        ss << "#define CAMERA_UBO_BINDING "     << ShaderVariant::CAMERA_UBO_BINDING    << "\n";
        ss << "#define OBJECT_UBO_BINDING "     << ShaderVariant::OBJECT_UBO_BINDING    << "\n";
        ss << "#define LIGHT_UBO_BINDING "      << ShaderVariant::LIGHT_UBO_BINDING     << "\n";
        ss << "#define MATERIAL_UBO_BINDING "   << ShaderVariant::MATERIAL_UBO_BINDING  << "\n";
        ss << "#define SH9_UBO_BINDING "        << ShaderVariant::SH9_UBO_BINDING       << "\n";

        return ss.str();
    }

    static std::string InjectDefinitions(const std::string& src, const std::string& defineBlock)
    {
        if (defineBlock.empty()) return src;

        static const std::regex kMarker(R"(//\s*\[\s*FEATURES_ENABLE_DISABLE\s*\]\s*)");
        if (std::regex_search(src, kMarker)) 
        {
            return std::regex_replace(src, kMarker, defineBlock + "\n", std::regex_constants::format_first_only);
        }

        auto nextNL = [&](size_t p){ return src.find('\n', p); };
        size_t lineStart = 0;
        while (lineStart < src.size() && (src[lineStart]==' '||src[lineStart]=='\t'||src[lineStart]=='\r'||src[lineStart]=='\n'))
            ++lineStart;

        if (lineStart < src.size() && src.compare(lineStart, 8, "#version") == 0)
        {
            size_t insertAt = nextNL(lineStart);
            insertAt = (insertAt == std::string::npos) ? src.size() : insertAt + 1;

            while (insertAt < src.size()) 
            {
                size_t peek = insertAt;
                while (peek < src.size() && (src[peek]==' '||src[peek]=='\t'||src[peek]=='\r')) ++peek;
                if (peek < src.size() && src.compare(peek, 10, "#extension") == 0) 
                {
                    size_t extEnd = nextNL(peek);
                    insertAt = (extEnd == std::string::npos) ? src.size() : extEnd + 1;

                } else break;
            }

            std::string out;
            out.reserve(src.size() + defineBlock.size() + 8);
            out.append(src, 0, insertAt);
            out.append(defineBlock);
            if (!defineBlock.empty() && defineBlock.back()!='\n') out.push_back('\n');
            out.append(src, insertAt, std::string::npos);
            return out;
        }

        std::string out = defineBlock;
        if (!defineBlock.empty() && defineBlock.back()!='\n') out.push_back('\n');
        out += src;
        return out;
    }

    std::shared_ptr<IShader> ShaderVariant::GetVariant(UUID baseUUID, const std::string & baseName, const std::filesystem::path & sourceFile, ShaderFeatureMask features)
    {
        ShaderVariantKey key{ baseUUID, features, sourceFile };
        auto it = m_ShaderCache.find(key);
        if (it != m_ShaderCache.end())
        {
            if(!it->second.expired())
            {
                auto spit = it->second.lock();
                return spit;
            }
        }

        std::string                                 defineBlock     = CreateDefinitions(features);
        std::unordered_map<ShaderType, std::string> shaderSources   = IShader::ReadFullShaderFile(sourceFile);
        if (shaderSources.empty())
        {
            MOTION_ASSERT(false, "Failed to read shader sources from: {0}", sourceFile.string());
            return nullptr;
        }

        for (auto& [type, src] : shaderSources)
            src = InjectDefinitions(src, defineBlock);

        UUID vID        = UniqueIdentity::GetUniqueID();
        auto name       = std::format("{}_{}", baseName, static_cast<std::uint32_t>(features));

        std::shared_ptr<IShader> shader{ nullptr };
        switch(Renderer::GetAPI())
        {
            case RenderingAPI::OpenGL:      shader  = std::make_shared<GL_Shader>(vID, name, shaderSources, sourceFile);    break;
            case RenderingAPI::Vulkan:      MOTION_ASSERT(false, "Vulkan support not implemented yet!");                    break;
            case RenderingAPI::DirectX:     MOTION_ASSERT(false, "DirectX support not implemented yet!");                   break;
            default:                        MOTION_ASSERT(false, "Unknown rendering API!");                                 break;
        }

        m_ShaderCache[key] = shader;
        return shader;
    }

    void ShaderVariant::Invalidate(UUID baseUUID)
    {
        for (auto it = m_ShaderCache.begin(); it != m_ShaderCache.end(); )
        {
            if (it->first.VariantID == baseUUID) it = m_ShaderCache.erase(it);
            else ++it;
        }
    }
}