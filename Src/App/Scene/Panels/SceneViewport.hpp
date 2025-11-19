#pragma once

#include <imgui/imgui.h>
#include <imguizmo/ImGuizmo.h>

#include "ScenePanel.hpp"

namespace Motion
{
    class SceneView : public IPanel
    {
    public:
        SceneView() = default;
        virtual ~SceneView() = default;

        void OnRender(Scene* scene) override;
    private:
        void DrawForceVector(SceneContext& context, const glm::vec3& origin, const glm::vec3& force, const ImU32& color, float scale);
        void DrawMomentumVector(SceneContext& context, const glm::vec3& position, const glm::vec3& momentum, const ImU32& color, float scale);
        void DrawTrajectoryPath(SceneContext& context, const std::vector<glm::vec3>& path, const ImU32& color);
    };
}
