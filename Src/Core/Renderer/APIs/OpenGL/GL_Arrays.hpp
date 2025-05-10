#pragma once

#include "Arrays.hpp"

namespace Motion::Core
{
    class GLVertexArray : public IVertexArray
    {
        public:
            GLVertexArray();
            virtual ~GLVertexArray() override;

            virtual void Bind() const override;
            virtual void Unbind() const override;

            virtual RendererID GetID() const override;
            virtual void EmplaceVertexBuffer(const std::shared_ptr<IVertexBuffer>& vtxBuffer) override;
            virtual void EmplaceIndexBuffer(const std::shared_ptr<IElementBuffer>& idxBuffer) override;
            virtual std::vector<std::shared_ptr<IVertexBuffer>>& GetVertexBuffer() override;
            virtual std::shared_ptr<IElementBuffer>& GetElementBuffer() override;

        private:
            RendererID m_RendererID{0};
            std::vector<std::shared_ptr<IVertexBuffer>> m_VertexBuffers{};
            std::shared_ptr<IElementBuffer> m_IndexBuffer{};
    };

    std::shared_ptr<GLVertexArray> GL_CreateVertexArray();
}