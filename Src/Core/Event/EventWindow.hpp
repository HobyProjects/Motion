#pragma once

#include "Event.hpp"

namespace Motion::Core
{
    class EventWindowClose final : public IEvent
    {
    public:
        EventWindowClose() = default;
        virtual ~EventWindowClose() = default;

        EVENT_CLASS_TYPE(EventType::WindowClose);
        EVENT_CLASS_CATEGORY(EventCategory::Window);
    };

    template<typename T>
        requires std::is_integral_v<T>
    class EventWindowResize final : public IEvent
    {
    public:
        EventWindowResize(T width, T height) : m_Width(width), m_Height(height) {}
        virtual ~EventWindowResize() = default;

        EVENT_CLASS_TYPE(EventType::WindowResize);
        EVENT_CLASS_CATEGORY(EventCategory::Window);

        T Width() const { return m_Width; }
        T Height() const { return m_Height; }

    private:
        T m_Width{ static_cast<T>(0) };
        T m_Height{ static_cast<T>(0) };
    };

    template<typename T>
        requires std::is_integral_v<T>
    class EventWindowPosChange final : public IEvent
    {
    public:
        EventWindowPosChange(T posX, T posY) :
            m_PosX(posX), m_PosY(posY) {
        }
        virtual ~EventWindowPosChange() = default;

        EVENT_CLASS_TYPE(EventType::WindowPosChange);
        EVENT_CLASS_CATEGORY(EventCategory::Window);

        T GetPosX() const { return m_PosX; }
        T GetPosY() const { return m_PosY; }

    private:
        T m_PosX{ static_cast<T>(0) };
        T m_PosY{ static_cast<T>(0) };
    };

    class EventWindowFocusGain final : public IEvent
    {
    public:
        EventWindowFocusGain() = default;
        virtual ~EventWindowFocusGain() = default;

        EVENT_CLASS_TYPE(EventType::WindowFocusGain);
        EVENT_CLASS_CATEGORY(EventCategory::Window);
    };

    class EventWindowFocusLost final : public IEvent
    {
    public:
        EventWindowFocusLost() = default;
        virtual ~EventWindowFocusLost() = default;

        EVENT_CLASS_TYPE(EventType::WindowFocusLost);
        EVENT_CLASS_CATEGORY(EventCategory::Window);
    };

    template<typename T>
        requires std::is_integral_v<T>
    class EventWindowFrameBufferSizeChange final : public IEvent
    {
    public:
        EventWindowFrameBufferSizeChange(T width, T height) :
            m_Width(width), m_Height(height) {
        }
        virtual ~EventWindowFrameBufferSizeChange() = default;

        EVENT_CLASS_TYPE(EventType::WindowResize);
        EVENT_CLASS_CATEGORY(EventCategory::Window);

        T Width() const { return m_Width; }
        T Height() const { return m_Height; }

    private:
        T m_Width{ static_cast<T>(0) };
        T m_Height{ static_cast<T>(0) };
    };

    class EventWindowMaximized final : public IEvent
    {
    public:
        EventWindowMaximized() = default;
        virtual ~EventWindowMaximized() = default;

        EVENT_CLASS_TYPE(EventType::WindowMaximize);
        EVENT_CLASS_CATEGORY(EventCategory::Window);
    };

    class EventWindowMinimized final : public IEvent
    {
    public:
        EventWindowMinimized() = default;
        virtual ~EventWindowMinimized() = default;

        EVENT_CLASS_TYPE(EventType::WindowMinimize);
        EVENT_CLASS_CATEGORY(EventCategory::Window);
    };
}