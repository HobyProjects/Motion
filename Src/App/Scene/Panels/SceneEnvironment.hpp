#pragma once

#include "ScenePanel.hpp"
#include "SceneCommon.hpp"
#include "SceneUtils.hpp"

namespace Motion
{
    class SceneEnvironmentSettings : public IPanel
    {
        public:
            SceneEnvironmentSettings() = default;
            virtual ~SceneEnvironmentSettings() = default;

            void OnRender(Scene* scene) override;
    };
}