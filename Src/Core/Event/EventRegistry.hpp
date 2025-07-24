#pragma once

#include <unordered_map>
#include <functional>

#include "Event.hpp"

namespace Motion
{
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
         * @brief Registers a callback function for a specific event type.
         *
         * If a callback is already registered for the given event type, it will be replaced
         * with the new callback. Otherwise, the event type and callback will be added to the registry.
         *
         * @param eventType The type of event to register the callback for.
         * @param callback The callback function to be invoked when the event occurs.
         */
        static void Register(EventType eventType, EventRegistryCallbackFunction callback)
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
        static void Unregister(EventType eventType)
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
        static void Invoke(EventType eventType, Args... args)
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
        static EventRegistryCallbackFunction Get(EventType eventType)
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
        static void Clear()
        {
            m_EventRegistry.clear();
        }

    private:
        inline static std::unordered_map<EventType, EventRegistryCallbackFunction> m_EventRegistry;
    };
}