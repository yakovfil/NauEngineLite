// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nau/test/helpers/assert_catcher_guard.h"

#include <memory>

#include "nau/diag/device_error.h"
#include "nau/rtti/rtti_impl.h"

namespace nau::test
{

    namespace
    {
        class AssertCatcherDeviceError final : public diag::IDeviceError
        {
            NAU_RTTI_CLASS(AssertCatcherDeviceError, diag::IDeviceError)

        public:
            AssertCatcherDeviceError(AssertCatcherGuard& guard) :
                m_guard(guard)
            {
            }

            diag::FailureActionFlag handleFailure(const diag::FailureData& data) final
            {
                ++m_guard.assertFailureCounter;
                ++m_guard.fatalFailureCounter;

                return diag::FailureAction::None;
            }

        private:
            AssertCatcherGuard& m_guard;
        };
    }  // namespace

    AssertCatcherGuard::AssertCatcherGuard()
    {
        diag::setDeviceError(std::make_unique<AssertCatcherDeviceError>(*this));
    }

    AssertCatcherGuard::~AssertCatcherGuard()
    {
        diag::setDeviceError(nullptr);
    }
}  // namespace nau::test
