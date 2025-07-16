#include "CorePCH.hpp"
#include "DrawCommand.hpp"

namespace Motion::Core
{
    /**
     * @brief Submits a draw command to the queue for later execution.
     *
     * This method is thread-safe and adds the provided draw command to the current write buffer.
     * The draw command is moved into the buffer to avoid unnecessary copying.
     *
     * @param drawCommand A shared pointer to the DrawCommand to be submitted.
     */
    void DrawCommandQueue::Submit(const DrawCommand& drawCommand)
    {
        std::scoped_lock lock(m_SubmitMutex);
        m_Buffers[m_WriteIndex].push_back(drawCommand);
    }

    /**
     * @brief Retrieves the buffer of draw commands that is ready to be consumed.
     *
     * This function returns a reference to the vector of shared pointers to DrawCommand objects
     * from the buffer that is not currently being written to. It is typically used in a double-buffered
     * system to safely access the set of draw commands prepared in the previous frame or cycle.
     *
     * @return Reference to the vector of shared pointers to DrawCommand objects that are ready for consumption.
     */
    std::vector<DrawCommand>& DrawCommandQueue::Consume()
    {
        return m_Buffers[1 - m_WriteIndex.load()];
    }

    /**
     * @brief Swaps the active command buffer for drawing operations.
     *
     * This method clears the current write buffer and switches the write index
     * to the other buffer in a double-buffered queue. It ensures thread safety
     * by locking the submit mutex during the operation. This allows for efficient
     * command submission and processing without data races.
     */
    void DrawCommandQueue::SwapBuffers()
    {
        {
            std::scoped_lock lock(m_SubmitMutex);
            m_Buffers[m_WriteIndex].clear();
            m_WriteIndex.store(1 - m_WriteIndex.load());
        }
    }
}
