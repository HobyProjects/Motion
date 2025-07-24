#pragma once

#include "Event.hpp"

namespace Motion
{
    class EventWindowClose final : public IEvent
    {
    public:
        EventWindowClose() = default;
        virtual ~EventWindowClose() = default;

        EVENT_CLASS_TYPE(EventType::WindowClose);
        EVENT_CLASS_CATEGORY(EventCategory::Window);
    };

    class EventWindowResize final : public IEvent
    {
    public:
        EventWindowResize(std::int32_t width, std::int32_t height) : m_Width(width), m_Height(height) {}
        virtual ~EventWindowResize() = default;

        EVENT_CLASS_TYPE(EventType::WindowResize);
        EVENT_CLASS_CATEGORY(EventCategory::Window);

        std::int32_t Width() const { return m_Width; }
        std::int32_t Height() const { return m_Height; }

    private:
        std::int32_t m_Width{ 0 };
        std::int32_t m_Height{ 0 };
    };

    class EventWindowPosChange final : public IEvent
    {
    public:
        EventWindowPosChange(std::int32_t posX, std::int32_t posY) : m_PosX(posX), m_PosY(posY) {}
        virtual ~EventWindowPosChange() = default;

        EVENT_CLASS_TYPE(EventType::WindowPosChange);
        EVENT_CLASS_CATEGORY(EventCategory::Window);

        std::int32_t GetPosX() const { return m_PosX; }
        std::int32_t GetPosY() const { return m_PosY; }

    private:
        std::int32_t m_PosX{ 0 };
        std::int32_t m_PosY{ 0 };
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

    class EventWindowFrameBufferSizeChange final : public IEvent
    {
    public:
        EventWindowFrameBufferSizeChange(std::int32_t width, std::int32_t height) : m_Width(width), m_Height(height) {}
        virtual ~EventWindowFrameBufferSizeChange() = default;

        EVENT_CLASS_TYPE(EventType::WindowResize);
        EVENT_CLASS_CATEGORY(EventCategory::Window);

        std::int32_t Width() const { return m_Width; }
        std::int32_t Height() const { return m_Height; }

    private:
        std::int32_t m_Width{ 0 };
        std::int32_t m_Height{ 0 };
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