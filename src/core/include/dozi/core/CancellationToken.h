#pragma once

#include <atomic>

namespace dozi::core {

class CancellationToken final {
public:
    void cancel() noexcept { cancelled_.store(true, std::memory_order_relaxed); }
    [[nodiscard]] bool isCancelled() const noexcept
    {
        return cancelled_.load(std::memory_order_relaxed);
    }

private:
    std::atomic<bool> cancelled_ { false };
};

} // namespace dozi::core
