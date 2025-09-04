#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Scene.hpp"

namespace Motion
{
    class SceneEditorLayer;

    struct ScenePanelContext
    {
        std::shared_ptr<Scene>  ActiveScene{ nullptr };
        SceneSpecification      ActiveSceneSpecification{};
        SceneCamera             ActiveCamera{};
        FrameTextureID          ActiveViewportTexture{};
        ImGuiLayer*             UILayerInstance{ nullptr };
        SceneEditorLayer*       EditorLayerInstance{ nullptr };
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

}