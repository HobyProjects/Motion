#pragma once

#include "Arrays.hpp"

namespace Motion
{
    class GL_VertexArray final : public IVertexArray
    {
    public:
        GL_VertexArray();
        virtual ~GL_VertexArray() override;

        virtual void Bind() const override;
        virtual void Unbind() const override;

        virtual RendererID GetID() const override;
        virtual void EmplaceVertexBuffer(const std::shared_ptr<IVertexBuffer>& vtxBuffer) override;
        virtual void EmplaceIndexBuffer(const std::shared_ptr<IElementBuffer>& idxBuffer) override;
        virtual std::vector<std::shared_ptr<IVertexBuffer>>& GetVertexBuffer() override;
        virtual std::shared_ptr<IElementBuffer>& GetElementBuffer() override;

        static std::shared_ptr<GL_VertexArray> Create();

    private:
        RendererID m_RendererID{ 0 };
        std::vector<std::shared_ptr<IVertexBuffer>> m_VertexBuffers{};
        std::shared_ptr<IElementBuffer> m_IndexBuffer{};
    };

}