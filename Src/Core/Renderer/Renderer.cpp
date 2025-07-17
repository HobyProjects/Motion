#include "CorePCH.hpp"
#include "Renderer.hpp"

namespace Motion::Core
{
#ifdef MOTION_PLATFORM_WINDOWS
    // This should be DirectX but for now we are using OpenGL
    static RenderingAPI s_RenderingAPI = RenderingAPI::OpenGL;
#elif defined(MOTION_PLATFORM_LINUX)
    // This should be Vulkan but for now we are using OpenGL
    static RenderingAPI s_RenderingAPI = RenderingAPI::OpenGL;
#else
#error "Unknown platform!"
#endif

    static DrawCommandQueue s_CommandQueue;
    static std::thread s_RenderThread;
    static std::condition_variable s_RenderCV;
    static std::mutex s_RenderMutex;
    static bool s_FrameReady = false;
    static bool s_Running = true;
    static std::uint32_t s_DrawCalls = 0;


    /**
     * @brief Initializes the Renderer subsystem.
     *
     * This function sets up the rendering backend based on the selected rendering API.
     * It initializes the appropriate renderer (e.g., OpenGL) and starts the render thread,
     * which waits for frame readiness, consumes queued render commands, and processes them.
     *
     * @note Currently, only OpenGL is implemented. Vulkan and DirectX will trigger assertions.
     * @note The render thread runs in the background and processes commands when a new frame is ready.
     *
     * @throws Assertion failure if an unsupported or unknown rendering API is selected.
     */
    void Renderer::Init()
    {
        switch (s_RenderingAPI)
        {
        case RenderingAPI::OpenGL:
            GL_Renderer::Init();
            break;
        case RenderingAPI::Vulkan:
            MOTION_ASSERT(false, "Vulkan is not implemented yet!");
            break;
        case RenderingAPI::DirectX:
            MOTION_ASSERT(false, "DirectX is not implemented yet!");
            break;
        default:
            MOTION_ASSERT(false, "Unknown rendering API!");
            break;
        }


        s_RenderThread = std::thread([this] {
            while (s_Running) {
                std::unique_lock lock(s_RenderMutex);
                s_RenderCV.wait(lock, [] { return s_FrameReady; });

                auto& commands = s_CommandQueue.Consume();
                if (!commands.empty())
                {
                    std::sort(commands.begin(), commands.end(), [](const DrawCommand& a, const DrawCommand& b) { return a < b; });
                    for (const auto& command : commands)
                    {
                        Flush(command);
                        s_DrawCalls++;
                    }
                }

                s_FrameReady = false;
            }
            });
    }

    /**
     * @brief Shuts down the renderer and cleans up resources.
     *
     * This function stops the rendering thread, notifies any waiting threads,
     * and joins the rendering thread if it is still running. It then performs
     * cleanup specific to the currently selected rendering API.
     *
     * For OpenGL, it calls the appropriate cleanup routine. For Vulkan and DirectX,
     * this function asserts as those APIs are not yet implemented. If an unknown
     * rendering API is selected, an assertion is triggered.
     */
    void Renderer::Quit()
    {
        s_Running = false;
        s_RenderCV.notify_one();
        if (s_RenderThread.joinable())
            s_RenderThread.join();

        switch (s_RenderingAPI)
        {
        case RenderingAPI::OpenGL:
            GL_Renderer::Quit();
            break;
        case RenderingAPI::Vulkan:
            MOTION_ASSERT(false, "Vulkan is not implemented yet!");
            break;
        case RenderingAPI::DirectX:
            MOTION_ASSERT(false, "DirectX is not implemented yet!");
            break;
        default:
            MOTION_ASSERT(false, "Unknown rendering API!");
            break;
        }
    }

    /**
     * @brief Retrieves the current rendering API in use.
     *
     * @return The currently selected RenderingAPI.
     */
    RenderingAPI Renderer::GetAPI()
    {
        return s_RenderingAPI;
    }

    /**
     * @brief Clears the current rendering target using the selected rendering API.
     *
     * This function dispatches the clear operation to the appropriate rendering backend
     * based on the value of s_RenderingAPI. Currently, only OpenGL is implemented.
     * For Vulkan and DirectX, the function will trigger an assertion as they are not yet implemented.
     *
     * @note If an unknown rendering API is selected, an assertion will be triggered.
     */
    void Renderer::Clear()
    {
        switch (s_RenderingAPI)
        {
        case RenderingAPI::OpenGL:
            GL_Renderer::Clear();
            break;
        case RenderingAPI::Vulkan:
            MOTION_ASSERT(false, "Vulkan is not implemented yet!");
            break;
        case RenderingAPI::DirectX:
            MOTION_ASSERT(false, "DirectX is not implemented yet!");
            break;
        default:
            MOTION_ASSERT(false, "Unknown rendering API!");
            break;
        }
    }

