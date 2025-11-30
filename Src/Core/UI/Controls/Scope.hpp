#pragma once

#include <string>
#include <format>
#include <cstdint>
#include <imgui/imgui.h>

namespace Motion
{
    class ScopeID
    {
    public:
        explicit ScopeID(const char* id) 
        { 
            ImGui::PushID(id); 
            m_StringID = id;
        }

        explicit ScopeID(const std::string& id) 
        { 
            ImGui::PushID(id.c_str()); 
            m_StringID = id;
        }

        explicit ScopeID(int id) 
        { 
            ImGui::PushID(id); 
            m_IntegerID = id;
        }

        explicit ScopeID(const void* id) 
        { 
            ImGui::PushID(id);
            m_IntegerID = reinterpret_cast<std::uintptr_t>(id); 
        }

        template<typename... Args>
        explicit ScopeID(std::format_string<Args...> fmt, Args&&... args)
        {
            std::string fmtID = std::format(fmt, std::forward<Args>(args)...);
            ImGui::PushID(fmtID.c_str());
            m_StringID = fmtID;
        }

        ~ScopeID() 
        { 
            ImGui::PopID(); 
        }

        std::string GetStringID() const { return m_StringID; }
        std::int32_t GetIntegerID() const { return m_IntegerID; }

        ScopeID(const ScopeID&) = delete;
        ScopeID& operator=(const ScopeID&) = delete;
        ScopeID(ScopeID&&) = delete;
        ScopeID& operator=(ScopeID&&) = delete;

    private:
        std::string m_StringID{};
        std::int32_t m_IntegerID{};
    };
}