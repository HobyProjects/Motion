#pragma once

#include <string_view>
#include <functional>
#include <type_traits>

#include "Base.hpp"

namespace Motion::Core
{
    enum class EventCategory : uint32_t 
    {
        Window = Bits<0>::value,
        Keyboard = Bits<1>::value,
        Mouse = Bits<2>::value,
        GamePad = Bits<3>::value,
        Unknown = Bits<4>::value
    };

    inline uint32_t operator|(EventCategory a, EventCategory b) { return static_cast<uint32_t>(a) | static_cast<uint32_t>(b); }
    inline uint32_t operator&(EventCategory a, EventCategory b) { return static_cast<uint32_t>(a) & static_cast<uint32_t>(b); }
    inline uint32_t operator^(EventCategory a, EventCategory b) { return static_cast<uint32_t>(a) ^ static_cast<uint32_t>(b); }
    inline uint32_t operator~(EventCategory a) { return ~static_cast<uint32_t>(a); }

    enum class EventType : uint32_t
    {
        WindowClose = Bits<0>::value,
        WindowResize = Bits<1>::value,
        WindowPosChange = Bits<2>::value,
        WindowFocusGain = Bits<3>::value,
        WindowFocusLost = Bits<4>::value,
        WindowFrameBufferSizeChange = Bits<5>::value,
        WindowMaximize = Bits<6>::value,
        WindowMinimize = Bits<7>::value,

        KeyboardKeyPress = Bits<8>::value,
        KeyboardKeyRelease = Bits<9>::value,
        KeyboardKeyRepeate = Bits<10>::value,
        KeybaordKeyChar = Bits<11>::value,

        MouseButtonDown = Bits<12>::value,
        MouseButtonUp = Bits<13>::value,
        MouseWheelScroll = Bits<14>::value,
        MouseCursorPosChange = Bits<15>::value,
        MouseCursorWindowEnter = Bits<16>::value,
        MouseCursorWindowLeave = Bits<17>::value
    };

    inline uint32_t operator|(EventType a, EventType b) { return static_cast<uint32_t>(a) | static_cast<uint32_t>(b); }
    inline uint32_t operator&(EventType a, EventType b) { return static_cast<uint32_t>(a) & static_cast<uint32_t>(b); }
    inline uint32_t operator^(EventType a, EventType b) { return static_cast<uint32_t>(a) ^ static_cast<uint32_t>(b); }
    inline uint32_t operator~(EventType a) { return ~static_cast<uint32_t>(a); }

    #define EVENT_CLASS_TYPE(EVENT_TYPE) static EventType StaticType() { return EVENT_TYPE; }\
        virtual EventType Type() const override { return StaticType(); }\
        virtual std::string_view What() const override { return std::string_view(#EVENT_TYPE); }

    #define EVENT_CLASS_CATEGORY(EVENT_CATEGORY) virtual EventCategory Category() const override { return EVENT_CATEGORY; }\
        virtual bool Equals(EventCategory category) const override { return static_cast<bool>(Category() == category); }

    class IEvent
    {
        public:
            IEvent() = default;
            virtual ~IEvent() = default;

            virtual EventType Type() const = 0;
            virtual std::string_view What() const = 0;
            virtual EventCategory Category() const = 0;
            virtual bool Equals(EventCategory category) const = 0;
    };

    class EventHandler
    {
        public:
            EventHandler(uint32_t windowHandle, IEvent& eventRef):
                m_Event(eventRef), m_WinHandle(windowHandle){}
            ~EventHandler() = default;

            template<typename TEvent>
            bool Dispatch(const std::function<bool(uint32_t, TEvent&)>& callBackFun)
            {
                static_assert(std::is_base_of_v<IEvent, TEvent>, "TEvent must be a subclass of IEvent!");
                if(m_Event.Type() == TEvent::StaticType())
                {
                    m_IsEventHandled |= callBackFun(m_WinHandle, static_cast<TEvent&>(m_Event));
                    return m_IsEventHandled;
                }

                return false;
            }

            bool IsHandled() const { return m_IsEventHandled; }
            uint32_t GetWindowHandle() const { return m_WinHandle; }

        private:
            IEvent& m_Event;
            uint32_t m_WinHandle{0};
            bool m_IsEventHandled{false};
    };

    template<typename... Args>
    class EventRegistry
    {
        private:
            EventRegistry() = default;
            ~EventRegistry() = default;

            EventRegistry(const EventRegistry&) = delete;
            EventRegistry& operator=(const EventRegistry&) = delete;
            EventRegistry(EventRegistry&&) = delete;
            EventRegistry& operator=(EventRegistry&&) = delete;

        public:
            using EventRegistryCallbackFunction = std::function<void(Args...)>;

            static EventRegistry& GetInstance()
            {
                static EventRegistry instance;
                return instance;
            }

            static void Register(EventType eventType, EventRegistryCallbackFunction callback)
            {
                auto& instance = GetInstance();
                auto it = instance.m_EventRegistry.find(eventType);
                if(it != instance.m_EventRegistry.end())
                {
                    it->second = callback;
                }
                else
                {
                    instance.m_EventRegistry.emplace(eventType, callback);
                }
            }

            static void Unregister(EventType eventType)
            {
                auto& instance = GetInstance();
                auto it = instance.m_EventRegistry.find(eventType);
                if(it != instance.m_EventRegistry.end())
                {
                    instance.m_EventRegistry.erase(it);
                }
            }

            static void Invoke(EventType eventType, Args... args)
            {
                auto& instance = GetInstance();
                auto it = instance.m_EventRegistry.find(eventType);
                if(it != instance.m_EventRegistry.end())
                {
                    it->second(std::forward<Args>(args)...);
                }
            }

            static EventRegistryCallbackFunction Get(EventType eventType)
            {
                auto& instance = GetInstance();
                auto it = instance.m_EventRegistry.find(eventType);
                if(it != instance.m_EventRegistry.end())
                {
                    return it->second;
                }
                return nullptr;
            }

            static void Clear()
            {
                auto& instance = GetInstance();
                instance.m_EventRegistry.clear();
            }

        private:
            static inline std::unordered_map<EventType, EventRegistryCallbackFunction> m_EventRegistry;
    };


    using ApplicationCallbackFunction = std::function<void(uint64_t, IEvent&)>;
    #define EVENT_CALLBACK(CALLBACK_FUNC) [this](auto&&... args) -> decltype(auto) { return this->CALLBACK_FUNC(std::forward<decltype(args)>(args)...); }
}