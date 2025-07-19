#include "CorePCH.hpp"

namespace Motion::Core
{
    /**
     * @brief Destructor for the LayersManager class.
     *
     * Iterates through all managed layers, calling their OnDetach() method to perform
     * any necessary cleanup before clearing the internal layer collection.
     * Ensures that all layers are properly detached before the LayersManager is destroyed.
     */
    LayersManager::~LayersManager()
    {
        for (auto layer : m_Layers)
        {
            layer->OnDetach();
        }

        m_Layers.clear();
    }

    /**
     * @brief Adds a new layer to the LayersManager at the current insertion index.
     *
     * This function inserts the provided layer into the internal layer list at the position
     * specified by m_LayerInsertIndex. After insertion, it calls the OnAttach() method of the layer
     * to perform any necessary initialization. The insertion index is then incremented to ensure
     * subsequent layers are added in the correct order.
     *
     * @param layer A shared pointer to the Layer to be added.
     */
    void LayersManager::PushLayer(std::shared_ptr<Layer> layer)
    {
        m_Layers.emplace(m_Layers.begin() + m_LayerInsertIndex, layer);
        layer->OnAttach();
        m_LayerInsertIndex++;
    }

    /**
     * @brief Removes the specified layer from the managed layers stack.
     *
     * This function searches for the given layer within the range of layers that have been inserted
     * (from the beginning up to m_LayerInsertIndex). If the layer is found, it calls the layer's
     * OnDetach() method, removes it from the layers container, and updates the insertion index.
     *
     * @param layer A shared pointer to the Layer instance to be removed.
     */
    void LayersManager::PopLayer(std::shared_ptr<Layer> layer)
    {
        auto it = std::find(m_Layers.begin(), m_Layers.begin() + m_LayerInsertIndex, layer);
        if (it != m_Layers.end())
        {
            layer->OnDetach();
            m_Layers.erase(it);
            m_LayerInsertIndex--;
        }
    }

    /**
     * @brief Adds an overlay layer to the LayersManager.
     *
     * This function appends the given overlay layer to the internal layer stack
     * and calls its OnAttach() method to perform any necessary initialization.
     *
     * @param overlay A shared pointer to the Layer to be added as an overlay.
     */
    void LayersManager::PushOverlay(std::shared_ptr<Layer> overlay)
    {
        m_Layers.emplace_back(overlay);
        overlay->OnAttach();
    }

    /**
     * @brief Removes the specified overlay layer from the manager.
     *
     * This function searches for the given overlay in the overlay section of the layer stack,
     * detaches it by calling its OnDetach() method, and then removes it from the internal list.
     *
     * @param overlay A shared pointer to the overlay layer to be removed.
     */
    void LayersManager::PopOverlay(std::shared_ptr<Layer> overlay)
    {
        auto it = std::find(m_Layers.begin() + m_LayerInsertIndex, m_Layers.end(), overlay);
        if (it != m_Layers.end())
        {
            overlay->OnDetach();
            m_Layers.erase(it);
        }
    }
}