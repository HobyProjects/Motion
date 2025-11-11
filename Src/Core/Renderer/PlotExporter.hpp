#pragma once

#include <string>
#include <cstdint>
#include <memory>
#include <vector>

#include <imgui/imgui.h>
#include <imgui/implot.h>
#include <glm/glm.hpp>

namespace Motion
{
    /**
     * @brief Circular buffer for storing plot data points with automatic wrapping
     */
    struct ScrollingBuffer 
    {
        int MaxSize;
        int Offset;
        ImVector<ImVec2> Data;
        
        ScrollingBuffer(int max_size = 2000) 
        {
            MaxSize = max_size;
            Offset = 0;
            Data.reserve(MaxSize);
        }

        void AddPoint(float x, float y) 
        {
            if (Data.size() < MaxSize)
            {
                Data.push_back(ImVec2(x, y));
            }
            else 
            {
                Data[Offset] = ImVec2(x, y);
                Offset = (Offset + 1) % MaxSize;
            }
        }
        
        void Erase() 
        {
            if (Data.size() > 0) 
            {
                Data.shrink(0);
                Offset = 0;
            }
        }

        /**
         * @brief Get the data point at a given index, accounting for circular buffer offset
         */
        ImVec2 GetPoint(int index) const
        {
            if (Data.size() < MaxSize)
            {
                return Data[index];
            }
            else
            {
                return Data[(Offset + index) % MaxSize];
            }
        }

        /**
         * @brief Get the actual number of data points stored
         */
        int GetSize() const
        {
            return static_cast<int>(Data.size());
        }
    };

    /**
     * @brief Rolling buffer that wraps data within a time span
     */
    struct RollingBuffer 
    {
        float Span;
        ImVector<ImVec2> Data;
        
        RollingBuffer() 
        {
            Span = 10.0f;
            Data.reserve(2000);
        }

        void AddPoint(float x, float y) 
        {
            float xmod = fmodf(x, Span);
            if (!Data.empty() && xmod < Data.back().x)
                Data.shrink(0);
            Data.push_back(ImVec2(xmod, y));
        }
    };

    /**
     * @brief Stores plot data and configuration for a single entity
     */
    struct EntityPlotData
    {
        ScrollingBuffer LinearVelocity;
        ScrollingBuffer AngularVelocity;

        ImVec2 LinearPlotPos;
        ImVec2 LinearPlotSize;
        ImVec2 AngularPlotPos;
        ImVec2 AngularPlotSize;
        float TimeAccumulator = 0.0f;

        std::vector<float> TimePoints;
        
        glm::vec3 ImpulseDirection = glm::vec3(0.0f, 1.0f, 0.0f);
        float ImpulseMagnitude = 10.0f;
        glm::vec3 ImpulsePosition = glm::vec3(0.0f, 0.0f, 0.0f);
        bool UseLocalPosition = true;
        
        EntityPlotData() : LinearVelocity(2000), AngularVelocity(2000) {}
    };

    /**
     * @brief Interface for exporting plot data and images
     * 
     * Provides methods for capturing plot regions as PNG images and
     * exporting plot data to CSV format. Implementation is rendering API specific.
     */
    class IPlotExporter
    {
    public:
        IPlotExporter() = default;
        virtual ~IPlotExporter() = default;

        /**
         * @brief Save a specific screen region to PNG
         * @param filename Output file path (should end with .png)
         * @param plotPos Position of the region in screen coordinates
         * @param plotSize Size of the region in pixels
         * @return true if successful, false otherwise
         */
        virtual bool SavePlotRegionToPNG(const std::string& filename, ImVec2 plotPos, ImVec2 plotSize) = 0;
        
        /**
         * @brief Save the current ImGui window to PNG
         * @param filename Output file path (should end with .png)
         * @return true if successful, false otherwise
         */
        virtual bool SaveImGuiWindowToPNG(const std::string& filename) = 0;
        
        /**
         * @brief Export plot buffer data to CSV format
         * @param filename Output file path (should end with .csv)
         * @param buffer The scrolling buffer containing the data
         * @param xLabel Label for the X-axis column
         * @param yLabel Label for the Y-axis column
         * @return true if successful, false otherwise
         */
        virtual bool ExportPlotDataToCSV(const std::string& filename, 
                                        const ScrollingBuffer& buffer, 
                                        const std::string& xLabel = "Time", 
                                        const std::string& yLabel = "Value") = 0;

        /**
         * @brief Factory method to create the appropriate exporter for the current rendering API
         * @return Shared pointer to the created exporter, or nullptr on failure
         */
        static std::shared_ptr<IPlotExporter> Create();
    };
}