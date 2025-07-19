#pragma once

#include <memory>
#include <vector>

#include "Layer.hpp"

namespace Motion::Core
{
    class LayersManager
    {
    private:
        LayersManager() = default;
        ~LayersManager();

        LayersManager(const LayersManager&) = delete;
        LayersManager& operator=(const LayersManager&) = delete;
        LayersManager(LayersManager&&) = delete;
        LayersManager& operator=(LayersManager&&) = delete;

    public:
        /**
         * @brief Returns the singleton instance of LayersManager.
         *
         * This method ensures that only one instance of LayersManager exists throughout the application.
         * It initializes the instance if it does not already exist.
         *
         * @return Reference to the singleton LayersManager instance.
         */
        static LayersManager& GetInstance()
        {
            static LayersManager instance;
            return instance;
        }

    public:
        void PushLayer(std::shared_ptr<Layer> layer);
        void PushOverlay(std::shared_ptr<Layer> layer);
        void PopLayer(std::shared_ptr<Layer> layer);
        void PopOverlay(std::shared_ptr<Layer> layer);

        std::vector<std::shared_ptr<Layer>>::iterator begin() { return m_Layers.begin(); }
        std::vector<std::shared_ptr<Layer>>::iterator end() { return m_Layers.end(); }
        std::vector<std::shared_ptr<Layer>>::const_iterator begin() const { return m_Layers.begin(); }
        std::vector<std::shared_ptr<Layer>>::const_iterator end() const { return m_Layers.end(); }
        std::vector<std::shared_ptr<Layer>>::reverse_iterator rbegin() { return m_Layers.rbegin(); }
        std::vector<std::shared_ptr<Layer>>::reverse_iterator rend() { return m_Layers.rend(); }
        std::vector<std::shared_ptr<Layer>>::const_reverse_iterator rbegin() const { return m_Layers.rbegin(); }
        std::vector<std::shared_ptr<Layer>>::const_reverse_iterator rend() const { return m_Layers.rend(); }

    private:
        std::vector<std::shared_ptr<Layer>> m_Layers;
        unsigned int m_LayerInsertIndex = 0;
    };
}