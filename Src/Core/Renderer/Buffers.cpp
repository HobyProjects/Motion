#include "CorePCH.hpp"

namespace Motion
{
    /**
     * @brief Creates a vertex buffer of the specified allocation size using the current rendering API.
     *
     * This function selects the appropriate vertex buffer creation method based on the active rendering API.
     * If the API is not implemented, an assertion will be triggered and nullptr will be returned.
     *
     * @param allocatorSize The size (in bytes) to allocate for the vertex buffer.
     * @return std::shared_ptr<IVertexBuffer> A shared pointer to the created vertex buffer, or nullptr if the API is not implemented.
     */
    std::shared_ptr<IVertexBuffer> BufferFactory::CreateVertexBuffer(std::uint32_t allocatorSize)
    {
        switch (Renderer::GetAPI())
        {
        case RenderingAPI::OpenGL:         return GL_CreateVertexBuffer(allocatorSize);
        case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!"); return nullptr;
        case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); return nullptr;
        default:                           MOTION_ASSERT(false, "Unknown rendering API!"); return nullptr;
        };
    }

    /**
     * @brief Creates a vertex buffer using the specified rendering API.
     *
     * This function creates and returns a shared pointer to an IVertexBuffer object,
     * initialized with the provided vertex data and size. The actual implementation
     * depends on the currently selected rendering API (e.g., OpenGL, Vulkan, DirectX).
     *
     * @param data Pointer to the array of vertex data (float values).
     * @param size The size of the vertex data array in bytes.
     * @return std::shared_ptr<IVertexBuffer> A shared pointer to the created vertex buffer,
     *         or nullptr if the rendering API is not implemented or unknown.
     *
     * @note Currently, only the OpenGL API is implemented. Other APIs will trigger an assertion failure.
     */
    std::shared_ptr<IVertexBuffer> BufferFactory::CreateVertexBuffer(float* data, std::uint32_t size)
    {
        switch (Renderer::GetAPI())
        {
        case RenderingAPI::OpenGL:         return GL_CreateVertexBuffer(data, size);
        case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!"); return nullptr;
        case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); return nullptr;
        default:                           MOTION_ASSERT(false, "Unknown rendering API!"); return nullptr;
        };
    }

    /**
     * @brief Creates an element buffer using the specified rendering API.
     *
     * This factory method creates and returns a shared pointer to an IElementBuffer
     * implementation, depending on the currently selected rendering API.
     *
     * @param data Pointer to the array of element indices.
     * @param size The number of elements in the buffer.
     * @return std::shared_ptr<IElementBuffer> A shared pointer to the created element buffer,
     *         or nullptr if the rendering API is not implemented or unknown.
     *
     * @note Currently, only the OpenGL API is implemented. Vulkan and DirectX will trigger assertions.
     */
    std::shared_ptr<IElementBuffer> BufferFactory::CreateElementBuffer(std::uint32_t* data, std::uint32_t size)
    {
        switch (Renderer::GetAPI())
        {
        case RenderingAPI::OpenGL:         return GL_CreateElementBuffer(data, size);
        case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!"); return nullptr;
        case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); return nullptr;
        default:                           MOTION_ASSERT(false, "Unknown rendering API!"); return nullptr;
        };
    }

    /**
     * @brief Creates a shader buffer of the specified size and binding point for the current rendering API.
     *
     * This factory method instantiates a shader buffer compatible with the active rendering backend.
     * If the rendering API is not implemented, an assertion is triggered and nullptr is returned.
     *
     * @param size The size, in bytes, of the shader buffer to create.
     * @param binding The binding point to which the buffer will be bound in the shader.
     * @return std::shared_ptr<IShaderBuffer> A shared pointer to the created shader buffer, or nullptr if not implemented.
     */
    std::shared_ptr<IShaderBuffer> BufferFactory::CreateShaderBuffer(std::uint32_t size, BindingPoint binding)
    {
        switch (Renderer::GetAPI())
        {
        case RenderingAPI::OpenGL:         return GL_CreateShaderBuffer(size, binding);
        case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!"); return nullptr;
        case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); return nullptr;
        default:                           MOTION_ASSERT(false, "Unknown rendering API!"); return nullptr;
        };
    }

    /**
     * @brief Creates a uniform buffer object for the specified rendering API.
     *
     * This function creates and returns a shared pointer to an IUniformBuffer instance,
     * configured with the given size and binding point. The actual implementation depends
     * on the currently selected rendering API (e.g., OpenGL, Vulkan, DirectX).
     *
     * @param size The size (in bytes) of the uniform buffer to create.
     * @param binding The binding point to which the uniform buffer will be bound.
     * @return std::shared_ptr<IUniformBuffer> A shared pointer to the created uniform buffer,
     *         or nullptr if the rendering API is not implemented or unknown.
     *
     * @note Currently, only the OpenGL implementation is available. Other APIs will trigger
     *       an assertion and return nullptr.
     */
    std::shared_ptr<IUniformBuffer> BufferFactory::CreateUniformBuffer(std::uint32_t size, BindingPoint binding)
    {
        switch (Renderer::GetAPI())
        {
        case RenderingAPI::OpenGL:         return GL_CreateUniformBuffer(size, binding);
        case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!"); return nullptr;
        case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); return nullptr;
        default:                           MOTION_ASSERT(false, "Unknown rendering API!"); return nullptr;
        };
    }

    /**
     * @brief Creates a frame buffer object based on the specified rendering API.
     *
     * This function instantiates a frame buffer according to the provided
     * FrameBufferSpecification and the currently active rendering API.
     * Supported APIs are OpenGL (implemented), while Vulkan and DirectX are
     * not yet implemented and will trigger assertions.
     *
     * @param specification The specification describing the properties of the frame buffer to create.
     * @return std::shared_ptr<IFrameBuffer> A shared pointer to the created frame buffer object,
     *         or nullptr if the API is not implemented or unknown.
     */
    std::shared_ptr<IFrameBuffer> BufferFactory::CreateFrameBuffer(const FrameBufferSpecification& specification)
    {
        switch (Renderer::GetAPI())
        {
        case RenderingAPI::OpenGL:         return GL_CreateFrameBuffer(specification);
        case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!"); return nullptr;
        case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); return nullptr;
        default:                           MOTION_ASSERT(false, "Unknown rendering API!"); return nullptr;
        };
    }


}
