#pragma once

#include <string>
#include <vector>
#include <functional>
#include <algorithm>

#include <imgui/imgui.h>
#include <imgui/implot.h>
#include <glm/glm.hpp>

#include "Responsive.hpp"
#include "Scope.hpp"

namespace Motion
{
    struct PlotAxisConfig
    {
        bool AutoFit{true};
        bool LockMin{false};
        bool LockMax{false};
        bool Invert{false};
        bool NoGridLines{false};
        bool NoTickMarks{false};
        bool NoTickLabels{false};
        double Min{0.0};
        double Max{1.0};
        const char* Label{nullptr};
        const char* Format{nullptr};

        ImPlotAxisFlags GetFlags() const
        {
            ImPlotAxisFlags flags = ImPlotAxisFlags_None;
            
            if(AutoFit)         flags |= ImPlotAxisFlags_AutoFit;
            if(LockMin)         flags |= ImPlotAxisFlags_LockMin;
            if(LockMax)         flags |= ImPlotAxisFlags_LockMax;
            if(Invert)          flags |= ImPlotAxisFlags_Invert;
            if(NoGridLines)     flags |= ImPlotAxisFlags_NoGridLines;
            if(NoTickMarks)     flags |= ImPlotAxisFlags_NoTickMarks;
            if(NoTickLabels)    flags |= ImPlotAxisFlags_NoTickLabels;
            
            return flags;
        }
    };

    struct PlotConfig
    {
        ImVec2 Size{-1, 300};
        bool NoTitle{false};
        bool NoLegend{false};
        bool NoMenus{false};
        bool NoBoxSelect{false};
        bool Equal{false};
        bool Crosshairs{false};
        bool CanvasOnly{false};
        
        PlotAxisConfig XAxis{};
        PlotAxisConfig YAxis{};
        
        ResponsiveLayout::Options Layout{};

        ImPlotFlags GetFlags() const
        {
            ImPlotFlags flags = ImPlotFlags_None;
            
            if(NoTitle)         flags |= ImPlotFlags_NoTitle;
            if(NoLegend)        flags |= ImPlotFlags_NoLegend;
            if(NoMenus)         flags |= ImPlotFlags_NoMenus;
            if(NoBoxSelect)     flags |= ImPlotFlags_NoBoxSelect;
            if(Equal)           flags |= ImPlotFlags_Equal;
            if(Crosshairs)      flags |= ImPlotFlags_Crosshairs;
            if(CanvasOnly)      flags |= ImPlotFlags_CanvasOnly;
            
            return flags;
        }
    };

    struct PlotLineConfig
    {
        ImVec4 Color{0, 0, 0, -1}; // -1 alpha means use default color
        float Thickness{1.0f};
        bool Shaded{false};
        bool Stairs{false};
        bool Bars{false};
        bool Stems{false};
        bool Scatter{false};
        ImPlotMarker MarkerStyle{ImPlotMarker_None};
        float MarkerSize{4.0f};
    };

    class ScopedPlotStyle
    {
    public:
        ScopedPlotStyle(ImPlotStyleVar idx, float val) 
        { 
            ImPlot::PushStyleVar(idx, val); 
        }
        
        ScopedPlotStyle(ImPlotStyleVar idx, int val) 
        { 
            ImPlot::PushStyleVar(idx, val); 
        }
        
        ScopedPlotStyle(ImPlotStyleVar idx, const ImVec2& val) 
        { 
            ImPlot::PushStyleVar(idx, val); 
        }
        
        ~ScopedPlotStyle() 
        { 
            ImPlot::PopStyleVar(); 
        }

        ScopedPlotStyle(const ScopedPlotStyle&) = delete;
        ScopedPlotStyle& operator=(const ScopedPlotStyle&) = delete;
    };

    class ScopedPlotColor
    {
    public:
        ScopedPlotColor(ImPlotCol idx, const ImVec4& col) 
        { 
            ImPlot::PushStyleColor(idx, col); 
        }
        
