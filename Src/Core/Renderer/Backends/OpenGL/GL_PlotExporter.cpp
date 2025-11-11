#include "CorePCH.hpp"
#include "GL_PlotExporter.hpp"

#include <glad/glad.h>
#include <stb/stb_image_write.h>

namespace Motion
{
    bool GL_PlotExporter::ReadFramebufferAndSave(const std::string& filename, 
                                                 std::int32_t x, std::int32_t y, 
                                                 std::int32_t width, std::int32_t height)
    {
        // Validate dimensions
        if (width <= 0 || height <= 0)
        {
            MOTION_CORE_ERROR("Invalid dimensions for framebuffer capture: {}x{}", width, height);
            return false;
        }

        // Allocate buffer for pixel data (RGB format, 3 bytes per pixel)
        const std::size_t bufferSize = 3 * width * height;
        std::vector<std::uint8_t> pixels(bufferSize);
        
        // Read pixels from the current framebuffer
        // Note: OpenGL's origin is bottom-left, so we need to handle coordinate conversion
        glReadPixels(x, y, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
        
        // Check for OpenGL errors
        GLenum error = glGetError();
        if (error != GL_NO_ERROR)
        {
            MOTION_CORE_ERROR("OpenGL error during glReadPixels: {}", error);
            return false;
        }

        // Flip the image vertically (OpenGL reads bottom-to-top, PNG writes top-to-bottom)
        std::vector<std::uint8_t> flipped(bufferSize);
        const std::int32_t rowSize = width * 3;
        
        for (std::int32_t row = 0; row < height; row++)
        {
            const std::uint8_t* srcRow = pixels.data() + (height - 1 - row) * rowSize;
            std::uint8_t* dstRow = flipped.data() + row * rowSize;
            std::memcpy(dstRow, srcRow, rowSize);
        }

        // Write PNG file using stb_image_write
        // Parameters: filename, width, height, channels, data, stride_in_bytes
        std::int32_t result = stbi_write_png(filename.c_str(), width, height, 3, flipped.data(), rowSize);
        
        if (result == 0)
        {
            MOTION_ERROR("Failed to write PNG file: {}", filename);
            return false;
        }

        MOTION_CORE_INFO("Successfully saved plot to: {}", filename);
        return true;
    }

    bool GL_PlotExporter::SavePlotRegionToPNG(const std::string& filename, ImVec2 plotPos, ImVec2 plotSize)
    {
        // Convert ImGui screen coordinates to OpenGL framebuffer coordinates
        const std::int32_t width = static_cast<std::int32_t>(plotSize.x);
        const std::int32_t height = static_cast<std::int32_t>(plotSize.y);
        
        if (width <= 0 || height <= 0)
        {
            MOTION_CORE_ERROR("Invalid plot dimensions: {}x{}", width, height);
            return false;
        }

        // Get the current viewport to determine framebuffer height
        GLint viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);
        const std::int32_t viewportHeight = viewport[3];
        
        // Convert from ImGui coordinates (top-left origin) to OpenGL coordinates (bottom-left origin)
        const std::int32_t glX = static_cast<std::int32_t>(plotPos.x);
        const std::int32_t glY = viewportHeight - static_cast<std::int32_t>(plotPos.y) - height;
        
        return ReadFramebufferAndSave(filename, glX, glY, width, height);
    }

    bool GL_PlotExporter::SaveImGuiWindowToPNG(const std::string& filename)
    {
        // Get the current ImGui window's position and size
        ImVec2 windowPos = ImGui::GetWindowPos();
        ImVec2 windowSize = ImGui::GetWindowSize();
        
        // Get DPI scaling information
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        const float dpiScale = (viewport != nullptr && viewport->DpiScale > 0.0f) ? viewport->DpiScale : 1.0f;
        
        // Apply DPI scaling to convert from ImGui coordinates to framebuffer coordinates
        const ImVec2 framebufferPos = ImVec2(
            windowPos.x * dpiScale,
            windowPos.y * dpiScale
        );
        
        const ImVec2 framebufferSize = ImVec2(
            windowSize.x * dpiScale,
            windowSize.y * dpiScale
        );
        
        MOTION_CORE_INFO("Capturing ImGui window at ({}, {}) with size {}x{} (DPI scale: {})", 
                       framebufferPos.x, framebufferPos.y, 
                       framebufferSize.x, framebufferSize.y, 
                       dpiScale);
        
        return SavePlotRegionToPNG(filename, framebufferPos, framebufferSize);
    }

    bool GL_PlotExporter::ExportPlotDataToCSV(const std::string& filename, 
                                              const ScrollingBuffer& buffer, 
                                              const std::string& xLabel, 
                                              const std::string& yLabel)
    {
        // Open file for writing
        FILE* file = fopen(filename.c_str(), "w");
        if (!file)
        {
            MOTION_CORE_ERROR("Failed to open CSV file for writing: {}", filename);
            return false;
        }
        
        // Write CSV header
        fprintf(file, "%s,%s\n", xLabel.c_str(), yLabel.c_str());
        
        const std::int32_t dataSize = buffer.GetSize();
        
        // Handle empty buffer
        if (dataSize == 0)
        {
            MOTION_CORE_WARN("Exporting empty buffer to CSV: {}", filename);
            fclose(file);
            return true;
        }
        
        // Export data in chronological order
        // If buffer hasn't wrapped yet (size < MaxSize), data is already in order
        // If buffer has wrapped, we need to start from Offset to maintain chronological order
        if (dataSize < buffer.MaxSize)
        {
            // Buffer hasn't wrapped - data is in chronological order
            for (std::int32_t i = 0; i < dataSize; i++)
            {
                const ImVec2& point = buffer.Data[i];
                fprintf(file, "%.6f,%.6f\n", point.x, point.y);
            }
        }
        else
        {
            // Buffer has wrapped - start from Offset to get chronological order
            for (std::int32_t i = 0; i < dataSize; i++)
            {
                const ImVec2& point = buffer.GetPoint(i);
                fprintf(file, "%.6f,%.6f\n", point.x, point.y);
            }
        }
        
        fclose(file);
        MOTION_CORE_INFO("Successfully exported {} data points to: {}", dataSize, filename);
        return true;
    }
}