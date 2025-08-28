#pragma once

#include <vector>
#include "RenderCommand.hpp"
namespace Motion
{
    class CommandQueue
    {
        public:
            CommandQueue() = default;
            ~CommandQueue() = default;

            void Submit(const RenderCommand& command);
            void Execute();
            void Clear();
            void Sort();

        private:
            void EnsureInitialized();
            void ApplyStage(const RenderPass& pass, const RenderFlags& flags);

        private:
            inline static std::shared_ptr<IUniformBuffer>      s_CameraUBO{nullptr};
            inline static std::shared_ptr<IUniformBuffer>      s_LightUBO{nullptr};
            inline static std::shared_ptr<IUniformBuffer>      s_MaterialUBO{nullptr};
            inline static std::shared_ptr<IUniformBuffer>      s_ModelUBO{nullptr};
            inline static std::shared_ptr<IRenderingStage>     s_RenderingStage{nullptr};

            inline static CameraViewProjection     s_CameraData{};
            inline static SunLighting              s_LightData{};
            inline static MaterialAttributes       s_MaterialData{};
            inline static ModelMatrix              s_ModelData{};

            inline static std::shared_ptr<ITexture> s_GrayTexture{nullptr};
            inline static std::shared_ptr<ITexture> s_WhiteTexture{nullptr};
            inline static std::shared_ptr<ITexture> s_BlackTexture{nullptr};
            inline static std::shared_ptr<ITexture> s_NormalTexture{nullptr};

        private:
            std::vector<RenderCommand> m_Opaque;
            std::vector<RenderCommand> m_AlphaTest;
            std::vector<RenderCommand> m_Transparent;
            std::vector<RenderCommand> m_DepthOnly;
            std::vector<RenderCommand> m_Shadow;
            std::vector<RenderCommand> m_ForwardLit;
            std::vector<RenderCommand> m_PostProcess;
            std::vector<RenderCommand> m_Overlay;
    };
}