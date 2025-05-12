#pragma once

#include <memory>
#include "Buffers.hpp"

namespace Motion::Core
{
    using RendererID = uint32_t;

    class IVertexArray
    {
        public:
            IVertexArray() = default;
            virtual ~IVertexArray() = default;

            virtual void Bind() const = 0;
            virtual void Unbind() const = 0;

            virtual RendererID GetID() const = 0;
            virtual void EmplaceVertexBuffer(const std::shared_ptr<IVertexBuffer>& vtxBuffer) = 0;
            virtual void EmplaceIndexBuffer(const std::shared_ptr<IElementBuffer>& idxBuffer) = 0;
            virtual std::vector<std::shared_ptr<IVertexBuffer>>& GetVertexBuffer() = 0;
            virtual std::shared_ptr<IElementBuffer>& GetElementBuffer() = 0;
    };

    class ArrayBuilder
    {
        private:
            ArrayBuilder() = default;
            ~ArrayBuilder() = default;

            ArrayBuilder(const ArrayBuilder&) = delete;
            ArrayBuilder& operator=(const ArrayBuilder&) = delete;
            ArrayBuilder(ArrayBuilder&&) = delete;
            ArrayBuilder& operator=(ArrayBuilder&&) = delete;

        public:
            static std::shared_ptr<IVertexArray> CreateVertexArray();
    };
}