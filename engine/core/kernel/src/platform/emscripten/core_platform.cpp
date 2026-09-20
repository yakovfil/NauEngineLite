// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <cstdio>

#include "nau/debug/debugger.h"
#include "nau/diag/device_error.h"
#include "nau/diag/log_subscribers.h"
#include "nau/io/special_paths.h"
#include "nau/io/stream.h"
#include "nau/rtti/rtti_impl.h"
#include "nau/string/string_conv.h"
#include "nau/string/utf8_noexcept.h"

namespace nau::debug
{
    bool isRunningUnderDebugger()
    {
        // Browser devtools attachment is not detectable by this core API.
        return false;
    }
}  // namespace nau::debug

namespace nau::diag
{
    class CoreDeviceError final : public IDeviceError
    {
        NAU_RTTI_CLASS(nau::diag::CoreDeviceError, IDeviceError)

        FailureActionFlag handleFailure(const FailureData& data) override
        {
            std::fprintf(stderr, "Nau failure: %.*s: %.*s\n",
                         static_cast<int>(data.condition.size()), data.condition.data(),
                         static_cast<int>(data.message.size()), data.message.data());
            std::fflush(stderr);
            return data.kind == AssertionKind::Fatal ? FailureActionFlag{FailureAction::Abort} : FailureActionFlag{};
        }
    };

    IDeviceError::Ptr createDefaultDeviceError()
    {
        return eastl::make_unique<CoreDeviceError>();
    }

    ILogSubscriber::Ptr createDebugOutputLogSubscriber()
    {
        return createConioOutputLogSubscriber();
    }
}  // namespace nau::diag

namespace nau::strings
{
    eastl::wstring utf8ToWString(eastl::u8string_view text)
    {
        static_assert(sizeof(wchar_t) == 4);
        eastl::wstring result;
        nau_utf8::utf8to32(text.begin(), text.end(), std::back_inserter(result));
        return result;
    }

    eastl::u8string wstringToUtf8(eastl::wstring_view text)
    {
        std::u32string decoded;
        nau_utf8::utf16to32(text.begin(), text.end(), std::back_inserter(decoded));
        eastl::u8string result;
        for (char32_t cp : decoded)
        {
            utf8::internal::append(static_cast<utf8::utfchar32_t>(cp), std::back_inserter(result));
        }
        return result;
    }
}  // namespace nau::strings

namespace nau::io
{
    IStreamBase::Ptr createNativeFileStream(const char*, AccessModeFlag, OpenFileMode)
    {
        std::fputs("Nau: native host file streams are unavailable on wasm32\n", stderr);
        return nullptr;
    }

    std::filesystem::path getKnownFolderPath(KnownFolder folder)
    {
        if (folder == KnownFolder::Current)
        {
            return std::filesystem::current_path();
        }
        std::fputs("Nau: requested host folder is unavailable on wasm32\n", stderr);
        return {};
    }

    eastl::u8string getNativeTempFilePath(eastl::u8string_view)
    {
        std::fputs("Nau: native host temporary files are unavailable on wasm32\n", stderr);
        return {};
    }
}  // namespace nau::io