        ScopedPlotColor(ImPlotCol idx, ImU32 col) 
        { 
            ImPlot::PushStyleColor(idx, col); 
        }
        
        ~ScopedPlotColor() 
        { 
            ImPlot::PopStyleColor(); 
        }

        ScopedPlotColor(const ScopedPlotColor&) = delete;
        ScopedPlotColor& operator=(const ScopedPlotColor&) = delete;
    };

    struct PlotData
    {
        std::vector<float> X;
        std::vector<float> Y;

        std::string Label;
        PlotLineConfig Style;

        void Reserve(size_t size)
        {
            X.reserve(size);
            Y.reserve(size);
        }

        void Clear()
        {
            X.clear();
            Y.clear();
        }

        void AddPoint(float x, float y)
        {
            X.push_back(x);
            Y.push_back(y);
        }

        size_t Size() const
        {
            return std::min(X.size(), Y.size());
        }

        bool Empty() const
        {
            return X.empty() || Y.empty();
        }
    };

    struct PlotBuffer
    {
        std::vector<float> Data;
        size_t MaxSize;
        size_t Offset{0};

        explicit PlotBuffer(size_t maxSize = 1000) : MaxSize(maxSize)
        {
            Data.reserve(maxSize);
        }

        void AddPoint(float value)
        {
            if(Data.size() < MaxSize)
            {
                Data.push_back(value);
            }
            else
            {
                Data[Offset] = value;
                Offset = (Offset + 1) % MaxSize;
            }
        }

        void Clear()
        {
            Data.clear();
            Offset = 0;
        }

        size_t Size() const { return Data.size(); }
        bool Empty() const { return Data.empty(); }
        bool IsFull() const { return Data.size() >= MaxSize; }
    };

    inline bool BeginPlot(const std::string& label, const PlotConfig& config = {})
    {
        ScopeID id(label);
        return ImPlot::BeginPlot(label.c_str(), config.Size, config.GetFlags());
    }

    inline void EndPlot()
    {
        ImPlot::EndPlot();
    }

    inline void SetupPlotAxes(const PlotConfig& config)
    {
        ImPlot::SetupAxis(ImAxis_X1, config.XAxis.Label, config.XAxis.GetFlags());
        ImPlot::SetupAxis(ImAxis_Y1, config.YAxis.Label, config.YAxis.GetFlags());
        
        if(!config.XAxis.AutoFit)
            ImPlot::SetupAxisLimits(ImAxis_X1, config.XAxis.Min, config.XAxis.Max);
        
        if(!config.YAxis.AutoFit)
            ImPlot::SetupAxisLimits(ImAxis_Y1, config.YAxis.Min, config.YAxis.Max);
        
        if(config.XAxis.Format)
            ImPlot::SetupAxisFormat(ImAxis_X1, config.XAxis.Format);
        
        if(config.YAxis.Format)
            ImPlot::SetupAxisFormat(ImAxis_Y1, config.YAxis.Format);
    }

