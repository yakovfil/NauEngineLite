// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <unordered_map>

#include "audio_common.hpp"

NAU_AUDIO_BEGIN

// ** Subscribable

template <typename Callback>
class Subscribable
{
public:
    virtual void subscribe(void* subscriber, Callback callback);
    virtual void unsubscribe(void* subscriber);

protected:
    virtual void notifyAll();

private:
    std::unordered_map<uint64_t, Callback> m_watchers;
};

template <typename Callback>
void Subscribable<Callback>::subscribe(void* subscriber, Callback callback)
{
    m_watchers[reinterpret_cast<int64_t>(subscriber)] = callback;
}

template <typename Callback>
void Subscribable<Callback>::unsubscribe(void* subscriber)
{
    m_watchers.erase(reinterpret_cast<int64_t>(subscriber));
}

template <typename Callback>
void Subscribable<Callback>::notifyAll()
{
    for (auto& [_, watcherCallback] : m_watchers) {
        watcherCallback();
    }
}

NAU_AUDIO_END
