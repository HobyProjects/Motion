#include "CorePCH.hpp"

namespace Motion
{
    bool GL_PlotExporter::SavePlotRegionToPNG(const std::string& filename, ImVec2 plotPos, ImVec2 plotSize)
    {
        std::int32_t width = static_cast<std::int32_t>(plotSize.x);
        std::int32_t height = static_cast<std::int32_t>(plotSize.y);
        
        if (width <= 0 || height <= 0)
            return false;

        std::vector<std::uint8_t> pixels(3 * width * height);
    
        GLint viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);
        std::int32_t viewportHeight = viewport[3];
        
        std::int32_t glY = viewportHeight - static_cast<std::int32_t>(plotPos.y) - height;
        
        glReadPixels(
            static_cast<std::int32_t>(plotPos.x), 
            glY,
            width, 
            height, 
            GL_RGB, 
            GL_UNSIGNED_BYTE, 
            pixels.data()
        );

        std::vector<std::uint8_t> flipped(3 * width * height);
        for (std::int32_t y = 0; y < height; y++)
        {
            memcpy(
                flipped.data() + y * width * 3,
                pixels.data() + (height - 1 - y) * width * 3,
                width * 3
            );
        }

        std::int32_t result = stbi_write_png(filename.c_str(), width, height, 3, flipped.data(), width * 3);
        return result != 0;
    }

    bool GL_PlotExporter::SaveImGuiRegionToPNG(const std::string& filename)
    {
        ImVec2 windowPos = ImGui::GetWindowPos();
        ImVec2 windowSize = ImGui::GetWindowSize();
        
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        
        // Handle DPI scaling
        ImVec2 framebufferScale = ImVec2(1.0f, 1.0f);
        if (viewport->DpiScale > 1.0f)
        {
            framebufferScale = ImVec2(viewport->DpiScale, viewport->DpiScale);
        }
        
        ImVec2 fbPos = ImVec2(
            windowPos.x * framebufferScale.x,
            windowPos.y * framebufferScale.y
        );
        
        ImVec2 fbSize = ImVec2(
            windowSize.x * framebufferScale.x,
            windowSize.y * framebufferScale.y
        );
        
        return SavePlotRegionToPNG(filename, fbPos, fbSize);
    }

    bool GL_PlotExporter::SaveCurrentPlotToPNG(const std::string& filename, std::int32_t width, std::int32_t height)
    {
        if (width <= 0 || height <= 0)
            return false;

        GLint previousFramebuffer;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFramebuffer);
        
        GLint previousViewport[4];
        glGetIntegerv(GL_VIEWPORT, previousViewport);

        GLuint framebuffer, texture;
        glGenFramebuffers(1, &framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
        
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, previousFramebuffer);
            glDeleteFramebuffers(1, &framebuffer);
            glDeleteTextures(1, &texture);
            return false;
        }
        
        glViewport(0, 0, width, height);
        
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        std::vector<std::uint8_t> pixels(3 * width * height);
        glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
        
        std::vector<std::uint8_t> flipped(3 * width * height);
        for (std::int32_t y = 0; y < height; y++)
        {
            memcpy(
                flipped.data() + y * width * 3,
                pixels.data() + (height - 1 - y) * width * 3,
                width * 3
            );
        }
        
        std::int32_t result = stbi_write_png(filename.c_str(), width, height, 3, flipped.data(), width * 3);
        glBindFramebuffer(GL_FRAMEBUFFER, previousFramebuffer);
        glViewport(previousViewport[0], previousViewport[1], previousViewport[2], previousViewport[3]);
        
        glDeleteFramebuffers(1, &framebuffer);
        glDeleteTextures(1, &texture);
        
        return result != 0;
    }

    bool GL_PlotExporter::ExportPlotDataToCSV(const std::string& filename, const ScrollingBuffer& buffer, const std::string& xLabel, const std::string& yLabel)
    {
        FILE* file = fopen(filename.c_str(), "w");
        if (!file)
            return false;
        
        fprintf(file, "%s,%s\n", xLabel.c_str(), yLabel.c_str());
        std::int32_t dataSize = static_cast<std::int32_t>(buffer.Data.size());
        
        if (dataSize == 0)
        {
            fclose(file);
            return true;
        }
        
        if (dataSize < buffer.MaxSize)
        {
            for (std::int32_t i = 0; i < dataSize; i++)
            {
                fprintf(file, "%.6f,%.6f\n", buffer.Data[i].x, buffer.Data[i].y);
            }
        }
        else
        {
            for (std::int32_t i = 0; i < dataSize; i++)
            {
                std::int32_t idx = (buffer.Offset + i) % buffer.MaxSize;
                fprintf(file, "%.6f,%.6f\n", buffer.Data[idx].x, buffer.Data[idx].y);
            }
        }
        
        fclose(file);
        return true;
    }
}