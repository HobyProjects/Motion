#pragma once

#include "ScenePanel.hpp"

namespace Motion
{
    class MaterialEditor : public IPanel
    {
    public:
        MaterialEditor() = default;
        virtual ~MaterialEditor() = default;

        virtual void OnCreate(Scene* scene) override;
        virtual void OnRender(Scene* scene) override;
        
    private:
        void DrawMaterialUI(std::shared_ptr<Material>& mat);
        
    private:
        std::vector<std::shared_ptr<BaseMaterial>> m_Materials{};
    };
}