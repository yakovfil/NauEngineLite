// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#include "nau/shared/logger.h"

#include <time.h>

#include <chrono>
#include <filesystem>
#if defined(_WIN32) || defined(_WIN64)
    #include "nau/shared/platform/win/windows_console.h"
typedef nau::WinConsoleStyle IConsoleStyle;
#elif defined(__linux__) || defined(__linux)
    #include "nau/shared/platform/linux/linux_console.h"
#else
    #include "nau/shared/platform/mac/mac_console.h"
#endif
#include <cassert>

namespace nau
{
    namespace logger
    {
        static IConsoleStyle g_consoleStyle;
        static nau::diag::Logger::SubscriptionHandle g_fileOutHandle;
        static nau::diag::Logger::SubscriptionHandle g_consoleOutHandle;
        static nau::diag::Logger::Ptr g_logger;

        void init(std::string_view output, bool verbosity)
        {
            std::filesystem::path filename;

            if (output.empty())
            {
                auto currPath = std::filesystem::current_path().string();
                output = currPath;
            }

            filename = std::filesystem::path(output) / std::filesystem::path(std::format("log_{}.log", std::to_string(std::time(nullptr))));

            g_logger = nau::diag::createLogger();
            g_fileOutHandle = g_logger->subscribe(nau::diag::createFileOutputLogSubscriber({filename.string().c_str()}));
        }

        void writeOutput(nau::diag::LogLevel level, const std::string& fmt, const LoggerTag& tag, const nau::diag::SourceInfo& info)
        {
            g_logger->logMessage(level, {tag.m_tag}, info, fmt.data());
        }

        void addConsoleOutput(bool verbosity)
        {
            g_consoleOutHandle = getLogger()->subscribe([&](const nau::diag::LoggerMessage& message)
            {
                g_consoleStyle.setColor(static_cast<unsigned char>(message.level));

                std::cout << message.data.c_str() << std::endl;

                g_consoleStyle.reset();
            }, [verbosity](const nau::diag::LoggerMessage& message) -> bool
            {
                return verbosity ? true : message.level > nau::diag::LogLevel::Info;
            });
        }

        nau::diag::Logger::Ptr getLogger()
        {
            return g_logger;
        }
    }  // namespace logger
}  // namespace nau