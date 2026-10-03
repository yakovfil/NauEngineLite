// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
// nau/threading/barrier.h

#pragma once
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <optional>

#include "nau/kernel/kernel_config.h"

namespace nau::threading
{

    /**
     *
     */
    class NAU_KERNEL_EXPORT Barrier
    {
    public:
        Barrier(size_t total);

        Barrier(const Barrier&) = delete;

        bool enter(std::optional<std::chrono::milliseconds> timeout = std::nullopt);

    private:
        const size_t m_total;
        std::atomic_size_t m_counter{0};
        std::mutex m_mutex;
        std::condition_variable m_signal;
    };

}  // namespace nau::threading
