#include "CorePCH.hpp"

#include "Panels.hpp"
#include "SceneEditorLayer.hpp"

namespace Motion
{
    static std::shared_ptr<ImGuiLayer> s_ImGuiLayer{ nullptr };
    static bool s_RequestLayoutReset = false;

    SceneEditorLayer::SceneEditorLayer(WindowHandle handle, const std::shared_ptr<ImGuiLayer>& imguiLayer) : Layer("EditorLayer")
    {
        s_ImGuiLayer = imguiLayer;
        m_Panels = std::make_shared<ScenePanelManager>();
    }

    void SceneEditorLayer::OnAttach()
    {
        //----------------------------------------------------------------------------
        // TEMP: (Until creating a scene serializer and application config serializer)
        //---------------------------------------------------------------------------

        auto& AM = AssetManager::GetInstance();
        AM.Create<IShader>("ENV_SKY", "Assets/Shaders/GLSL/Environment/Environment.glsl");
        AM.Create<IShader>("ENV_IRR", "Assets/Shaders/GLSL/Environment/EnvironmentIrradiance.glsl");
        AM.Create<IShader>("ENV_PRE", "Assets/Shaders/GLSL/Environment/EnvironmentPrefiltered.glsl");
        AM.Create<IShader>("ENV_CUB", "Assets/Shaders/GLSL/Environment/EnvironmentCubeConverter.glsl");
        AM.Create<IShader>("ENV_BRD", "Assets/Shaders/GLSL/Environment/EnvironmentBRDF.glsl");
        
        BaseMaterial::Import("Assets/Materials/Metal/Base.yaml");

        //------------------------------------------------------------------------------------

        m_Viewport.Size                 = m_CurrentViewportSize;
        m_Viewport.FrameSpec.Name       = "SceneEditorFrame";
        m_Viewport.FrameSpec.Width      = (std::uint32_t)m_CurrentViewportSize.x;
        m_Viewport.FrameSpec.Height     = (std::uint32_t)m_CurrentViewportSize.y;
        m_Framebuffer = IFrameBuffer::Create(m_Viewport.FrameSpec);

        // Create a single-sample present FBO for ImGui
        FrameBufferSpecification present{};
        present.Name   = "SceneEditorPresent";
        present.Width  = (std::uint32_t)m_CurrentViewportSize.x;
        present.Height = (std::uint32_t)m_CurrentViewportSize.y;
        m_PresentFramebuffer = IFrameBuffer::Create(present);

        EnvironmentSpecification specEnv;
        specEnv.UseSHDiffuse = false;
        specEnv.BuildBRDFLUT = true;
        specEnv.HDRfile = "Assets/HDRI/Scene.hdr";

        SceneSpecification spec{};
        spec.Name = "Default Scene";
        spec.IsActive = true;
        spec.Viewport = m_Viewport;
        spec.Environment = SceneEnvironment();
        spec.Environment.EnvironmentInstance = IEnvironment::Create(specEnv);

        if (m_Scenes.empty())
            m_Scenes.push_back(std::make_shared<Scene>(spec));

        m_ActiveScene = m_Scenes[0];
        m_ActiveScene->Activate(true);

         // Seed initial texture ID so first UI pass isn't null
        m_SceneTextures[m_ActiveScene] = m_PresentFramebuffer->GetAttachment(FrameBufferColorAttachmentStandards::Standard).ID;

        //------------------------------------------------------------------------------------

        m_Panels->Emplace<SceneViewportPanel>();
        m_Panels->Emplace<SceneEntityInspectPanel>();
        m_Panels->Emplace<SceneEntityPropertiesPanel>();
        m_Panels->Emplace<SceneSettingsPanel>();
        m_Panels->Emplace<SceneViewPanel>();
        m_Panels->Emplace<MaterialEditorPanel>();
    }

    void SceneEditorLayer::OnDetach()
    {
        m_Framebuffer.reset();
        m_Scenes.clear();
    }

