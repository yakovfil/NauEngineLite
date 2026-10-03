// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nau/platform/windows/diag/win_error.h"

#include <string>
#include <string_view>

#include "nau/string/string_conv.h"

namespace nau::diag
{
    namespace
    {
        // https://msdn.microsoft.com/en-us/library/ms679351(v=VS.85).aspx
        inline DWORD formatMessageHelper(DWORD messageId, LPWSTR& buffer)
        {
            return FormatMessageW(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                nullptr, messageId,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                reinterpret_cast<LPWSTR>(&buffer), 0, nullptr);
        }

        inline DWORD formatMessageHelper(DWORD messageId, LPSTR& buffer)
        {
            return FormatMessageA(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                nullptr, messageId,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                reinterpret_cast<LPSTR>(&buffer), 0, nullptr);
        }

        template <typename Char>
        std::basic_string<Char> getWindowsMessage(DWORD messageId)
        {
            if (messageId == 0)
            {
                return {};
            }

            Char* messageBuffer = nullptr;
            scope_on_leave
            {
                if (messageBuffer)
                {
                    ::LocalFree(messageBuffer);
                }
            };

            std::basic_string<Char> resultMessage;
            const auto length = formatMessageHelper(messageId, messageBuffer);
            if (length == 0)
            {
                SetLastError(0);
            }
            else if (messageBuffer != nullptr)
            {
                resultMessage.assign(messageBuffer, length);
            }

            return resultMessage;
        }

        inline std::string makeWinErrorMessage(unsigned errorCode)
        {
            std::wstring message = getWinErrorMessageW(errorCode);
            const auto errorMessage = strings::wstringToUtf8(std::wstring_view{message.data(), message.size()});
            return std::string{reinterpret_cast<const char*>(errorMessage.data()), errorMessage.size()};
        }

        inline std::string makeWinErrorMessage(unsigned errorCode, std::string_view customMessage)
        {
            const std::wstring errorMessageW = getWinErrorMessageW(errorCode);
            const std::u8string errorMessage = strings::wstringToUtf8(std::wstring_view{errorMessageW.data(), errorMessageW.size()});
            const std::string_view errorMessageSV = strings::toStringView(errorMessage);
            const std::string formattedMessage = ::fmt::format("{}. code:({}):{}", customMessage, errorCode, errorMessageSV);
            return std::string{formattedMessage.data(), formattedMessage.size()};
        }
    }  // namespace

    WinCodeError::WinCodeError(const diag::SourceInfo& sourceInfo, unsigned errorCode) :
        DefaultError<>(sourceInfo, makeWinErrorMessage(errorCode)),
        m_errorCode(errorCode)
    {
    }

    WinCodeError::WinCodeError(const diag::SourceInfo& sourceInfo, std::string message, unsigned errorCode) :
        DefaultError<>(sourceInfo, makeWinErrorMessage(errorCode, message)),
        m_errorCode(errorCode)
    {
    }

    unsigned WinCodeError::getErrorCode() const
    {
        return m_errorCode;
    }

    unsigned getAndResetLastErrorCode()
    {
        const auto error = GetLastError();
        if (error != 0)
        {
            SetLastError(0);
        }

        return error;
    }

    std::wstring getWinErrorMessageW(unsigned errorCode)
    {
        return getWindowsMessage<wchar_t>(errorCode);
    }

    std::string getWinErrorMessageA(unsigned errorCode)
    {
        return getWindowsMessage<char>(errorCode);
    }
}  // namespace nau::diag
