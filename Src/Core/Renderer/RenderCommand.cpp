#include "CorePCH.hpp"
#include "CommandQueue.hpp"
#include "Renderer.hpp"

namespace Motion
{
    void CommandQueue::Submit(const RenderCommand& c)
    {
        switch (c.Pass)
        {
            case RenderPass::Opaque:        m_Opaque.push_back(c);          break;
            case RenderPass::AlphaTest:     m_AlphaTest.push_back(c);       break;
            case RenderPass::Transparent:   m_Transparent.push_back(c);     break;
            case RenderPass::DepthOnly:     m_DepthOnly.push_back(c);       break;
            case RenderPass::Shadow:        m_Shadow.push_back(c);          break;
            case RenderPass::ForwardLit:    m_ForwardLit.push_back(c);      break;
            case RenderPass::PostProcess:   m_PostProcess.push_back(c);     break;
            case RenderPass::Overlay:       m_Overlay.push_back(c);         break;
            default: break;
        }
    }

    void CommandQueue::Sort()
    {
        auto frontToBack = [](const RenderCommand& a, const RenderCommand& b) { return a < b; };
        auto backToFront = [](const RenderCommand& a, const RenderCommand& b) { return a > b; };

        std::sort(m_Opaque.begin(),      m_Opaque.end(),      frontToBack);
        std::sort(m_AlphaTest.begin(),   m_AlphaTest.end(),   frontToBack);
        std::sort(m_Transparent.begin(), m_Transparent.end(), backToFront);

        // The rest can stay as-is or frontToBack if your SortKey encodes distance
        // std::sort(m_DepthOnly.begin(),  m_DepthOnly.end(),  frontToBack);
        // std::sort(m_Shadow.begin(),     m_Shadow.end(),     frontToBack);
        // std::sort(m_ForwardLit.begin(), m_ForwardLit.end(), frontToBack);
    }

    void CommandQueue::EnsureInitialized()
    {
        if(!s_ModelUBO)    s_ModelUBO    = IUniformBuffer::Create(sizeof(ModelMatrix),        ShaderVariant::ObjectUboBinding);
        if(!s_CameraUBO)   s_CameraUBO   = IUniformBuffer::Create(sizeof(CameraViewProjection),ShaderVariant::CameraUboBinding);
        if(!s_LightUBO)    s_LightUBO    = IUniformBuffer::Create(sizeof(SunLighting),        ShaderVariant::LightUboBinding);
        if(!s_MaterialUBO) s_MaterialUBO = IUniformBuffer::Create(sizeof(MaterialAttributes), ShaderVariant::MaterialUboBinding);

        if(!s_WhiteTexture)  s_WhiteTexture  = ITexture::Create(1, 1, glm::vec3(1.0f, 1.0f, 1.0f));
        if(!s_BlackTexture)  s_BlackTexture  = ITexture::Create(1, 1, glm::vec3(0.0f, 0.0f, 0.0f));
        if(!s_GrayTexture)   s_GrayTexture   = ITexture::Create(1, 1, glm::vec3(0.5f, 0.5f, 0.5f));
        if(!s_NormalTexture) s_NormalTexture = ITexture::Create(1, 1, glm::vec3(0.5f, 0.5f, 1.0f));
    }