    /**
     * @brief Sets the clear color for the current rendering context.
     *
     * This function sets the color used to clear the rendering target (e.g., the screen or framebuffer)
     * based on the currently selected rendering API. If the rendering API is not implemented,
     * an assertion will be triggered.
     *
     * @param color The color to use when clearing, represented as a glm::vec4 (RGBA).
     */
    void Renderer::ClearColor(const glm::vec4& color)
    {
        switch (s_RenderingAPI)
        {
        case RenderingAPI::OpenGL:
            GL_Renderer::ClearColor(color);
            break;
        case RenderingAPI::Vulkan:
            MOTION_ASSERT(false, "Vulkan is not implemented yet!");
            break;
        case RenderingAPI::DirectX:
            MOTION_ASSERT(false, "DirectX is not implemented yet!");
            break;
        default:
            MOTION_ASSERT(false, "Unknown rendering API!");
            break;
        }
    }

    /**
     * @brief Sets the viewport for rendering.
     *
     * Configures the rendering viewport to the specified position and size.
     * The implementation depends on the currently selected rendering API.
     *
     * @param x The x-coordinate of the lower left corner of the viewport.
     * @param y The y-coordinate of the lower left corner of the viewport.
     * @param width The width of the viewport.
     * @param height The height of the viewport.
     */
    void Renderer::SetViewport(int32_t x, int32_t y, int32_t width, int32_t height)
    {
        switch (s_RenderingAPI)
        {
        case RenderingAPI::OpenGL:
            GL_Renderer::SetViewport(x, y, width, height);
            break;
        case RenderingAPI::Vulkan:
            MOTION_ASSERT(false, "Vulkan is not implemented yet!");
            break;
        case RenderingAPI::DirectX:
            MOTION_ASSERT(false, "DirectX is not implemented yet!");
            break;
        default:
            MOTION_ASSERT(false, "Unknown rendering API!");
            break;
        }
    }

    /**
     * @brief Draws indexed geometry using the currently selected rendering API.
     *
     * This function dispatches the indexed draw call to the appropriate rendering backend
     * (e.g., OpenGL, Vulkan, DirectX) based on the value of s_RenderingAPI. If the selected
     * API is not implemented, an assertion will be triggered.
     *
     * @param indicesCount The number of indices to draw.
     */
    void Renderer::DrawIndexed(uint32_t indicesCount)
    {
        switch (s_RenderingAPI)
        {
        case RenderingAPI::OpenGL:
            GL_Renderer::DrawIndexed(indicesCount);
            break;
        case RenderingAPI::Vulkan:
            MOTION_ASSERT(false, "Vulkan is not implemented yet!");
            break;
        case RenderingAPI::DirectX:
            MOTION_ASSERT(false, "DirectX is not implemented yet!");
            break;
        default:
            MOTION_ASSERT(false, "Unknown rendering API!");
            break;
        }
    }

    /**
     * @brief Submits a draw command to the renderer's command queue.
     *
     * This function enqueues the provided draw command so that it can be executed
     * during the rendering process. The draw command encapsulates all necessary
     * information required to render a specific object or set of objects.
     *
     * @param drawCommand A shared pointer to the DrawCommand to be submitted.
     */
    void Renderer::Submit(const DrawCommand& drawCommand)
    {
        s_CommandQueue.Submit(drawCommand);
    }


    void Renderer::Submit(const FrameDrawCommand& frameDrawCommand)
    {
    }

    /**
     * @brief Prepares the renderer for a new frame.
     *
     * Resets the draw call counter to zero at the beginning of each frame.
     * This should be called before any rendering operations for the current frame.
     */
    void Renderer::BeginFrame()
    {
        s_DrawCalls = 0;
    }

    /**
     * @brief Signals the end of the current rendering frame.
     *
     * This function performs the following actions:
     * - Swaps the draw buffers to present the rendered frame.
     * - Sets the frame ready flag in a thread-safe manner.
     * - Notifies one waiting thread that the frame is ready for further processing.
     *
     * Thread safety is ensured using a mutex lock when updating the frame ready flag.
     */
    void Renderer::EndFrame()
    {



        s_CommandQueue.SwapBuffers(); // Flip draw buffers
        {
            std::lock_guard lock(s_RenderMutex);
            s_FrameReady = true;
        }
        s_RenderCV.notify_one();
    }

