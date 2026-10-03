// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nau/io/nau_container.h"

#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include "nau/memory/standard_aliases.h"
#include "nau/serialization/json.h"
#include "nau/string/string_utils.h"

namespace nau::io
{
    namespace
    {
        // TODO: refactor to using stack vector, stack string
        void writeHttpHeader(IStreamWriter::Ptr stream, const Vector<std::tuple<std::string, std::string>>& httpHeader)
        {
            std::string httpHeaderStringify;
            for (const auto& [name, value] : httpHeader)
            {
                httpHeaderStringify += name + ": " + value + "\n";
            }

            httpHeaderStringify += "\n\n";
            NAU_VERIFY(*stream->write(reinterpret_cast<std::byte*>(httpHeaderStringify.data()), httpHeaderStringify.size()) == httpHeaderStringify.size());
        }

        Result<size_t> readHttpHeader(IStreamReader::Ptr stream, std::vector<std::tuple<std::string, std::string>>& httpHeader)
        {
            std::string httpHeaderStringify;
            httpHeaderStringify.reserve(256);
            while ((httpHeaderStringify.find("\n\n") == std::string::npos))
            {
                char buffer[1] = "";
                const size_t actualRead = *stream->read(reinterpret_cast<std::byte*>(buffer), 1);
                if (actualRead == 0)
                {
                    NAU_ASSERT(false);
                    break;
                }

                httpHeaderStringify.append(buffer, 1);
            }

            NAU_ASSERT(httpHeaderStringify.find("\n\n") != std::string::npos);

            auto httpHeaderLineSequence = strings::split(httpHeaderStringify, std::string_view{"\n"});
            for (std::string_view headerLine : httpHeaderLineSequence)
            {
                auto [key, value] = strings::cut(headerLine, ':');
                httpHeader.emplace_back(std::string{strings::trim(key)}, std::string{strings::trim(value)});
            }
            return httpHeaderStringify.size();
        }
    }  // namespace

    void writeContainerHeader(IStreamWriter::Ptr outputStream, std::string_view kind, const RuntimeValue::Ptr& containerData)
    {
        io::IMemoryStream::Ptr tempStream = io::createMemoryStream();
        serialization::jsonWrite(tempStream->as<io::IStreamWriter&>(), containerData).ignore();

        const std::span<const std::byte> serializedData = tempStream->getBufferAsSpan();
        const std::string contentLength = std::to_string(serializedData.size());

        Vector<std::tuple<std::string, std::string>> httpHeader = {
            {"NauContent-Kind",        std::string(kind)},
            {   "Content-Type",       "application/json"},
            { "Content-Length", std::move(contentLength)}
        };

        writeHttpHeader(outputStream, httpHeader);

        tempStream->setPosition(io::OffsetOrigin::Begin, 0);
        io::copyStream(*outputStream, tempStream->as<io::IStreamReader&>()).ignore();
    }

    Result<std::tuple<RuntimeValue::Ptr, size_t>> readContainerHeader(IStreamReader::Ptr stream)
    {
        std::vector<std::tuple<std::string, std::string>> httpHeader;
        const size_t headerLength = *readHttpHeader(stream, httpHeader);
        size_t contentLength = 0;
        for (const auto& [name, value] : httpHeader)
        {
            if (name == "Content-Length")
            {
                contentLength = std::stoi(value.data()) + 1;
                break;
            }
        }
        NAU_ASSERT(contentLength != 0);

        BytesBuffer buffer(contentLength);
        NAU_VERIFY(*stream->read(buffer.data(), contentLength) == contentLength);
        RuntimeValue::Ptr result = *serialization::jsonParseString(strings::toU8StringView(asStringView(buffer)));

        return std::make_tuple(result, contentLength + headerLength);
    }
}  // namespace nau::io
