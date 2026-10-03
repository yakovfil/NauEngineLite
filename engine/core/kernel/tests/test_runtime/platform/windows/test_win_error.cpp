// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#include "nau/platform/windows/diag/win_error.h"

namespace nau::test
{
    /**
        Test that getWinErrorMessage returns something (without crash).
     */
    TEST(TestWinError, GetErrorMessage)
    {
        const unsigned AnyErrorCode = 1008;

        const auto message = diag::getWinErrorMessageA(AnyErrorCode);

        ASSERT_FALSE(message.empty());
    }
}  // namespace nau::test
