// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <gmock/gmock.h>
#include <gtest/gtest-param-test.h>
#include <gtest/gtest.h>

#include <filesystem>

#include "nau/shared/logger.h"

int main(int argc, char** argv)
{
    nau::logger::init(std::filesystem::current_path().string(), true);
    nau::logger::addConsoleOutput(true);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}