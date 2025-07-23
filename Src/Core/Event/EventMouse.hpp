#pragma once

#include "Event.hpp"
#include "KeyCodes.hpp"

namespace Motion
{
    template<typename T>
        requires std::is_enum_v<T>&& std::is_constructible_v<T, MouseButton>
    class EventMouseButtonDown : public IEvent
    {
    public:
        EventMouseButtonDown(T button) : m_Button(button) {}
        virtual ~EventMouseButtonDown() = default;

        EVENT_CLASS_TYPE(EventType::MouseButtonDown);
        EVENT_CLASS_CATEGORY(EventCategory::Mouse);

        T Button() const { return m_Button; }
        MouseButtonState State() const { return MOUSE_BUTTON_PRESSED; }

    private:
        T m_Button{ static_cast<T>(MOUSE_BUTTON_NONE) };
    };

    template<typename T>
        requires std::is_enum_v<T>&& std::is_constructible_v<T, MouseButton>
    class EventMouseButtonUp : public IEvent
    {
    public:
        EventMouseButtonUp(T button) : m_Button(button) {}
        virtual ~EventMouseButtonUp() = default;

        EVENT_CLASS_TYPE(EventType::MouseButtonUp);
        EVENT_CLASS_CATEGORY(EventCategory::Mouse);

        T Button() const { return m_Button; }
        MouseButtonState State() const { return MOUSE_BUTTON_RELEASED; }

    private:
        T m_Button{ static_cast<T>(MOUSE_BUTTON_NONE) };
    };

    template<typename T>
        requires std::is_integral_v<T> || std::is_floating_point_v<T>
    class EventMouseWheelScroll : public IEvent
    {
    public:
        EventMouseWheelScroll(T offsetX, T offsetY) : m_OffsetX(offsetX), m_OffsetY(offsetY) {}
        virtual ~EventMouseWheelScroll() = default;

        EVENT_CLASS_TYPE(EventType::MouseWheelScroll);
        EVENT_CLASS_CATEGORY(EventCategory::Mouse);

        T OffsetX() const { return m_OffsetX; }
        T OffsetY() const { return m_OffsetY; }

    private:
        T m_OffsetX{}, m_OffsetY{};
    };

    template<typename T>
        requires std::is_integral_v<T> || std::is_floating_point_v<T>
    class EventMouseCursorMove : public IEvent
    {
    public:
        EventMouseCursorMove(T x, T y) : m_X(x), m_Y(y) {}
        virtual ~EventMouseCursorMove() = default;

        EVENT_CLASS_TYPE(EventType::MouseCursorPosChange);
        EVENT_CLASS_CATEGORY(EventCategory::Mouse);

        T GetX() const { return m_X; }
        T GetY() const { return m_Y; }

    private:
        T m_X{}, m_Y{};
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