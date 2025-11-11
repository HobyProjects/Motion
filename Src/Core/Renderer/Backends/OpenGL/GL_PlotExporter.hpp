#pragma once

#include "PlotExporter.hpp"

namespace Motion
{
    /**
     * @brief OpenGL implementation of the plot exporter interface
     * 
     * Uses OpenGL functions to capture framebuffer contents and
     * save them as PNG images using stb_image_write.
     */
    class GL_PlotExporter : public IPlotExporter
    {
    public:
        GL_PlotExporter() = default;
        virtual ~GL_PlotExporter() = default;

        /**
         * @brief Save a specific screen region to PNG using glReadPixels
         */
        virtual bool SavePlotRegionToPNG(const std::string& filename, ImVec2 plotPos, ImVec2 plotSize) override;
        
        /**
         * @brief Save the current ImGui window to PNG, handling DPI scaling
         */
        virtual bool SaveImGuiWindowToPNG(const std::string& filename) override;
        
        /**
         * @brief Export buffer data to CSV file in chronological order
         */
        virtual bool ExportPlotDataToCSV(const std::string& filename, 
                                        const ScrollingBuffer& buffer, 
                                        const std::string& xLabel = "Time", 
                                        const std::string& yLabel = "Value") override;

    private:
        /**
         * @brief Internal helper to read pixels from the current framebuffer and save as PNG
         * @param filename Output file path
         * @param x X coordinate in OpenGL framebuffer space
         * @param y Y coordinate in OpenGL framebuffer space  
         * @param width Width in pixels
         * @param height Height in pixels
         * @return true if successful
         */
        bool ReadFramebufferAndSave(const std::string& filename, 
                                   std::int32_t x, std::int32_t y, 
                                   std::int32_t width, std::int32_t height);
    };
}