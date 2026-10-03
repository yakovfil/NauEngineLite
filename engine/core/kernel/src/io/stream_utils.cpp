// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nau/io/stream_utils.h"

#include <string_view>

namespace nau::io
{
    GenLoadOverStream::GenLoadOverStream(IStreamReader::Ptr stream, std::string_view targetName) :
        m_stream(std::move(stream)),
        m_targetName(targetName)
    {
        NAU_FATAL(m_stream);
    }

    void GenLoadOverStream::read(void* ptr, int size)
    {
        for (int actualRead = 0; actualRead < size;)
        {
            const Result<size_t> readResult = m_stream->read(reinterpret_cast<std::byte*>(ptr), static_cast<size_t>(size - actualRead));
            NAU_ASSERT(readResult);
            NAU_ASSERT(*readResult > 0, "Unexpected end of stream");
            if (!readResult || *readResult == 0)
            {
                return;
            }

            actualRead += static_cast<int>(*readResult);
            NAU_ASSERT(actualRead <= size);
        }
    }

    int GenLoadOverStream::tryRead(void* ptr, int size)
    {
        const auto readResult = m_stream->read(reinterpret_cast<std::byte*>(ptr), static_cast<size_t>(size));
        NAU_ASSERT(readResult);
        if (!readResult)
        {
            return 0;
        }

        return static_cast<int>(*readResult);
    }

    int GenLoadOverStream::tell()
    {
        return static_cast<int>(m_stream->getPosition());
    }

    void GenLoadOverStream::seekto(int position)
    {
        m_stream->setPosition(io::OffsetOrigin::Begin, position);
    }

    void GenLoadOverStream::seekrel(int offset)
    {
        m_stream->setPosition(io::OffsetOrigin::Current, offset);
    }

    const char* GenLoadOverStream::getTargetName()
    {
        const char* const DefaultTargetName = "io::GenLoadOverStream";
        return m_targetName.empty() ? DefaultTargetName : m_targetName.c_str();
    }

}  // namespace nau::io
