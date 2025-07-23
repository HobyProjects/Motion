#pragma once

#include "Event.hpp"
#include "KeyCodes.hpp"

namespace Motion
{
    template<typename T>
        requires std::is_enum_v<T>&& std::is_convertible_v<T, KeyCode>
    class EventKeyboardKeyPress : public IEvent
    {
    public:
        EventKeyboardKeyPress(T keyCode) : m_KeyCode(keyCode) {}
        virtual ~EventKeyboardKeyPress() = default;

        EVENT_CLASS_TYPE(EventType::KeyboardKeyPress);
        EVENT_CLASS_CATEGORY(EventCategory::Keyboard);

        T KeyCode() const { return m_KeyCode; }
        KeyState State() const { return KEY_PRESSED; }

    private:
        T m_KeyCode{ static_cast<T>(KeyCode::Unknown) };
    };

    template<typename T>
        requires std::is_enum_v<T>&& std::is_convertible_v<T, KeyCode>
    class EventKeyboardKeyRelease : public IEvent
    {
    public:
        EventKeyboardKeyRelease(T keyCode) : m_KeyCode(keyCode) {}
        virtual ~EventKeyboardKeyRelease() = default;

        EVENT_CLASS_TYPE(EventType::KeyboardKeyRelease);
        EVENT_CLASS_CATEGORY(EventCategory::Keyboard);

        T KeyCode() const { return m_KeyCode; }
        KeyState State() const { return KEY_RELEASED; }

    private:
        T m_KeyCode{ static_cast<T>(KeyCode::Unknown) };
    };

    template<typename T>
        requires std::is_enum_v<T>&& std::is_convertible_v<T, KeyCode>
    class EventKeyboardKeyRepeat : public IEvent
    {
    public:
        EventKeyboardKeyRepeat(T keyCode) : m_KeyCode(keyCode) {}
        virtual ~EventKeyboardKeyRepeat() = default;

        EVENT_CLASS_TYPE(EventType::KeyboardKeyRepeat);
        EVENT_CLASS_CATEGORY(EventCategory::Keyboard);

        T KeyCode() const { return m_KeyCode; }
        KeyState State() const { return KEY_REPEAT; }

    private:
        T m_KeyCode{ static_cast<T>(KeyCode::Unknown) };
    };

    class EventKeyboardKeyChar : public IEvent
    {
    public:
        EventKeyboardKeyChar(uint32_t character) : m_Character(character) {}
        virtual ~EventKeyboardKeyChar() = default;

        EVENT_CLASS_TYPE(EventType::KeyboardKeyChar);
        EVENT_CLASS_CATEGORY(EventCategory::Keyboard);

        uint32_t CodePoint() const { return m_Character; }
        KeyState State() const { return KEY_PRESSED; }

    private:
        uint32_t m_Character{ 0 };
    };

}