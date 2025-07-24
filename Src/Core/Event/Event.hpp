#pragma once

#include <string_view>
#include <functional>
#include <type_traits>

#include "Base.hpp"
#include "UUID.hpp"

#define EVENT_CALLBACK(CALLBACK_FUNC) [this](auto&&... args) -> decltype(auto) { return this->CALLBACK_FUNC(std::forward<decltype(args)>(args)...); }

namespace Motion
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



    /**
     * @brief Type definition for the event processing function.
     *
     * This type definition defines a function that takes a window handle (UUID) and an event reference
     * and returns void. It is used to process events in the event system.
     */
    using EventProcessingFunction = std::function<void(std::uint64_t, IEvent&)>;
}