    void SceneEditorLayer::OnUpdate(WindowHandle handle, Timer deltaTime)
    {
        m_ActiveScene->OnUpdate(handle, deltaTime);

        m_Framebuffer->Bind();
        Renderer::SetViewport(0, 0, (int)m_CurrentViewportSize.x, (int)m_CurrentViewportSize.y);
        Renderer::ClearColor({ 0.243f, 0.243f, 0.243f, 1.0f });
        Renderer::Clear();

        SceneRenderer::BeginScene();
        SceneRenderer::Submit(m_ActiveScene.get());
        SceneRenderer::EndScene();

        m_Framebuffer->Unbind();

        FrameTextureID finalTex = 0;
        if (m_Framebuffer->GetFrameSpecification().Samples > 1)
        {
            finalTex = m_Framebuffer->ResolveTo(m_PresentFramebuffer.get());
        }
        else
        {
            finalTex = m_Framebuffer->GetAttachment(FrameBufferColorAttachmentStandards::Standard).ID;
            m_Framebuffer->BlitTo(m_PresentFramebuffer.get(), FrameBufferBlitMask::Color, FrameBufferBlitFilter::Linear);
            finalTex = m_PresentFramebuffer->GetAttachment(FrameBufferColorAttachmentStandards::Standard).ID;
        }

        m_SceneTextures[m_ActiveScene] = finalTex;

    }

    void SceneEditorLayer::OnEvent(WindowHandle handle, IEvent& e)
    {
        m_ActiveScene->OnEvent(handle, e);
    }

    static void DrawViewportAxisWidget(const glm::mat4& view, float size = 60.0f)
    {
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        // Find bottom-left of viewport
        ImVec2 windowPos = ImGui::GetWindowPos();
        ImVec2 contentMin = ImGui::GetWindowContentRegionMin();
        ImVec2 viewportMin = ImVec2(windowPos.x + contentMin.x, windowPos.y + contentMin.y);
        ImVec2 axisOrigin = ImVec2(viewportMin.x + size + 10.0f, viewportMin.y + ImGui::GetWindowSize().y - size - 10.0f);
        glm::mat3 camBasis = glm::mat3(glm::transpose(view)); 

        struct Axis {
            glm::vec3 dir;
            ImU32 color;
            const char* label;
        };

        Axis axes[3] = {
            { glm::vec3(1,0,0), IM_COL32(200,60,60,255), "X" },
            { glm::vec3(0,1,0), IM_COL32(60,200,60,255), "Y" },
            { glm::vec3(0,0,1), IM_COL32(80,150,255,255), "Z" }
        };

        for (int i = 0; i < 3; ++i)
        {
            glm::vec3 localDir = camBasis * axes[i].dir; 
            localDir = glm::normalize(localDir);

            float len = size;
            ImVec2 p0 = axisOrigin;
            ImVec2 p1 = ImVec2((axisOrigin.x + localDir.x * len), (axisOrigin.y - localDir.y * len)); 

            drawList->AddLine(p0, p1, axes[i].color, 3.0f);
            ImVec2 labelPos = ImVec2(p1.x + 5.0f, p1.y - 5.0f); 
            drawList->AddText(labelPos, axes[i].color, axes[i].label);
        }

        drawList->AddCircleFilled(axisOrigin, 5.0f, IM_COL32(120, 120, 120, 255));
    }

    void SceneEditorLayer::OnUIRender(WindowHandle handle)
    {
        BuildDockspace();

        if(auto sink = Loggers::GetInstance().ImGuiSink())
        {
            sink->Draw(ICON_MD_TERMINAL " Console");
        }

        ScenePanelContext panelContext;
        panelContext.ActiveScene                    = m_ActiveScene;
        panelContext.ActiveCamera                   = m_ActiveScene->GetCamera();
        panelContext.ActiveSceneSpecification       = m_ActiveScene->GetSpecification();
        panelContext.ActiveViewportTexture          = m_SceneTextures[m_ActiveScene];
        panelContext.UILayerInstance                = s_ImGuiLayer.get();
        panelContext.EditorLayerInstance            = this;

        for (const auto& panel : *m_Panels)
            panel->RenderUI(panelContext);
    }

