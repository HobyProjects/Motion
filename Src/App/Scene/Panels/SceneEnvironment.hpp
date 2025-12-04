#pragma once

#include "ScenePanel.hpp"
#include "SceneCommon.hpp"
#include "SceneUtils.hpp"
#include "PostProcessing.hpp" 

namespace Motion
{
    class SceneEnvironmentSettings : public IPanel
    {
        public:
            SceneEnvironmentSettings() = default;
            virtual ~SceneEnvironmentSettings() = default;

            void OnRender(Scene* scene) override;
        
        private:
            void RenderPostProcessingSection(Scene* scene);
            void RenderBloomControls(ScenePhysics::PostProcessingSettings& pp, bool& changed);
            void RenderToneMappingControls(ScenePhysics::PostProcessingSettings& pp, bool& changed);
            void RenderColorGradingControls(ScenePhysics::PostProcessingSettings& pp, bool& changed);
            void RenderVignetteControls(ScenePhysics::PostProcessingSettings& pp, bool& changed);
            void RenderFXAAControls(ScenePhysics::PostProcessingSettings& pp, bool& changed);
    };
}