    inline void PlotLine(const char* label, const float* xs, const float* ys, int count, const PlotLineConfig& config = {})
    {
        if(config.Color.w >= 0)
        {
            ScopedPlotColor color(ImPlotCol_Line, config.Color);
            ScopedPlotStyle thickness(ImPlotStyleVar_LineWeight, config.Thickness);
            
            if(config.Shaded)
                ImPlot::PlotShaded(label, xs, ys, count);
            else if(config.Stairs)
                ImPlot::PlotStairs(label, xs, ys, count);
            else if(config.Bars)
                ImPlot::PlotBars(label, xs, ys, count, 0.67);
            else if(config.Stems)
                ImPlot::PlotStems(label, xs, ys, count);
            else if(config.Scatter)
            {
                ScopedPlotStyle marker(ImPlotStyleVar_Marker, static_cast<int>(config.MarkerStyle));
                ScopedPlotStyle markerSize(ImPlotStyleVar_MarkerSize, config.MarkerSize);
                ImPlot::PlotScatter(label, xs, ys, count);
            }
            else
            {
                if(config.MarkerStyle != ImPlotMarker_None)
                {
                    ScopedPlotStyle marker(ImPlotStyleVar_Marker, static_cast<int>(config.MarkerStyle));
                    ScopedPlotStyle markerSize(ImPlotStyleVar_MarkerSize, config.MarkerSize);
                    ImPlot::PlotLine(label, xs, ys, count);
                }
                else
                {
                    ImPlot::PlotLine(label, xs, ys, count);
                }
            }
        }
        else
        {
            ScopedPlotStyle thickness(ImPlotStyleVar_LineWeight, config.Thickness);
            
            if(config.Shaded)
                ImPlot::PlotShaded(label, xs, ys, count);
            else if(config.Stairs)
                ImPlot::PlotStairs(label, xs, ys, count);
            else if(config.Bars)
                ImPlot::PlotBars(label, xs, ys, count, 0.67);
            else if(config.Stems)
                ImPlot::PlotStems(label, xs, ys, count);
            else if(config.Scatter)
            {
                ScopedPlotStyle marker(ImPlotStyleVar_Marker, static_cast<int>(config.MarkerStyle));
                ScopedPlotStyle markerSize(ImPlotStyleVar_MarkerSize, config.MarkerSize);
                ImPlot::PlotScatter(label, xs, ys, count);
            }
            else
            {
                if(config.MarkerStyle != ImPlotMarker_None)
                {
                    ScopedPlotStyle marker(ImPlotStyleVar_Marker, static_cast<int>(config.MarkerStyle));
                    ScopedPlotStyle markerSize(ImPlotStyleVar_MarkerSize, config.MarkerSize);
                    ImPlot::PlotLine(label, xs, ys, count);
                }
                else
                {
                    ImPlot::PlotLine(label, xs, ys, count);
                }
            }
        }
    }

    inline void PlotLine(const PlotData& data)
    {
        if(!data.Empty())
            PlotLine(data.Label.c_str(), data.X.data(), data.Y.data(), static_cast<int>(data.Size()), data.Style);
    }

    inline void PlotLine(const char* label, const std::vector<float>& xs, const std::vector<float>& ys, const PlotLineConfig& config = {})
    {
        if(!xs.empty() && !ys.empty())
        {
            int count = static_cast<int>(std::min(xs.size(), ys.size()));
            PlotLine(label, xs.data(), ys.data(), count, config);
        }
    }

    inline void PlotLine(const char* label, const std::vector<glm::vec2>& points, const PlotLineConfig& config = {})
    {
        if(!points.empty())
        {
            std::vector<float> xs, ys;
            xs.reserve(points.size());
            ys.reserve(points.size());
            
            for(const auto& p : points)
            {
                xs.push_back(p.x);
                ys.push_back(p.y);
            }
            
            PlotLine(label, xs.data(), ys.data(), static_cast<int>(points.size()), config);
        }
    }

    inline void PlotBufferLine(const char* label, const PlotBuffer& buffer, const PlotLineConfig& config = {})
    {
        if(!buffer.Empty())
        {
            std::vector<float> xs(buffer.Size());
            for(size_t i = 0; i < buffer.Size(); ++i)
                xs[i] = static_cast<float>(i);
            
            PlotLine(label, xs.data(), buffer.Data.data(), static_cast<int>(buffer.Size()), config);
        }
    }

    inline void PlotHLine(const char* label, double y, const PlotLineConfig& config = {})
    {
        if(config.Color.w >= 0)
        {
            ScopedPlotColor color(ImPlotCol_Line, config.Color);
            ScopedPlotStyle thickness(ImPlotStyleVar_LineWeight, config.Thickness);
            ImPlot::PlotInfLines(label, &y, 1);
        }
        else
        {
            ScopedPlotStyle thickness(ImPlotStyleVar_LineWeight, config.Thickness);
            ImPlot::PlotInfLines(label, &y, 1);
        }
    }

