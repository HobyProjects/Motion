#pragma once

#include "Entity.hpp"
#include "Components.hpp"
#include "Buffers.hpp"

#include "SceneCamera.hpp"
#include "SceneRenderer.hpp"
#include "SceneEnviroment.hpp"
#include "ImguiLayer.hpp"

namespace Motion
{
    struct SceneViewport
    {
        FrameBufferSpecification FrameSpec{};
        glm::vec2 Size{ 0.0f, 0.0f };

        bool Focused{ false };
        bool Hovered{ false };

        SceneViewport() = default;
        ~SceneViewport() = default;
    };

    struct SceneSpecification
    {
        UUID SceneHandle{};
        std::string Name{};
        bool IsActive{ false };
        SceneViewport Viewport{};
        SceneEnvironment Environment{};

        SceneSpecification() = default;
        ~SceneSpecification() = default;
    };

    class SceneEditorLayer; // Forward declaration

    struct ScenePanelContext
    {
        std::shared_ptr<Scene> ActiveScene{ nullptr };
        SceneSpecification ActiveSceneSpecification{};
        SceneCamera ActiveCamera{};
        FrameTextureID ActiveViewportTexture{};
        ImGuiLayer* UILayerInstance{ nullptr };
        SceneEditorLayer* EditorLayerInstance{ nullptr };
    };

    enum class PanelCategory
    {
        ScenePanel,
        AssetsPanel,
        InspectorPanel,
        PropertiesPanel,
        ConsolePanel
    };

    class IScenePanel
    {
    public:
        virtual ~IScenePanel() = default;

        virtual std::string GetTitle() const = 0;
        virtual PanelCategory GetCategory() const = 0;
        virtual void RenderUI(ScenePanelContext& context) = 0;
    };

    class ScenePanelManager
    {
    public:
        ScenePanelManager() = default;
        ~ScenePanelManager() = default;

        template<class T, class... Args>
        T* Emplace(Args&&... args)
        {
            auto p = std::make_unique<T>(std::forward<Args>(args)...);
            T* raw = p.get();
            m_Panels.emplace_back(std::move(p));
            return raw;
        }

        std::vector<std::unique_ptr<IScenePanel>>::const_iterator begin() const { return m_Panels.begin(); }
        std::vector<std::unique_ptr<IScenePanel>>::const_iterator end() const { return m_Panels.end(); }

    private:
        std::vector<std::unique_ptr<IScenePanel>> m_Panels;
    };

    class SceneViewportPanel final : public IScenePanel
    {
    public:
        SceneViewportPanel() = default;
        ~SceneViewportPanel() = default;

        virtual std::string GetTitle() const override { return m_Title; }
        virtual PanelCategory GetCategory() const override { return PanelCategory::ScenePanel; }
        virtual void RenderUI(ScenePanelContext& context) override;

    private:
        std::string m_Title{ "SceneViewport" };
    };

    class SceneEntityInspectPanel final : public IScenePanel
    {
    public:
        SceneEntityInspectPanel() = default;
        ~SceneEntityInspectPanel() = default;

        virtual std::string GetTitle() const override { return m_Title; }
        virtual PanelCategory GetCategory() const override { return PanelCategory::PropertiesPanel; }
        virtual void RenderUI(ScenePanelContext& context) override;

    private:
        std::string m_Title{ "SceneEntities" };
    };

    class SceneEntityPropertiesPanel final : public IScenePanel
    {
    public:
        SceneEntityPropertiesPanel() = default;
        ~SceneEntityPropertiesPanel() = default;

        virtual std::string GetTitle() const override { return m_Title; }
        virtual PanelCategory GetCategory() const override { return PanelCategory::PropertiesPanel; }
        virtual void RenderUI(ScenePanelContext& context) override;

    private:
        std::string m_Title{ "SceneEntityProperties" };
    };

    class SceneSettingsPanel final : public IScenePanel
    {
    public:
        SceneSettingsPanel() = default;
        ~SceneSettingsPanel() = default;

        virtual std::string GetTitle() const override { return m_Title; }
        virtual PanelCategory GetCategory() const override { return PanelCategory::InspectorPanel; }
        virtual void RenderUI(ScenePanelContext& context) override;

    private:
        std::string m_Title{ "SceneSettings" };
    };

    class Scene
    {
    public:
        Scene() = default;
        Scene(const SceneSpecification& spec);
        ~Scene() = default;

        //---------------------------------------------------
        // Scene Update, Events and Rendering
        //---------------------------------------------------
        void OnUpdate(WindowHandle handle, Timer deltaTime) noexcept;
        void OnEvent(WindowHandle handle, IEvent& e) noexcept;
        void OnViewportSizeChanges(const glm::vec2& size) noexcept;

        //---------------------------------------------------
        // Entity Getters and Setters
        //---------------------------------------------------
        void SelectEntityIf();
        void SelectedEntity(const std::shared_ptr<Entity>& entity) { m_SelectedEntity = entity; }
        void EmplaceEntity(const std::shared_ptr<Entity>& entity) { m_Entities.emplace_back(std::move(entity)); }

        [[nodiscard]] std::shared_ptr<Entity> GetSelectedEntity() const { return m_SelectedEntity; }
        [[nodiscard]] std::shared_ptr<Entity> PickEntity(const glm::vec2& mousePos, const glm::vec2& viewportSize);

        //-----------------------------------------------
        // Scene Camera
        //-----------------------------------------------
        [[nodiscard]] SceneCamera& GetCamera() { return m_Camera; }
        [[nodiscard]] glm::mat4 GetCameraProjection() const { return m_Camera.Camera.Projection; }
        [[nodiscard]] glm::mat4 GetCameraView() const { return m_Camera.Camera.View; }
        [[nodiscard]] glm::vec3 GetCameraPosition() const { return m_Camera.Camera.Position; }

        //------------------------------------------------
        // Iterator for Scene Entities
        //------------------------------------------------
        [[nodiscard]] std::vector<std::shared_ptr<Entity>>::iterator begin() { return m_Entities.begin(); }
        [[nodiscard]] std::vector<std::shared_ptr<Entity>>::iterator end() { return m_Entities.end(); }
        [[nodiscard]] std::vector<std::shared_ptr<Entity>>::const_iterator begin() const { return m_Entities.begin(); }
        [[nodiscard]] std::vector<std::shared_ptr<Entity>>::const_iterator end() const { return m_Entities.end(); }

        //------------------------------------------------
        // Scene Specification
        //------------------------------------------------
        [[nodiscard]] SceneSpecification& GetSpecification() { return m_Specification; }
        [[nodiscard]] SceneEnvironment& GetEnvironment() { return m_Specification.Environment; }
        [[nodiscard]] std::string GetName() const { return m_Specification.Name; }
        [[nodiscard]] UUID GetID() const { return m_Specification.SceneHandle; }
        [[nodiscard]] bool IsActive() const { return m_Specification.IsActive; }

        void Activate(bool active) { m_Specification.IsActive = active; }
        void RenderEnvironment() const;

    private:
        std::vector<std::shared_ptr<Entity>> m_Entities{};
        std::shared_ptr<Entity> m_SelectedEntity{ EntityFactory::EMPTYENTITY };

        SceneSpecification m_Specification;
        SceneCamera m_Camera;

        friend class SceneRenderer;
    };
}