    void SceneEditorLayer::BuildDockspace()
    {
        ImGuiWindowFlags host =
            ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse;

        ImGuiDockNodeFlags dock = ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_AutoHideTabBar;

        const ImGuiViewport* vp = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(vp->WorkPos);
        ImGui::SetNextWindowSize(vp->WorkSize);
        ImGui::SetNextWindowViewport(vp->ID);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0,0 });
        ImGui::Begin("##DockHost", nullptr, host);
        ImGui::PopStyleVar(3);

        ImGuiID dockspace_id = ImGui::GetID("MainDockspace");
        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DockingEnable)
        {
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
            ImGui::DockSpace(dockspace_id, { 0,0 }, dock);
            ImGui::PopStyleColor();
        }

        ImGui::End();
    }

    void SceneEditorLayer::SetViewportSize(const glm::vec2 & size)
    {
        if (size == m_CurrentViewportSize || size.x <= 1.0f || size.y <= 1.0f) return;

        m_CurrentViewportSize = size;

        if (m_Framebuffer)         m_Framebuffer->ResizeFrame((int)size.x, (int)size.y);
        if (m_PresentFramebuffer)  m_PresentFramebuffer->ResizeFrame((int)size.x, (int)size.y);
        if (m_ActiveScene)         m_ActiveScene->OnViewportSizeChanges(size);
    }

    void SceneEditorLayer::SetActiveScene(const std::shared_ptr<Scene>& scene)
    {
        if (!scene || scene == m_ActiveScene) return;

        if (m_ActiveScene) m_ActiveScene->Activate(false);
        m_ActiveScene = scene;
        m_ActiveScene->Activate(true);

        m_CurrentViewportSize = m_ActiveScene->GetSpecification().Viewport.Size;
        m_Viewport = m_ActiveScene->GetSpecification().Viewport;
    }

    void SceneEditorLayer::RemoveScene(const std::shared_ptr<Scene>& scene)
    {
        if (!scene) return;

        bool deletingActive = (scene == m_ActiveScene);
        m_SceneTextures.erase(scene);

        auto it = std::find(m_Scenes.begin(), m_Scenes.end(), scene);
        if (it != m_Scenes.end()) m_Scenes.erase(it);

        if (m_Scenes.empty()) {
            // ensure at least one scene exists
            SceneSpecification spec{};
            spec.Name = "New Scene";
            spec.IsActive = true;
            spec.Viewport = m_Viewport;
            spec.Environment = SceneEnvironment();
            m_Scenes.push_back(std::make_shared<Scene>(spec));
        }

        if (deletingActive)
        {
            SetActiveScene(m_Scenes.front());
        }
    }

    std::shared_ptr<Scene> SceneEditorLayer::AddNewScene(const std::string& name, bool makeActive)
    {
        SceneSpecification spec{};
        spec.Name = name.empty() ? "Untitled Scene" : name;
        spec.IsActive = false;              // will be set via SetActiveScene if makeActive
        spec.Viewport = m_Viewport;         // copy current viewport config
        spec.Environment = SceneEnvironment(); // fresh env

        auto s = std::make_shared<Scene>(spec);
        m_Scenes.push_back(s);

        if (makeActive)
            SetActiveScene(s);

        return s;
    }

    void SceneEditorLayer::DeleteScene(UUID id)
    {
        if (m_Scenes.empty()) return;

        // If deleting the active scene, we’ll swap active afterward
        bool deletingActive = (m_ActiveScene && m_ActiveScene->GetID() == id);

        // erase from texture cache first (safe even if missing)
        for (auto it = m_SceneTextures.begin(); it != m_SceneTextures.end(); )
        {
            if (it->first && it->first->GetID() == id) it = m_SceneTextures.erase(it);
            else ++it;
        }

        // remove from list
        for (auto it = m_Scenes.begin(); it != m_Scenes.end(); ++it)
        {
            if ((*it)->GetID() == id)
            {
                m_Scenes.erase(it);
                break;
            }
        }

        // fixup active scene
        if (deletingActive)
        {
            if (!m_Scenes.empty())
            {
                m_ActiveScene = m_Scenes.front();
                m_ActiveScene->Activate(true);
            }
            else
            {
                m_ActiveScene.reset();
            }
        }
    }
}