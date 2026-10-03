// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <gtest/gtest.h>

#include <iostream>
#include <regex>
#include <sstream>

#include "nau/diag/log_subscribers.h"

TEST(AssetDiagnostics, TimestampFormattingOwnsItsWritableBuffer)
{
    std::ostringstream captured;
    struct Redirect
    {
        std::streambuf* previous;
        ~Redirect()
        {
            std::cout.rdbuf(previous);
        }
    } redirect{std::cout.rdbuf(captured.rdbuf())};
    nau::diag::LoggerMessage message{};
    message.index = 7;
    message.time = 1700000000;
    message.level = nau::diag::LogLevel::Info;
    message.tags = {"asset", "lookup"};
    message.data = "diagnostic payload";
    const auto subscriber = nau::diag::createConioOutputLogSubscriber();
    subscriber->processMessage(message);
    const auto output = captured.str();
    EXPECT_EQ(output.find('\0'), std::string::npos);
    EXPECT_TRUE(std::regex_search(output, std::regex(R"(\[7\]\[\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\]\[Info\]\[asset, lookup\]: diagnostic payload)")));
}
