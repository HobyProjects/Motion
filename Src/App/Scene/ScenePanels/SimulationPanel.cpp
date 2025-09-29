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
            if(entities.empty())
            {
                ImGui::TextDisabled(ICON_MD_INFO " No entities in scene");
                ImGui::End();
                return;
            }
            
            for(const auto& entt : entities)
            {
                std::shared_ptr<Entity> current = entt;
                std::shared_ptr<Entity> next = nullptr;

                while(current)
                {
                    if(current->Has<NodeComponent>())
                    {
                        next = current->Get<NodeComponent>().EnTTNext;
                        if(current->Get<NodeComponent>().IsRoot)
                        {
                            current = next;
                            continue;
                        }
                    }
                
                    auto& tag = current->Get<TagComponent>();
                    auto& rb  = current->Get<RigidBodyComponent>();

                    ImGui::PushID(current.get());
                    if(CollapsibleSection(tag.Tag.c_str(), false))
                    {
                        auto* body          = rb.PhysicsBody;

                        ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal, 1.5f);

                        static ScrollingBuffer s_lin, s_ang;   
                        static float t = 0.0f;
                        t += ImGui::GetIO().DeltaTime;

                        float linear_vel  = body->getLinearVelocity().length();  
                        float angular_vel = body->getAngularVelocity().length();
                        
                        s_lin.AddPoint(t, linear_vel);
                        s_ang.AddPoint(t, angular_vel);

                        static float history = 10.0f;
                        ImGui::SliderFloat("History", &history, 1.0f, 30.0f, "%.1f s");

                        ImPlotAxisFlags axis_flags = ImPlotAxisFlags_NoTickLabels;
                        if (ImPlot::BeginPlot("##Scrolling_Velocities", ImVec2(-1, 300))) 
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
                    ImGui::PopID();
                    current = next;
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