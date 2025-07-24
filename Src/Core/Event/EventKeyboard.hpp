#pragma once

#include "Event.hpp"
#include "KeyCodes.hpp"

namespace Motion
{
    class EventKeyboardKeyPress : public IEvent
    {
    public:
        EventKeyboardKeyPress(KeyCode keyCode) : m_KeyCode(keyCode) {}
        virtual ~EventKeyboardKeyPress() = default;

        EVENT_CLASS_TYPE(EventType::KeyboardKeyPress);
        EVENT_CLASS_CATEGORY(EventCategory::Keyboard);

        KeyCode Key() const { return m_KeyCode; }

    private:
        KeyCode m_KeyCode{ KEY_UNKNOWN };
    };

    class EventKeyboardKeyRelease : public IEvent
    {
    public:
        EventKeyboardKeyRelease(KeyCode keyCode) : m_KeyCode(keyCode) {}
        virtual ~EventKeyboardKeyRelease() = default;

        EVENT_CLASS_TYPE(EventType::KeyboardKeyRelease);
        EVENT_CLASS_CATEGORY(EventCategory::Keyboard);

        KeyCode Key() const { return m_KeyCode; }

    private:
        KeyCode m_KeyCode{ KEY_UNKNOWN };
    };

    class EventKeyboardKeyRepeat : public IEvent
    {
    public:
        EventKeyboardKeyRepeat(KeyCode keyCode) : m_KeyCode(keyCode) {}
        virtual ~EventKeyboardKeyRepeat() = default;

        EVENT_CLASS_TYPE(EventType::KeyboardKeyRepeat);
        EVENT_CLASS_CATEGORY(EventCategory::Keyboard);

        KeyCode Key() const { return m_KeyCode; }

    private:
        KeyCode m_KeyCode{ KEY_UNKNOWN };
    };

    class EventKeyboardKeyChar : public IEvent
    {
    public:
        EventKeyboardKeyChar(uint32_t character) : m_Character(character) {}
        virtual ~EventKeyboardKeyChar() = default;

        EVENT_CLASS_TYPE(EventType::KeyboardKeyChar);
        EVENT_CLASS_CATEGORY(EventCategory::Keyboard);

        uint32_t CodePoint() const { return m_Character; }

    private:
        uint32_t m_Character{ 0 };
    };

};