#pragma once

#include "Asset.hpp"
#include "Shaders.hpp"
#include "Mesh.hpp"
#include "Buffers.hpp"

namespace Motion
{
    class PostProcessor
    {
    public:
        PostProcessor(const FrameBufferSpecification& spec);
        ~PostProcessor() = default;

        void Process(FrameTextureID inputTextureID);
        FrameTextureID GetOutputTextureID() const;
        void OnResize(std::uint32_t width, std::uint32_t height);

    private:
        std::shared_ptr<IFrameBuffer> m_FrameBuffer{ nullptr };
        std::shared_ptr<IShader> m_Shader{ nullptr };
        std::shared_ptr<Mesh> m_ScreenQuad{ nullptr };
    };
}
