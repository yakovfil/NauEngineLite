// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
// helpers/buffer_test_utils.h


#pragma once
#include <gtest/gtest.h>

#include <optional>

#include "nau/memory/bytes_buffer.h"


namespace nau::test
{

    void fillBufferWithDefaultContent(nau::BytesBuffer& buffer, size_t offset = 0, std::optional<size_t> size = std::nullopt);

    nau::BytesBuffer createBufferWithDefaultContent(size_t size);

    testing::AssertionResult buffersEqual(const nau::BufferView& buffer1, const nau::BufferView& buffer2);

}  // namespace nau::test