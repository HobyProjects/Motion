#include "CorePCH.hpp"
#include "CommandQueue.hpp"
#include "Renderer.hpp"

namespace Motion
{
    static void ConfigureRegular()
    {
        auto rs             = Renderer::GetStageController();
        StageStatus state   = rs->Snapshot();

        state.DepthTest         = true;
        state.DepthWrite        = false;
        state.DepthFunc         = DepthFunction::LessEqual;

        state.CullEnabled       = false;
        state.Wireframe         = false;

        state.BlendEnabled      = true;
        state.SrcRGB            = BlendFactor::SrcAlpha;
        state.DstRGB            = BlendFactor::OneMinusSrcAlpha;
        state.SrcA              = BlendFactor::One;
        state.DstA              = BlendFactor::OneMinusSrcAlpha;
        state.BlendEqRGB        = BlendEquation::Add;
        state.BlendEqA          = BlendEquation::Add;

        state.ColorMaskR        = state.ColorMaskG = state.ColorMaskB = state.ColorMaskA = true;
        state.ScissorEnabled    = false;
        
        rs->Apply(state);
    }

    void CommandQueue::Sort()
    {
        auto frontToBack = [](const RenderCommand& a, const RenderCommand& b) { return a < b; };
        std::sort(m_CommandQueue.begin(), m_CommandQueue.end(), frontToBack);
    }

    void CommandQueue::EnsureInitialized()
    {
        std::call_once(m_InitOnce, [&]{
            m_WhiteTexture   = ITexture::Create(1, 1, glm::vec3(1.0f, 1.0f, 1.0f));
            m_BlackTexture   = ITexture::Create(1, 1, glm::vec3(0.0f, 0.0f, 0.0f));
            m_GrayTexture    = ITexture::Create(1, 1, glm::vec3(0.5f, 0.5f, 0.5f));
            m_NormalTexture  = ITexture::Create(1, 1, glm::vec3(0.5f, 0.5f, 1.0f));
        });
    }

