// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#include "nau/io/stream.h"
#include "nau/io/memory_stream.h"

namespace nau::test
{
    void fillBufferWithDefaultContent(BytesBuffer& buffer, std::optional<size_t> size)
    {
        uint8_t* const charPtr = reinterpret_cast<uint8_t*>(buffer.data());

        for(size_t i = 0; i < size; ++i)
        {
            charPtr[i] = static_cast<uint8_t>(i % std::numeric_limits<uint8_t>::max());
        }
    }

    TEST(TestStreamFunction, CopyStream)
    {
        using namespace nau::io;

        std::string_view testData = "test data";

        BytesBuffer buffer;
        buffer = fromStringView(testData);

        IMemoryStream::Ptr srcStream = createMemoryStream(std::move(buffer));
        IMemoryStream::Ptr dstStream = createMemoryStream();

        const size_t size = *io::copyStream(*dstStream, *srcStream);
        ASSERT_TRUE(size == testData.size());

        BytesBuffer result;
        dstStream->setPosition(OffsetOrigin::Begin, 0);
        dstStream->read(result.append(testData.size()), testData.size()).ignore();
        
        ASSERT_TRUE(memcmp(testData.data(), result.data(), testData.size()) == 0);
    }

    TEST(TestStreamFunction, CopyStream_WithLongData)
    {
        using namespace nau::io;

        constexpr size_t bufferSize = 1048576;
        BytesBuffer buffer(bufferSize);
        fillBufferWithDefaultContent(buffer, bufferSize);
        std::string_view testData = asStringView(buffer);

        IMemoryStream::Ptr srcStream = createMemoryStream(std::move(buffer));
        IMemoryStream::Ptr dstStream = createMemoryStream();

        const size_t size = *io::copyStream(*dstStream, *srcStream);
        ASSERT_TRUE(size == bufferSize);

        BytesBuffer result;
        dstStream->setPosition(OffsetOrigin::Begin, 0);
        dstStream->read(result.append(bufferSize), bufferSize).ignore();

        ASSERT_TRUE(memcmp(testData.data(), result.data(), bufferSize) == 0);
    }

    TEST(TestStreamFunction, CopyStream_CheckCorrectPosition)
    {
        using namespace nau::io;

        constexpr size_t bufferSize = 516;
        BytesBuffer buffer(bufferSize);
        fillBufferWithDefaultContent(buffer, bufferSize);
        std::string_view testData = asStringView(buffer);

        IMemoryStream::Ptr srcStream = createMemoryStream(std::move(buffer));
        IMemoryStream::Ptr dstStream = createMemoryStream();

        const size_t size = *io::copyStream(*dstStream, *srcStream);
        ASSERT_TRUE(size == bufferSize);

        BytesBuffer result;
        dstStream->setPosition(OffsetOrigin::Begin, 0);
        dstStream->read(result.append(bufferSize), bufferSize).ignore();

        ASSERT_TRUE(dstStream->getPosition() == srcStream->getPosition());
    }
}