    void CommandQueue::ApplyStage(const RenderPass& pass, const RenderFlags& flags)
    {
        auto state = s_RenderingStage->Snapshot();

        state.DepthTest  = !(flags & RenderFlags::DepthTestOff);
        state.DepthWrite = !(flags & RenderFlags::DepthWriteOff);
        state.DepthFunc  = DepthFunction::LessEqual;

        state.CullEnabled = !(flags & RenderFlags::DoubleSided);
        state.CullingMode = CullMode::Back;
        state.FrontFace   = FrontFace::CCW;
        state.Wireframe   = ((flags & RenderFlags::Wireframe) != static_cast<std::uint32_t>(RenderFlags::None));

        switch (pass)
        {
            case RenderPass::Opaque:
            case RenderPass::AlphaTest:
                state.DepthWrite   = true;
                state.BlendEnabled = false;
                break;

            case RenderPass::Transparent:
                state.DepthWrite   = false;
                state.BlendEnabled = true;
                if (flags & RenderFlags::Premultiplied)
                {
                    state.SrcRGB = BlendFactor::One;
                    state.DstRGB = BlendFactor::OneMinusSrcAlpha;
                    state.SrcA   = BlendFactor::One;
                    state.DstA   = BlendFactor::OneMinusSrcAlpha;
                }
                else if (flags & RenderFlags::Additive)
                {
                    state.SrcRGB = BlendFactor::One;
                    state.DstRGB = BlendFactor::One;
                    state.SrcA   = BlendFactor::One;
                    state.DstA   = BlendFactor::One;
                }
                else
                {
                    state.SrcRGB = BlendFactor::SrcAlpha;
                    state.DstRGB = BlendFactor::OneMinusSrcAlpha;
                    state.SrcA   = BlendFactor::One;
                    state.DstA   = BlendFactor::OneMinusSrcAlpha;
                }
                break;

            case RenderPass::DepthOnly:
            case RenderPass::Shadow:
                state.ColorMaskR = state.ColorMaskG = state.ColorMaskB = state.ColorMaskA = false;
                state.DepthWrite = true;
                state.BlendEnabled = false;
                break;

            case RenderPass::ForwardLit:
                state.DepthWrite   = true;
                state.BlendEnabled = false;
                break;

            case RenderPass::PostProcess:
                state.DepthTest    = false;
                state.DepthWrite   = false;
                state.BlendEnabled = true;
                break;

            case RenderPass::Overlay:
                state.DepthTest    = false;
                state.DepthWrite   = false;
                state.BlendEnabled = true;
                state.SrcRGB = BlendFactor::SrcAlpha;
                state.DstRGB = BlendFactor::OneMinusSrcAlpha;
                state.SrcA   = BlendFactor::One;
                state.DstA   = BlendFactor::OneMinusSrcAlpha;
                break;
        }

        s_RenderingStage->Apply(state);
    }

