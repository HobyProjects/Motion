#pragma once

#include <string_view>
#include <functional>
#include <type_traits>

#include "Base.hpp"
#include "UUID.hpp"

#define EVENT_CALLBACK(CALLBACK_FUNC) [this](auto&&... args) -> decltype(auto) { return this->CALLBACK_FUNC(std::forward<decltype(args)>(args)...); }

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
        KeyboardKeyRepeat = Bits<10>::value,
        KeyboardKeyChar = Bits<11>::value,

        MouseButtonDown = Bits<12>::value,
        MouseButtonUp = Bits<13>::value,
        MouseWheelScroll = Bits<14>::value,
        MouseCursorPosChange = Bits<15>::value,
        MouseCursorWindowEnter = Bits<16>::value,
        MouseCursorWindowLeave = Bits<17>::value
    };

    inline uint32_t operator|(EventType a, EventType b) { return static_cast<uint32_t>(a) | static_cast<uint32_t>(b); }
    inline uint32_t operator&(EventType a, EventType b) { return static_cast<uint32_t>(a) & static_cast<uint32_t>(b); }

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
        EventHandler(UUID windowHandle, IEvent& eventRef) : m_Event(eventRef), m_WinHandle(windowHandle) {}
        ~EventHandler() = default;

        /**
         * @brief Dispatches an event to the provided callback function if the event type matches.
         *
         * This function checks if the stored event's type matches the static type of the specified TEvent.
         * If the types match, it invokes the provided callback function with the window handle and the event,
         * and updates the handled state accordingly.
         *
         * @tparam TEvent The type of event to dispatch. Must be a subclass of IEvent.
         * @param callBackFun The callback function to invoke if the event type matches.
         *        It should accept a UUID and a reference to TEvent, and return a bool indicating if the event was handled.
         * @return true if the event was handled by the callback; false otherwise.
         */
        template<typename TEvent>
        bool Dispatch(const std::function<bool(UUID, TEvent&)>& callBackFun)
        {
            static_assert(std::is_base_of_v<IEvent, TEvent>, "TEvent must be a subclass of IEvent!");
            if (m_Event.Type() == TEvent::StaticType())
            {
                m_IsEventHandled |= callBackFun(m_WinHandle, static_cast<TEvent&>(m_Event));
                return m_IsEventHandled;
            }

            return false;
        }

        /**
         * @brief Checks if the event has been handled.
         *
         * This function returns a boolean indicating whether the event has been handled
         * by any callback function. It can be used to determine if further processing of
         * the event is necessary.
         *
         * @return true if the event has been handled; false otherwise.
         */
        [[nodiscard]] bool IsHandled() const { return m_IsEventHandled; }

        /**
         * @brief Retrieves the window handle associated with the event.
         *
         * This function returns the UUID of the window that the event is associated with.
         * It can be used to identify which window the event pertains to, especially in
         * multi-window applications.
         *
         * @return UUID The unique identifier of the window associated with the event.
         */
        [[nodiscard]] UUID GetWindowHandle() const { return m_WinHandle; }

    private:
        IEvent& m_Event;
        UUID m_WinHandle{ 0 };
        bool m_IsEventHandled{ false };
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
        /**
         * @brief Type alias for the callback function used in the event registry.
         *
         * This type alias defines a function that takes a variable number of arguments
         * and returns void. It is used to register event handlers in the event registry.
         */
        using EventRegistryCallbackFunction = std::function<void(Args...)>;


        /**
         * @brief Retrieves the singleton instance of the EventRegistry.
         *
         * This method ensures that only one instance of EventRegistry exists throughout
         * the application's lifetime. It provides global access to this instance.
         *
         * @return Reference to the singleton EventRegistry instance.
         */
        static EventRegistry& GetInstance()
        {
            static EventRegistry instance;
            return instance;
        }

        /**
         * @brief Registers a callback function for a specific event type.
         *
         * If a callback is already registered for the given event type, it will be replaced
         * with the new callback. Otherwise, the event type and callback will be added to the registry.
         *
         * @param eventType The type of event to register the callback for.
         * @param callback The callback function to be invoked when the event occurs.
         */
        void Register(EventType eventType, EventRegistryCallbackFunction callback)
        {
            auto it = m_EventRegistry.find(eventType);
            if (it != m_EventRegistry.end())
            {
                it->second = callback;
            }
            else
            {
                m_EventRegistry.emplace(eventType, callback);
            }
        }

        /**
         * @brief Unregisters all handlers associated with the specified event type.
         *
         * Removes the entry for the given event type from the event registry,
         * effectively unregistering all handlers that were previously registered
         * for this event type.
         *
         * @param eventType The type of event whose handlers should be unregistered.
         */
        void Unregister(EventType eventType)
        {
            auto it = m_EventRegistry.find(eventType);
            if (it != m_EventRegistry.end())
            {
                m_EventRegistry.erase(it);
            }
        }

        /**
         * @brief Invokes the event handler associated with the specified event type.
         *
         * This function looks up the event handler registered for the given event type
         * in the event registry and, if found, calls it with the provided arguments.
         *
         * @tparam Args Variadic template parameters representing the argument types to pass to the event handler.
         * @param eventType The type of the event to invoke.
         * @param args Arguments to forward to the event handler.
         */
        void Invoke(EventType eventType, Args... args)
        {
            auto it = m_EventRegistry.find(eventType);
            if (it != m_EventRegistry.end())
            {
                it->second(std::forward<Args>(args)...);
            }
        }

        /**
         * @brief Retrieves the callback function associated with the specified event type.
         *
         * Searches the event registry for the given event type and returns the corresponding
         * callback function if found. If the event type is not registered, returns nullptr.
         *
         * @param eventType The type of event for which to retrieve the callback function.
         * @return EventRegistryCallbackFunction The callback function associated with the event type,
         *         or nullptr if the event type is not registered.
         */
        EventRegistryCallbackFunction Get(EventType eventType)
        {
            auto it = m_EventRegistry.find(eventType);
            if (it != m_EventRegistry.end())
            {
                return it->second;
            }

            return nullptr;
        }

        /**
         * @brief Removes all registered events from the event registry.
         *
         * This function clears the internal event registry, effectively
         * unregistering all previously registered events. After calling
         * this function, the event registry will be empty.
         */
        void Clear()
        {
            m_EventRegistry.clear();
        }

    private:
        std::unordered_map<EventType, EventRegistryCallbackFunction> m_EventRegistry;
    };

    /**
     * @brief Type definition for the event processing function.
     *
     * This type definition defines a function that takes a window handle (UUID) and an event reference
     * and returns void. It is used to process events in the event system.
     */
    using EventProcessingFunction = std::function<void(std::uint64_t, IEvent&)>;
}