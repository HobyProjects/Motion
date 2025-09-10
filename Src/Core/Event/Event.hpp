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
        Window      = BIT(0),
        Keyboard    = BIT(1),
        Mouse       = BIT(2),
        GamePad     = BIT(3),
        Unknown     = BIT(4)
    };

    template<>
    struct enable_bitmask_operations<EventCategory> : std::true_type {};

    enum class EventType : uint32_t
    {
        WindowClose                         = BIT(0),
        WindowResize                        = BIT(1),
        WindowPosChange                     = BIT(2),
        WindowFocusGain                     = BIT(3),
        WindowFocusLost                     = BIT(4),
        WindowFrameBufferSizeChange         = BIT(5),
        WindowMaximize                      = BIT(6),
        WindowMinimize                      = BIT(7),

        KeyboardKeyPress                    = BIT(8),
        KeyboardKeyRelease                  = BIT(9),
        KeyboardKeyRepeat                   = BIT(10),
        KeyboardKeyChar                     = BIT(11),

        MouseButtonDown                     = BIT(12),
        MouseButtonUp                       = BIT(13),
        MouseWheelScroll                    = BIT(14),
        MouseCursorPosChange                = BIT(15),
        MouseCursorWindowEnter              = BIT(16),
        MouseCursorWindowLeave              = BIT(17)
    };

    template<>
    struct enable_bitmask_operations<EventType> : std::true_type {};

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