    void CommandQueue::Execute()
    {
        if (m_CommandQueue.empty()) return;
        EnsureInitialized();

        auto GetFallbackTexture = [&](TextureType type) -> std::shared_ptr<ITexture>
        {
            switch (type)
            {
                case TextureType::BaseColorTexture:        return m_WhiteTexture;
                case TextureType::NormalTexture:           return m_NormalTexture;
                case TextureType::RoughnessTexture:        return m_GrayTexture;
                case TextureType::MetallicTexture:         return m_BlackTexture;
                case TextureType::AmbientOcclusionTexture: return m_WhiteTexture;
                case TextureType::EmissiveTexture:         return m_BlackTexture;
                case TextureType::OpacityTexture:          return m_WhiteTexture;
                case TextureType::ORMTexture:              return m_WhiteTexture;
                default:                                   return m_WhiteTexture;
            }
        };

        auto BindTexture = [&](const std::shared_ptr<IShader>& shader, const char* uniformName,
                               TextureType type, const std::weak_ptr<ITexture>& texture, std::int32_t slot)
        {
            if (auto tex = texture.lock())
            {
                tex->Bind(slot);
            }
            else
            {
                GetFallbackTexture(type)->Bind(slot);
            }
            
            shader->SetUniform(uniformName, slot);
        };

        auto GetShaderVariant = [&](const ResolvedMaterials& RM) -> std::shared_ptr<IShader>
        {
            ShaderFeatureMask SFM{ShaderFeatureMask::USE_NONE};
            if (RM.TMask & TexturesBitMask::HasBaseColor)   SFM |= ShaderFeatureMask::USE_BASECOLOR_MAP;
            if (RM.TMask & TexturesBitMask::HasNormal)      SFM |= ShaderFeatureMask::USE_NORMAL_MAP;
            if (RM.TMask & TexturesBitMask::HasRoughness)   SFM |= ShaderFeatureMask::USE_ROUGHNESS_MAP;
            if (RM.TMask & TexturesBitMask::HasMetallic)    SFM |= ShaderFeatureMask::USE_METALLIC_MAP;
            if (RM.TMask & TexturesBitMask::HasEmissive)    SFM |= ShaderFeatureMask::USE_EMISSIVE_MAP;
            if (RM.TMask & TexturesBitMask::HasOcclusion)   SFM |= ShaderFeatureMask::USE_OCCLUSION_MAP;
            if (RM.TMask & TexturesBitMask::HasORM)         SFM |= ShaderFeatureMask::USE_ORM_MAP;

            auto& SV    = ShaderVariant::GetInstance();
            auto shader = SV.GetVariant("Assets/Shaders/GLSL/PBR/ModularPBR.glsl", SFM);
            MOTION_ASSERT(shader, "Failed to get shader variant");
            return shader;
        };

        auto ApplyCamera = [](IShader* shader, const RenderCommand& cmd)
        {
            shader->SetUniform("uView",   cmd.CameraData.View);
            shader->SetUniform("uProj",   cmd.CameraData.Projection);
            shader->SetUniform("uCamPos", cmd.CameraData.CameraPosition);
        };

        auto ApplyModelData = [](IShader* shader, const RenderCommand& cmd)
        {
            shader->SetUniform("uModel",  cmd.ModelData.Model);
            shader->SetUniform("uNormal", cmd.ModelData.Normal);
        };

        auto ApplyLightData = [](IShader* shader, const RenderCommand& cmd)
        {
            shader->SetUniform("uLight.Direction", cmd.LightData.Direction);
            shader->SetUniform("uLight.Color",     cmd.LightData.Color);
            shader->SetUniform("uLight.Intensity", cmd.LightData.Intensity);
        };

        auto ApplyMaterials = [&](const std::shared_ptr<IShader>& shader, const ResolvedMaterials& RM)
        {
            shader->SetUniform("uMat.BaseColorFactor",  RM.BaseColorFactor);
            shader->SetUniform("uMat.RoughnessFactor",  RM.RoughnessFactor);
            shader->SetUniform("uMat.MetallicFactor",   RM.MetallicFactor);
            shader->SetUniform("uMat.EmissiveStrength", RM.EmissiveStrength);
            shader->SetUniform("uMat.EmissiveColor",    RM.EmissiveFactor);
            shader->SetUniform("uMat.AOFactor",         RM.AOStrength);
            shader->SetUniform("uMat.OpacityFactor",    RM.OpacityFactor);

            if (RM.TMask & TexturesBitMask::HasNormal)
                shader->SetUniform("uMat.NormalScale", RM.NormalScale);

            if (RM.TMask & TexturesBitMask::HasBaseColor) BindTexture(shader, "uBaseColorMap", TextureType::BaseColorTexture, RM.BaseColor, TextureSlot::BaseColor);
            if (RM.TMask & TexturesBitMask::HasNormal)    BindTexture(shader, "uNormalMap",    TextureType::NormalTexture,    RM.Normal,    TextureSlot::Normal);
            if (RM.TMask & TexturesBitMask::HasRoughness) BindTexture(shader, "uRoughnessMap", TextureType::RoughnessTexture, RM.Roughness, TextureSlot::Roughness);
            if (RM.TMask & TexturesBitMask::HasMetallic)  BindTexture(shader, "uMetallicMap",  TextureType::MetallicTexture,  RM.Metallic,  TextureSlot::Metallic);
            if (RM.TMask & TexturesBitMask::HasOcclusion) BindTexture(shader, "uOcclusionMap", TextureType::AmbientOcclusionTexture, RM.AO, TextureSlot::AO);
            if (RM.TMask & TexturesBitMask::HasEmissive)  BindTexture(shader, "uEmissiveMap",  TextureType::EmissiveTexture,  RM.Emissive,  TextureSlot::Emissive);
            if (RM.TMask & TexturesBitMask::HasOpacity)   BindTexture(shader, "uOpacityMap",   TextureType::OpacityTexture,   RM.Opacity,   TextureSlot::Opacity);
            if (RM.TMask & TexturesBitMask::HasORM)       BindTexture(shader, "uORM",          TextureType::ORMTexture,       RM.ORM,       TextureSlot::ORM);
        };

        auto IssueDrawIndexed = [](const RenderCommand& cmd)
        {
            if (!cmd.MeshPointer) return;
            cmd.MeshPointer->Bind();

            DrawIndexedArgs args{};
            args.indexCount = cmd.MeshPointer->GetIndicesCount();
            args.indexType  = IndexType::UInt32;
            args.topology   = PrimitiveTopology::Triangles;
            Renderer::DrawIndexed(args);
        };


        Sort(); 

        Material*     lastMaterial              = nullptr;
        Mesh*         lastMesh                  = nullptr;
        std::shared_ptr<IShader> lastShader     = nullptr;

        for (const auto& cmd : m_CommandQueue)
        {
            if (!cmd.MeshPointer || !cmd.MaterialPointer)
                continue;

            const bool materialChanged = (cmd.MaterialPointer != lastMaterial);

            std::shared_ptr<IShader> shader;
            if (materialChanged || !lastShader)
            {
                const ResolvedMaterials RM = GetResolvedMaterials(cmd.MaterialPointer);
                shader = GetShaderVariant(RM);
                shader->Bind();

                ApplyCamera(shader.get(), cmd);
                ApplyLightData(shader.get(), cmd);

                ApplyMaterials(shader, RM);
                lastShader   = shader;
                lastMaterial = cmd.MaterialPointer;
            }
            else
            {
                lastShader->Bind();
                ApplyCamera(lastShader.get(), cmd);
                ApplyLightData(lastShader.get(), cmd);
                shader = lastShader;
            }

            ApplyModelData(shader.get(), cmd);
            if (cmd.MeshPointer != lastMesh)
                lastMesh = cmd.MeshPointer;

            
                
            ConfigureRegular();
            IssueDrawIndexed(cmd);
        }
    }
}
