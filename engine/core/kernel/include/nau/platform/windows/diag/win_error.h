// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once
#include <string>

#include "nau/diag/error.h"
#include "nau/kernel/kernel_config.h"

namespace nau::diag
{
    /**
     */
    NAU_KERNEL_EXPORT unsigned getAndResetLastErrorCode();

    /**
     */
    NAU_KERNEL_EXPORT std::wstring getWinErrorMessageW(unsigned errorCode);

    /**
     */
    NAU_KERNEL_EXPORT std::string getWinErrorMessageA(unsigned errorCode);

    /**
     */
    class NAU_KERNEL_EXPORT WinCodeError : public nau::DefaultError<>
    {
        NAU_ERROR(nau::diag::WinCodeError, nau::DefaultError<>)

    public:
        WinCodeError(const diag::SourceInfo& sourceInfo, unsigned errorCode = diag::getAndResetLastErrorCode());

        WinCodeError(const diag::SourceInfo& sourceInfo, std::string message, unsigned errorCode = diag::getAndResetLastErrorCode());

        unsigned getErrorCode() const;

    private:
        const unsigned m_errorCode;
    };

}  // namespace nau::diag
