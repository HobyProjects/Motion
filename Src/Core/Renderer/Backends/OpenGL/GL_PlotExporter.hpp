#pragma once

#include "PlotExporter.hpp"

namespace Motion
{
    class GL_PlotExporter : public IPlotExporter
    {
        public:
            GL_PlotExporter() = default;
            virtual ~GL_PlotExporter() = default;

            virtual bool SavePlotRegionToPNG(const std::string& filename, ImVec2 plotPos, ImVec2 plotSize) override;
            virtual bool SaveImGuiRegionToPNG(const std::string& filename) override;
            virtual bool SaveCurrentPlotToPNG(const std::string& filename, std::int32_t width = 1920, std::int32_t height = 1080) override;
            virtual bool ExportPlotDataToCSV(const std::string& filename, const ScrollingBuffer& buffer, const std::string& xLabel = "Time", const std::string& yLabel = "Value") override;
    };
}