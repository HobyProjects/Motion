#include "CorePCH.hpp"

namespace Motion
{
    std::shared_ptr<IPlotExporter> IPlotExporter::Create()
    {
        switch(Renderer::GetAPI())
        {
            case RenderingAPI::OpenGL:
                return std::make_shared<GL_PlotExporter>();
            default:
                MOTION_ASSERT(false, "Unsupported Rendering API for Plot Exporter!");
                return nullptr;
        };
    }
}