    void CommandQueue::Execute()
    {
        EnsureInitialized();

        auto GetFallbackTexture = [&](TextureType type) -> std::shared_ptr<ITexture>
        {
            switch (type)
            {
                case TextureType::BaseColorTexture:        return s_WhiteTexture;
                case TextureType::NormalTexture:           return s_NormalTexture;
                case TextureType::RoughnessTexture:        return s_GrayTexture;
                case TextureType::MetallicTexture:         return s_BlackTexture;
                case TextureType::AmbientOcclusionTexture: return s_WhiteTexture;
                case TextureType::EmissiveTexture:         return s_BlackTexture;
                case TextureType::OpacityTexture:          return s_WhiteTexture;
                case TextureType::DisplacementTexture:     return s_BlackTexture;
                case TextureType::ORMTexture:              return s_WhiteTexture;
                default:                                   return s_WhiteTexture;
            }
        };

        auto BindTexture = [&](TextureType type, ITexture* texture, std::int32_t slot) -> void
        {
            if(texture) texture->Bind(slot);
            else        GetFallbackTexture(type)->Bind(slot);
        };

        auto GetShaderVariant = [&](const ResolvedMaterials& RM, const EnvironmentSpecification& envSpec) -> std::shared_ptr<IShader>
        {
            ShaderFeatureMask SFM;

            if (envSpec.UseSHDiffuse)                         SFM |= ShaderFeatureMask::USE_SH9;
            if (RM.TMask & TexturesBitMask::HasORM)           SFM |= ShaderFeatureMask::USE_ORM_MAP;
            if (RM.TMask & TexturesBitMask::HasOpacity)       SFM |= ShaderFeatureMask::USE_OPACITY_MAP;
            if (RM.Mode == AlphaMode::Opaque)                 SFM |= ShaderFeatureMask::USE_ALPHA_MODE_OPAQUE;
            if (RM.Mode == AlphaMode::Mask)                   SFM |= ShaderFeatureMask::USE_ALPHA_MODE_MASK;
            if (RM.Mode == AlphaMode::Blend)
            {
                SFM |= ShaderFeatureMask::USE_ALPHA_MODE_BLEND;
                if (RM.BlendMode == AlphaBlendMode::Premultiplied)  SFM |= ShaderFeatureMask::USE_ALPHA_BLEND_PREMULTIPLIED;
                if (RM.BlendMode == AlphaBlendMode::Additive)       SFM |= ShaderFeatureMask::USE_ALPHA_BLEND_ADDITIVE;
                if (RM.BlendMode == AlphaBlendMode::AlphaBlend)     SFM |= ShaderFeatureMask::USE_ALPHA_BLEND;
            }

            auto& SV = ShaderVariant::GetInstance();
            auto                    shaderVariant = SV.GetVariant("Assets/Shaders/GLSL/PBR/StandardPBR.glsl", SFM);
            if (RM.UseDisplacement) shaderVariant = SV.GetVariant("Assets/Shaders/GLSL/PBR/StandardPBR_Displacement.glsl", SFM);

            return shaderVariant;
        };

        auto ApplyCamera = [&](const RenderCommand& cmd) -> void
        {
            s_CameraUBO->Bind();
            s_CameraData = cmd.CameraData;
            s_CameraUBO->Orphan();
            s_CameraUBO->SetRawBufferData(sizeof(CameraViewProjection), &s_CameraData);
        };

        auto ApplyModelData = [&](const RenderCommand& cmd) -> void
        {
            s_ModelUBO->Bind();
            s_ModelData = cmd.ModelData;
            s_ModelUBO->Orphan();
            s_ModelUBO->SetRawBufferData(sizeof(ModelMatrix), &s_ModelData);
        };

        auto ApplyLightData = [&](const RenderCommand& cmd) -> void
        {
            s_LightUBO->Bind();
            s_LightData = cmd.SunData;
            s_LightUBO->Orphan();
            s_LightUBO->SetRawBufferData(sizeof(SunLighting), &s_LightData);
        };

        auto ApplyEnvironmentTextures = [&](IEnvironment* env) -> void
        {
            IBLTextureBinding IBL;
            IBL.SlotBRDFLUT     = TextureSlot::BRDFLUT;
            IBL.SlotIrradiance  = TextureSlot::Irradiance;
            IBL.SlotPrefiltered = TextureSlot::Prefilter;
            env->BindIBL(IBL);
        };

        auto ApplyMaterials = [&](const ResolvedMaterials& RM) -> void
        {
            s_MaterialData.uBaseColorFactor   = RM.BaseColorFactor;
            s_MaterialData.uNormalScale       = RM.NormalScale;
            s_MaterialData.uRoughnessFactor   = RM.RoughnessFactor;
            s_MaterialData.uMetallicFactor    = RM.MetallicFactor;
            s_MaterialData.uAOFactor          = RM.AOStrength;
            s_MaterialData.uEmissiveStrength  = RM.EmissiveStrength;
            s_MaterialData.uEmissiveColor     = RM.EmissiveFactor;
            s_MaterialData.uSpecularStrength  = RM.SpecularStrength;
            s_MaterialData.uOpacityFactor     = RM.OpacityFactor;
            s_MaterialData.uAlphaCutOff       = RM.AlphaCutoff;

            s_MaterialData.uDisplacementScale = RM.DispScale;
            s_MaterialData.uDisplacementBias  = RM.DispBias;
            s_MaterialData.uTessMin           = RM.AdvDisplacement.TessellationMin;
            s_MaterialData.uTessMax           = RM.AdvDisplacement.TessellationMax;
            s_MaterialData.uPixPerEdge        = RM.AdvDisplacement.PixelsPerEdge;
            s_MaterialData.uLODNear           = RM.AdvDisplacement.LODNear;
            s_MaterialData.uLODFar            = RM.AdvDisplacement.LODFar;

            // Samplers (with fallbacks)
            BindTexture(TextureType::BaseColorTexture,        RM.BaseColor,   TextureSlot::BaseColor);
            BindTexture(TextureType::NormalTexture,           RM.Normal,      TextureSlot::Normal);
            BindTexture(TextureType::RoughnessTexture,        RM.Roughness,   TextureSlot::Roughness);
            BindTexture(TextureType::MetallicTexture,         RM.Metallic,    TextureSlot::Metallic);
            BindTexture(TextureType::AmbientOcclusionTexture, RM.AO,          TextureSlot::AO);
            BindTexture(TextureType::EmissiveTexture,         RM.Emissive,    TextureSlot::Emissive);
            BindTexture(TextureType::OpacityTexture,          RM.Opacity,     TextureSlot::Opacity);

            if (RM.UseDisplacement)
                BindTexture(TextureType::DisplacementTexture, RM.Displacement, TextureSlot::Displacement);
            if (RM.UseORMTextures)
                BindTexture(TextureType::ORMTexture,          RM.ORM,          TextureSlot::ORM);

            s_MaterialUBO->Bind();
            s_MaterialUBO->Orphan();
            s_MaterialUBO->SetRawBufferData(sizeof(MaterialAttributes), &s_MaterialData);
        };

        auto IssueDrawIndexed = [&](const RenderCommand& cmd)
        {
            const bool tess = static_cast<bool>(cmd.Flags & RenderFlags::Tessellation);

            DrawIndexedArgs args{};
            args.topology = tess ? PrimitiveTopology::Patches : PrimitiveTopology::Triangles;
            args.patchControlPoints = tess ? std::min(3, Renderer::Caps().maxPatchVertices) : 3;

            if (cmd.MeshPointer)
            {
                cmd.MeshPointer->Bind();
                args.indexCount = cmd.MeshPointer->GetIndicesCount();
                args.indexType  = IndexType::UInt32;
            }

            args.instanceCount = 1;
            args.firstIndex    = 0;
            args.baseVertex    = 0;
            args.baseInstance  = 0;

            Renderer::DrawIndexed(args);
        };

        auto DrawOnce = [&](const RenderCommand& cmd)
        {
            if (!cmd.MeshPointer || !cmd.MaterialPointer || !cmd.EnvPointer)
                return;

            ApplyStage(cmd.Pass, cmd.Flags);

            ResolvedMaterials RM = GetResolvedMaterials(cmd.MaterialPointer);
            std::shared_ptr<IShader> shader = GetShaderVariant(RM, cmd.EnvPointer->GetSpecification());
            shader->Bind();

            ApplyCamera(cmd);
            ApplyModelData(cmd);
            ApplyLightData(cmd);
            ApplyMaterials(RM);
            ApplyEnvironmentTextures(cmd.EnvPointer);

            IssueDrawIndexed(cmd);
        };

        Sort();

        auto DrawPass = [&](std::vector<RenderCommand>& bucket)
        {
            for (const auto& cmd : bucket) DrawOnce(cmd);
        };

        DrawPass(m_DepthOnly);
        DrawPass(m_Shadow);
        DrawPass(m_Opaque);
        DrawPass(m_AlphaTest);
        DrawPass(m_ForwardLit);
        DrawPass(m_Transparent);
        DrawPass(m_PostProcess);
        DrawPass(m_Overlay);
    }

    void CommandQueue::Clear()
    {
        m_Opaque.clear();
        m_AlphaTest.clear();
        m_Transparent.clear();
        m_DepthOnly.clear();
        m_Shadow.clear();
        m_ForwardLit.clear();
        m_PostProcess.clear();
        m_Overlay.clear();
    }
}