    inline void PlotVLine(const char* label, double x, const PlotLineConfig& config = {})
    {
        if(config.Color.w >= 0)
        {
            ScopedPlotColor color(ImPlotCol_Line, config.Color);
            ScopedPlotStyle thickness(ImPlotStyleVar_LineWeight, config.Thickness);
            ImPlot::PlotInfLines(label, &x, 1, ImPlotInfLinesFlags_Horizontal);
        }
        else
        {
            ScopedPlotStyle thickness(ImPlotStyleVar_LineWeight, config.Thickness);
            ImPlot::PlotInfLines(label, &x, 1, ImPlotInfLinesFlags_Horizontal);
        }
    }

    inline void PlotText(const char* text, double x, double y, const ImVec2& pixel_offset = ImVec2(0, 0))
    {
        ImPlot::PlotText(text, x, y, pixel_offset);
    }

    class PlotBuilder
    {
    public:
        explicit PlotBuilder(const std::string& title) : m_Title(title) {}

        PlotBuilder& Size(const ImVec2& size) 
        { 
            m_Config.Size = size; 
            return *this; 
        }

        PlotBuilder& XAxisLabel(const char* label) 
        { 
            m_Config.XAxis.Label = label; 
            return *this; 
        }

        PlotBuilder& YAxisLabel(const char* label) 
        { 
            m_Config.YAxis.Label = label; 
            return *this; 
        }

        PlotBuilder& XAxisRange(double min, double max) 
        { 
            m_Config.XAxis.Min = min;
            m_Config.XAxis.Max = max;
            m_Config.XAxis.AutoFit = false;
            return *this; 
        }

        PlotBuilder& YAxisRange(double min, double max) 
        { 
            m_Config.YAxis.Min = min;
            m_Config.YAxis.Max = max;
            m_Config.YAxis.AutoFit = false;
            return *this; 
        }

        PlotBuilder& AutoFit() 
        { 
            m_Config.XAxis.AutoFit = true;
            m_Config.YAxis.AutoFit = true;
            return *this; 
        }

        PlotBuilder& NoLegend() 
        { 
            m_Config.NoLegend = true; 
            return *this; 
        }

        PlotBuilder& Crosshairs() 
        { 
            m_Config.Crosshairs = true; 
            return *this; 
        }

        PlotBuilder& Equal() 
        { 
            m_Config.Equal = true; 
            return *this; 
        }

        PlotBuilder& AddLine(const PlotData& data)
        {
            m_Data.push_back(data);
            return *this;
        }

        PlotBuilder& AddLine(const std::string& label, const std::vector<float>& xs, const std::vector<float>& ys, const PlotLineConfig& style = {})
        {
            PlotData data;
            data.Label = label;
            data.X = xs;
            data.Y = ys;
            data.Style = style;
            m_Data.push_back(data);
            return *this;
        }

        void Render()
        {
            if(BeginPlot(m_Title, m_Config))
            {
                SetupPlotAxes(m_Config);
                
                for(const auto& data : m_Data)
                    PlotLine(data);
                
                EndPlot();
            }
        }

    private:
        std::string m_Title;
        PlotConfig m_Config;
        std::vector<PlotData> m_Data;
    };

    inline void SimplePlot(const std::string& label, const std::vector<float>& values, const PlotConfig& config = {})
    {
        if(values.empty()) return;

        if(BeginPlot(label, config))
        {
            SetupPlotAxes(config);
            
            std::vector<float> xs(values.size());
            for(size_t i = 0; i < values.size(); ++i)
                xs[i] = static_cast<float>(i);
            
            ImPlot::PlotLine(label.c_str(), xs.data(), values.data(), static_cast<int>(values.size()));
            
            EndPlot();
        }
    }

