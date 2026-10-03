// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#include "nau/io/memory_stream.h"
#include "nau/memory/bytes_buffer.h"

namespace nau::test
{
    using namespace ::testing;

    io::IMemoryStream::Ptr createMemoryStream(std::string_view testData)
    {
        BytesBuffer buffer = fromStringView(testData);

        return io::createMemoryStream(std::move(buffer));
    }

    TEST(TestMemoryStream, ReadFromStream)
    {
        std::string_view testData = "test data";
        io::IMemoryStream::Ptr memoryStream = createMemoryStream(testData);

        BytesBuffer resultBuffer;
        const size_t result = *memoryStream->read(resultBuffer.append(testData.size()), testData.size());

        ASSERT_TRUE(result == testData.size());
        ASSERT_THAT(asStringView(resultBuffer), Eq(testData));
    }

    TEST(TestMemoryStream, SetPosition_FromBegin)
    {
        std::string_view testData = "test data";
        io::IMemoryStream::Ptr memoryStream = createMemoryStream(testData);

        constexpr size_t offset = 5;
        const size_t offsetDataSize = testData.size() - offset;
        memoryStream->setPosition(io::OffsetOrigin::Begin, offset);

        BytesBuffer resultBuffer;
        const size_t result = *memoryStream->read(resultBuffer.append(offsetDataSize), offsetDataSize);

        ASSERT_TRUE(result == offsetDataSize);
        ASSERT_THAT(asStringView(resultBuffer), Eq("data"));
    }

    TEST(TestMemoryStream, SetPosition_FromCurrent)
    {
        std::string_view testData = "test data";
        io::IMemoryStream::Ptr memoryStream = createMemoryStream(testData);

        constexpr size_t beginOffset = 4;
        memoryStream->setPosition(io::OffsetOrigin::Begin, beginOffset);

        constexpr size_t offset = 1;
        const size_t offsetDataSize = testData.size() - beginOffset - offset;
        memoryStream->setPosition(io::OffsetOrigin::Current, offset);

        BytesBuffer resultBuffer;
        const size_t result = *memoryStream->read(resultBuffer.append(offsetDataSize), offsetDataSize);

        ASSERT_TRUE(result == offsetDataSize);
        ASSERT_THAT(asStringView(resultBuffer), Eq("data"));
    }

    TEST(TestMemoryStream, SetPosition_FromEnd)
    {
        std::string_view testData = "test data";
        io::IMemoryStream::Ptr memoryStream = createMemoryStream(testData);

        constexpr size_t offset = 4;
        memoryStream->setPosition(io::OffsetOrigin::End, -offset);

        BytesBuffer resultBuffer;
        const size_t result = *memoryStream->read(resultBuffer.append(offset), offset);
        
        ASSERT_TRUE(result == offset);
        ASSERT_THAT(asStringView(resultBuffer), Eq("data"));
    }
}  // namespace nau::test