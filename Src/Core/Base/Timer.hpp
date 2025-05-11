#pragma once

namespace Motion::Core
{
    class Timer
    {
        public:
            explicit Timer(float deltaTime = 0.0f) : m_DeltaTime(deltaTime) {}
            ~Timer() = default;

            float GetDeltaTime() const { return m_DeltaTime; }
            float GetDeltaTimeSeconds() const { return m_DeltaTime; }
            float GetDeltaTimeMilliseconds() const { return m_DeltaTime * 1000.0f; }

            operator float() const { return m_DeltaTime; }

        private:
            float m_DeltaTime{ 0.0f };
    };
}