#pragma once

#include <string>
#include <cstdint>

#include <imgui/imgui.h>
#include <imgui/implot.h>
#include <glm/glm.hpp>

namespace Motion
{
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
                Data.push_back(ImVec2(x, y));
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
    };

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

    struct EntityPlotData
    {
        ScrollingBuffer LinearVelocity;
        ScrollingBuffer AngularVelocity;

        ImVec2 LinearPlotPos;
        ImVec2 LinearPlotSize;
        ImVec2 AngularPlotPos;
        ImVec2 AngularPlotSize;
        float TimeAccumulator = 0.0f;
        
        glm::vec3 ImpulseDirection = glm::vec3(0.0f, 1.0f, 0.0f);
        float ImpulseMagnitude = 10.0f;
        glm::vec3 ImpulsePosition = glm::vec3(0.0f, 0.0f, 0.0f);
        bool UseLocalPosition = true;
        
        EntityPlotData() : LinearVelocity(2000), AngularVelocity(2000) {}
    };

    class IPlotExporter
    {
    public:
        IPlotExporter() = default;
        virtual ~IPlotExporter() = default;

        virtual bool SavePlotRegionToPNG(const std::string& filename, ImVec2 plotPos, ImVec2 plotSize) = 0;
        virtual bool SaveImGuiRegionToPNG(const std::string& filename) = 0;
        virtual bool SaveCurrentPlotToPNG(const std::string& filename, std::int32_t width = 1920, std::int32_t height = 1080) = 0;
        virtual bool ExportPlotDataToCSV(const std::string& filename, const ScrollingBuffer& buffer, const std::string& xLabel = "Time", const std::string& yLabel = "Value") = 0;

        static std::shared_ptr<IPlotExporter> Create();
    };
}