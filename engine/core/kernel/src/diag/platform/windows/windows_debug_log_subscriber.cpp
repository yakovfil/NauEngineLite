// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <nau/string/string.h>

#include <memory>
#include <string_view>

#include "nau/diag/log_subscribers.h"
#include "nau/diag/logging.h"
#include "nau/string/string_conv.h"

namespace nau::diag
{
    /**
     */
    class WindowsDebugLogSubscriber final : public ILogSubscriber
    {
    public:
        ~WindowsDebugLogSubscriber()
        {
        }

    private:
        static std::basic_string_view<char8_t> levelToStr(LogLevel level)
        {
            if (level == LogLevel::Debug)
            {
                return u8"[D]";
            }
            else if (level == LogLevel::Info)
            {
                return u8"[I]";
            }
            else if (level == LogLevel::Warning)
            {
                return u8"[W]";
            }
            else if (level == LogLevel::Error)
            {
                return u8"[E]";
            }
            else if (level == LogLevel::Critical)
            {
                return u8"[C]";
            }

            return u8"[V]";
        }

        void processMessage(const LoggerMessage& message) override;
    };

    static nau::string timeToString(time_t time)
    {
        // TODO use format
        nau::string timeString{64};
        strftime((char*)timeString.data(), 64, "[%F %H:%M:%S]", std::localtime(&time));
        return timeString;
    }

    void WindowsDebugLogSubscriber::processMessage(const LoggerMessage& data)
    {
        if (!::IsDebuggerPresent())
        {
            return;
        }

        const auto fullMessage =
            nau::string::format(u8"{}({}):\n{}{}: {}. \n",
                                nau::string{data.source.filePath},
                                data.source.line.value_or(-1),
                                levelToStr(data.level),
                                timeToString(data.time),
                                data.data);

        const auto debugText = strings::utf8ToWString(fullMessage);

        ::OutputDebugStringW(debugText.c_str());
    }

    ILogSubscriber::Ptr createDebugOutputLogSubscriber()
    {
        return std::make_unique<WindowsDebugLogSubscriber>();
    }
}  // namespace nau::diag