    inline void FunctionPlot(const std::string& label, std::function<float(float)> func, float xMin, float xMax, int samples = 1000, const PlotConfig& config = {})
    {
        std::vector<float> xs(samples);
        std::vector<float> ys(samples);
        
        float step = (xMax - xMin) / (samples - 1);
        for(int i = 0; i < samples; ++i)
        {
            xs[i] = xMin + i * step;
            ys[i] = func(xs[i]);
        }

        if(BeginPlot(label, config))
        {
            SetupPlotAxes(config);
            ImPlot::PlotLine(label.c_str(), xs.data(), ys.data(), samples);
            EndPlot();
        }
    }

    inline void ScatterPlot(const std::string& label, const std::vector<glm::vec2>& points, const PlotConfig& config = {})
    {
        if(points.empty()) return;

        std::vector<float> xs, ys;
        xs.reserve(points.size());
        ys.reserve(points.size());
        
        for(const auto& p : points)
        {
            xs.push_back(p.x);
            ys.push_back(p.y);
        }

        if(BeginPlot(label, config))
        {
            SetupPlotAxes(config);
            
            ScopedPlotStyle marker(ImPlotStyleVar_Marker, static_cast<int>(ImPlotMarker_Circle));
            ImPlot::PlotScatter(label.c_str(), xs.data(), ys.data(), static_cast<int>(points.size()));
            
            EndPlot();
        }
    }

    inline void HistogramPlot(const std::string& label, const std::vector<float>& values, int bins = 20, const PlotConfig& config = {})
    {
        if(values.empty()) return;

        if(BeginPlot(label, config))
        {
            SetupPlotAxes(config);
            ImPlot::PlotHistogram(label.c_str(), values.data(), static_cast<int>(values.size()), bins);
            EndPlot();
        }
    }

    class RealTimePlot
    {
    public:
        RealTimePlot(const std::string& title, size_t maxPoints = 1000, float history = 10.0f)
            : m_Title(title), m_MaxPoints(maxPoints), m_History(history)
        {
            m_Buffer.reserve(maxPoints);
        }

        void AddPoint(float value)
        {
            float t = static_cast<float>(ImGui::GetTime());
            m_Buffer.push_back({t, value});
            
            while(!m_Buffer.empty() && t - m_Buffer.front().x > m_History)
                m_Buffer.erase(m_Buffer.begin());
            
            if(m_Buffer.size() > m_MaxPoints)
                m_Buffer.erase(m_Buffer.begin());
        }

        void Clear()
        {
            m_Buffer.clear();
        }

        void Render(const PlotConfig& config = {})
        {
            if(m_Buffer.empty()) return;

            PlotConfig rtConfig = config;
            rtConfig.XAxis.AutoFit = false;
            rtConfig.YAxis.AutoFit = true;
            
            float t = static_cast<float>(ImGui::GetTime());
            rtConfig.XAxis.Min = t - m_History;
            rtConfig.XAxis.Max = t;

            if(BeginPlot(m_Title, rtConfig))
            {
                SetupPlotAxes(rtConfig);
                
                std::vector<float> xs, ys;
                xs.reserve(m_Buffer.size());
                ys.reserve(m_Buffer.size());
                
                for(const auto& p : m_Buffer)
                {
                    xs.push_back(p.x);
                    ys.push_back(p.y);
                }
                
                ImPlot::PlotLine(m_Title.c_str(), xs.data(), ys.data(), static_cast<int>(m_Buffer.size()));
                
                EndPlot();
            }
        }

        size_t Size() const { return m_Buffer.size(); }
        bool Empty() const { return m_Buffer.empty(); }

    private:
        std::string m_Title;
        size_t m_MaxPoints;
        float m_History;
        std::vector<glm::vec2> m_Buffer;
    };
}