    /**
     * @brief Executes the rendering of a draw command by binding the appropriate shader, material, and mesh.
     *
     * This function retrieves the material associated with the given draw command and selects the appropriate shader
     * based on the material's shading method. It then binds the shader, sets the necessary uniform variables (such as
     * model and view-projection matrices), binds the material and mesh, and issues a draw call. If the mesh or shader
     * is not properly initialized, an error is logged.
     *
     * @param drawCommand The draw command containing references to the material, mesh, and transformation matrices to be used for rendering.
     */
    void Renderer::Flush(const DrawCommand& drawCommand)
    {
        std::shared_ptr<Material> material = AssetManager::GetInstance().Get<Material>(drawCommand.MaterialID);
        std::shared_ptr<IShader> shader{ nullptr };

        switch (material->GetShadingMethod())
        {
        case MaterialShadingMethod::Auto:
            shader = AssetManager::GetInstance().Get<IShader>("DefaultShader");
            break;
        case MaterialShadingMethod::Phong:
            shader = AssetManager::GetInstance().Get<IShader>("PhongShader");
            break;
        case MaterialShadingMethod::PBR:
            shader = AssetManager::GetInstance().Get<IShader>("PBRShader");
            break;
        case MaterialShadingMethod::Unlit:
            shader = AssetManager::GetInstance().Get<IShader>("UnlitShader");
            break;
        default:
            MOTION_CORE_ERROR("Unknown shading method for material: {0}", material->GetName());
            shader = AssetManager::GetInstance().Get<IShader>("DefaultShader");
            break;
        };

        if (shader->IsAssetInitialized())
        {
            shader->Bind();
            shader->SetUniform(UniformCache::GlobalAttri_ModelMatrix, drawCommand.ModelMatrix);
            shader->SetUniform(UniformCache::GlobalAttri_ViewProjMatrix, drawCommand.ViewProjectionMatrix);

            material->Bind(shader);
            std::shared_ptr<Mesh> mesh = AssetManager::GetInstance().Get<Mesh>(drawCommand.MeshID);
            if (mesh->IsAssetInitialized())
            {
                mesh->Bind();
                DrawIndexed(mesh->GetIndicesCount());
                mesh->Unbind();
            }
            else
            {
                MOTION_CORE_ERROR("Mesh with ID {0} is not initialized or does not exist.", drawCommand.MeshID);
            }
            material->Unbind();
            shader->Unbind();
        }
    }

    uint32_t Renderer::GetDrawCalls()
    {
        return s_DrawCalls;
    }

    /**
     * @brief Retrieves the singleton instance of the Renderer.
     *
     * This static method ensures that only one instance of Renderer exists
     * throughout the application's lifetime. It provides global access to that instance.
     *
     * @return Reference to the singleton Renderer instance.
     */
    Renderer& Renderer::GetInstance()
    {
        static Renderer instance;
        return instance;
    }

    /**
     * @brief Applies the specified draw flags to the current rendering context.
     *
     * This function sets various rendering options based on the provided draw flags.
     * It modifies depth writing, polygon mode, and other rendering states as needed.
     *
     * @param flags The draw flags to apply.
     */
    void Renderer::ApplyDrawFlags(DrawFlags flags)
    {
        switch (s_RenderingAPI)
        {
        case RenderingAPI::OpenGL:
            GL_Renderer::ApplyDrawFlags(flags);
            break;
        case RenderingAPI::Vulkan:
            MOTION_ASSERT(false, "Vulkan is not implemented yet!");
            break;
        case RenderingAPI::DirectX:
            MOTION_ASSERT(false, "DirectX is not implemented yet!");
            break;
        default:
            MOTION_ASSERT(false, "Unknown rendering API!");
            break;
        };
    }

    /**
     * @brief Resets the draw flags to their default state.
     *
     * This function clears any previously set draw flags and restores the default rendering state.
     * It is typically called at the end of a frame or before starting a new frame.
     */
    void Renderer::ResetDrawFlags()
    {
        switch (s_RenderingAPI)
        {
        case RenderingAPI::OpenGL:
            GL_Renderer::ResetDrawFlags();
            break;
        case RenderingAPI::Vulkan:
            MOTION_ASSERT(false, "Vulkan is not implemented yet!");
            break;
        case RenderingAPI::DirectX:
            MOTION_ASSERT(false, "DirectX is not implemented yet!");
            break;
        default:
            MOTION_ASSERT(false, "Unknown rendering API!");
            break;
        }
    }
}