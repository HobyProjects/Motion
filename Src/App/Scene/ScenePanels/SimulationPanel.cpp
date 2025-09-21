#include "CorePCH.hpp"
#include "SimulationPanel.hpp"

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
            Offset  = 0;
            Data.reserve(MaxSize);
        }

        void AddPoint(float x, float y) 
        {
            if (Data.size() < MaxSize)
                Data.push_back(ImVec2(x,y));
            else 
            {
                Data[Offset] = ImVec2(x,y);
                Offset =  (Offset + 1) % MaxSize;
            }
        }
        void Erase() 
        {
            if (Data.size() > 0) 
            {
                Data.shrink(0);
                Offset  = 0;
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

    void SimulationPanel::RenderUI(ScenePanelContext& context)
    {
        ImGui::Begin(ICON_MD_VIEW_COMFORTABLE " Simulation Inspector");
        
        if(context.ActiveScene->InSimulationMode())
        {
            auto entities = context.ActiveScene->GetEntities();
            for(const auto& entt : entities)
            {
                auto& tag           = entt->GetComponent<TagComponent>();
                auto& transform     = entt->GetComponent<TransformComponent>();
                auto& rb            = entt->GetComponent<RigidBodyComponent>();
                auto& col           = entt->GetComponent<ColliderComponent>();

                if(CollapsibleSection(tag.Tag.c_str(), false))
                {
                    ImGui::Text("Position   : %f, %f, %f", transform.Translation.x, transform.Translation.y, transform.Translation.z);
                    ImGui::Text("Rotation   : %f, %f, %f", transform.Rotation.x, transform.Rotation.y, transform.Rotation.z);
                    ImGui::Text("Scale      : %f, %f, %f", transform.Scale.x, transform.Scale.y, transform.Scale.z);
                    
                    ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal, 1.5f);

                    auto* body = rb.PhysicsBody;
                    auto* collider = col.Collider;
                    auto* attribute = col.Attributes;

                    ImGui::Text("Mass            : %f (kg)", body->getMass());
                    ImGui::Text("Friction        : %f (N/m^2)", attribute->getFrictionCoefficient());
                    ImGui::Text("Restitution     : %f (0 ~ 1)", attribute->getBounciness());
                    ImGui::Text("Linear Damping  : %f (kg/s)", body->getLinearDamping());
                    ImGui::Text("Angular Damping : %f (kg/s)", body->getAngularDamping());

                    ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal, 1.5f);

                    static ScrollingBuffer s_lin, s_ang;   
                    static float t = 0.0f;
                    t += ImGui::GetIO().DeltaTime;

                    float linear_vel  = body->getLinearVelocity().length();  
                    float angular_vel = body->getAngularVelocity().length(); 

                    if (linear_vel == 0.0f && angular_vel == 0.0f) 
                    {
                        ImVec2 m = ImGui::GetMousePos();
                        linear_vel  = m.x * 0.0005f;
                        angular_vel = m.y * 0.0005f;
                    }

                    s_lin.AddPoint(t, linear_vel);
                    s_ang.AddPoint(t, angular_vel);

                    static float history = 10.0f;
                    ImGui::SliderFloat("History", &history, 1.0f, 30.0f, "%.1f s");

                    ImPlotAxisFlags axis_flags = ImPlotAxisFlags_NoTickLabels;
                    if (ImPlot::BeginPlot("##Scrolling_Velocities", ImVec2(-1, 220))) 
                    {
                        ImPlot::SetupAxes("Time (s)", "Velocity", axis_flags, 0);
                        ImPlot::SetupAxisLimits(ImAxis_X1, t - history, t, ImGuiCond_Always);

                        ImPlot::SetupLegend(ImPlotLocation_NorthWest, 0);
                        ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL, 0.35f);
                        if (!s_lin.Data.empty()) 
                        {
                            ImPlot::PlotLine(
                                "Linear (m/s)",
                                &s_lin.Data[0].x, &s_lin.Data[0].y,
                                (int)s_lin.Data.size(),
                                0, s_lin.Offset, 2 * sizeof(float)
                            );
                        }

                        if (!s_ang.Data.empty()) 
                        {
                            ImPlot::PlotLine(
                                "Angular (rad/s)",
                                &s_ang.Data[0].x, &s_ang.Data[0].y,
                                (int)s_ang.Data.size(),
                                0, s_ang.Offset, 2 * sizeof(float)
                            );
                        }

                        ImPlot::EndPlot();
                    }
                }
            }
        }
        else
        {
            ImGui::TextUnformatted(ICON_MD_INFO " Simulation is not active.");
        }
        
        ImGui::End();
    }
}