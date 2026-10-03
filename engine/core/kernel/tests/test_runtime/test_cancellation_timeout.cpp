// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <memory>
#include <gtest/gtest.h>

#include <string>

#include "nau/diag/device_error.h"
#include "nau/rtti/rtti_impl.h"
#include "nau/utils/cancellation.h"
#include "nau/utils/scope_guard.h"

namespace
{
    class TimeoutDiagnostic final : public nau::diag::IDeviceError
    {
        NAU_RTTI_CLASS(TimeoutDiagnostic, nau::diag::IDeviceError)

    public:
        unsigned count = 0;
        std::string message;

        nau::diag::FailureActionFlag handleFailure(const nau::diag::FailureData& data) override
        {
            ++count;
            message.assign(data.message.data(), data.message.size());
            return nau::diag::FailureAction::None;
        }
    };
}  // namespace

TEST(TestCancellation, TimeoutRetainsUnsupportedDiagnostic)
{
    auto device = std::make_unique<TimeoutDiagnostic>();
    auto* diagnostic = device.get();
    nau::diag::IDeviceError::Ptr previous;
    nau::diag::setDeviceError(std::move(device), &previous);
    scope_on_leave
    {
        nau::diag::setDeviceError(std::move(previous));
    };

    nau::CancellationSource source;
    auto token = source.getCancellation();
    unsigned calls = 0;
    auto subscription = token.subscribe([](void* data)
    {
        ++*static_cast<unsigned*>(data);
    }, &calls);

    source.setTimeout(std::chrono::milliseconds(0));
    source.setTimeout(std::chrono::microseconds(1000));
    EXPECT_EQ(diagnostic->count, 2u);
    EXPECT_EQ(diagnostic->message, "CancellationState::SetTimeout is not implemented");
    EXPECT_FALSE(source.isCancelled());
    EXPECT_FALSE(token.isCancelled());
    EXPECT_EQ(calls, 0u);

    source.cancel();
    EXPECT_TRUE(token.isCancelled());
    EXPECT_EQ(calls, 1u);
}
