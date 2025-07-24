#pragma once

#include "Event.hpp"
#include "KeyCodes.hpp"

namespace Motion
{
    class EventMouseButtonDown : public IEvent
    {
    public:
        EventMouseButtonDown(MouseButton button) : m_Button(button) {}
        virtual ~EventMouseButtonDown() = default;

        EVENT_CLASS_TYPE(EventType::MouseButtonDown);
        EVENT_CLASS_CATEGORY(EventCategory::Mouse);

        MouseButton Button() const { return m_Button; }

    private:
        MouseButton m_Button{ MOUSE_BUTTON_UNKNOWN };
    };

    class EventMouseButtonUp : public IEvent
    {
    public:
        EventMouseButtonUp(MouseButton button) : m_Button(button) {}
        virtual ~EventMouseButtonUp() = default;

        EVENT_CLASS_TYPE(EventType::MouseButtonUp);
        EVENT_CLASS_CATEGORY(EventCategory::Mouse);

        MouseButton Button() const { return m_Button; }

    private:
        MouseButton m_Button{ MOUSE_BUTTON_UNKNOWN };
    };

    class EventMouseWheelScroll : public IEvent
    {
    public:
        EventMouseWheelScroll(double offsetX, double offsetY) : m_OffsetX(offsetX), m_OffsetY(offsetY) {}
        virtual ~EventMouseWheelScroll() = default;

        EVENT_CLASS_TYPE(EventType::MouseWheelScroll);
        EVENT_CLASS_CATEGORY(EventCategory::Mouse);

        double OffsetX() const { return m_OffsetX; }
        double OffsetY() const { return m_OffsetY; }

    private:
        double m_OffsetX{ 0.0 }, m_OffsetY{ 0.0 };
    };

    class EventMouseCursorMove : public IEvent
    {
    public:
        EventMouseCursorMove(double x, double y) : m_X(x), m_Y(y) {}
        virtual ~EventMouseCursorMove() = default;

        EVENT_CLASS_TYPE(EventType::MouseCursorPosChange);
        EVENT_CLASS_CATEGORY(EventCategory::Mouse);

        double GetX() const { return m_X; }
        double GetY() const { return m_Y; }

    private:
        double m_X{ 0.0 }, m_Y{ 0.0 };
    };

    class EventMouseCursorWindowEnter : public IEvent
    {
    public:
        EventMouseCursorWindowEnter() = default;
        virtual ~EventMouseCursorWindowEnter() = default;

        EVENT_CLASS_TYPE(EventType::MouseCursorWindowEnter);
        EVENT_CLASS_CATEGORY(EventCategory::Mouse);
    };

    class EventMouseCursorWindowLeave : public IEvent
    {
    public:
        EventMouseCursorWindowLeave() = default;
        virtual ~EventMouseCursorWindowLeave() = default;

        EVENT_CLASS_TYPE(EventType::MouseCursorWindowLeave);
        EVENT_CLASS_CATEGORY(EventCategory::Mouse);
    };
}