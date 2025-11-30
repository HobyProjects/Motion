#pragma once

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/imgui_stdlib.h>

namespace Motion
{
    class ResponsiveLayout 
    {
    public:
        struct Options 
        {
            float LabelWidthRatio = 0.35f;
            float MinLabelWidth = 100.0f;
            float MaxLabelWidth = 200.0f;
            float Spacing = 12.0f;
            bool TruncateLabel = true;
        };

        static void BeginLabelControl(const char* label, const Options& opts = {}) 
        {
            float availWidth = ImGui::GetContentRegionAvail().x;
            float labelWidth = availWidth * opts.LabelWidthRatio;
            labelWidth = ImClamp(labelWidth, opts.MinLabelWidth, opts.MaxLabelWidth);
            
            ImGui::AlignTextToFramePadding();
            if (opts.TruncateLabel)
            {
                RenderTruncatedText(label, labelWidth);
            } 
            else 
            {
                ImGui::Text("%s", label);
            }
            
            ImGui::SameLine(labelWidth + opts.Spacing);
            float controlWidth = availWidth - labelWidth - opts.Spacing;
            ImGui::SetNextItemWidth(controlWidth);
        }

    private:
        static void RenderTruncatedText(const char* text, float maxWidth) 
        {
            ImVec2 textSize = ImGui::CalcTextSize(text);
            
            if (textSize.x <= maxWidth) 
            {
                ImGui::Text("%s", text);
            } 
            else 
            {
                std::string truncated = text;
                const char* ellipsis = "...";
                float ellipsisWidth = ImGui::CalcTextSize(ellipsis).x;
                float availableWidth = maxWidth - ellipsisWidth;
                size_t len = truncated.length();
                while (len > 0) {
                    std::string test = truncated.substr(0, len);
                    if (ImGui::CalcTextSize(test.c_str()).x <= availableWidth) {
                        break;
                    }
                    len--;
                }
                
                truncated = truncated.substr(0, len) + ellipsis;
                ImGui::Text("%s", truncated.c_str());
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("%s", text);
                }
            }
        }
    };
}