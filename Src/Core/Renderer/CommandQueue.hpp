#pragma once

#include <vector>
#include "RenderCommand.hpp"

namespace Motion
{
    class CommandQueue
    {
        public:
            CommandQueue() = default;
            ~CommandQueue() = default;

            void Execute();
            void Sort();
            
            inline void Clear() { m_CommandQueue.clear(); }
            inline void Submit(const RenderCommand& command) { m_CommandQueue.emplace_back(command); }

        private:
            void EnsureInitialized();
        
        private:
            std::shared_ptr<ITexture> m_GrayTexture{nullptr};
            std::shared_ptr<ITexture> m_WhiteTexture{nullptr};
            std::shared_ptr<ITexture> m_BlackTexture{nullptr};
            std::shared_ptr<ITexture> m_NormalTexture{nullptr};

            std::vector<RenderCommand> m_CommandQueue;
            std::once_flag m_InitOnce;
    };
}