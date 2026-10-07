// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nau/io/nau_container.h"

#include <charconv>
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
                if (httpHeaderStringify.size() >= 16384)
                    return NauMakeError("Container HTTP header exceeds 16 KiB");
                char buffer[1] = "";
                auto read = stream->read(reinterpret_cast<std::byte*>(buffer), 1);
                if (!read)
                    return read.getError();
                if (*read != 1)
                    return NauMakeError("Truncated container HTTP header");

                httpHeaderStringify.append(buffer, 1);
            }

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
        if (!stream)
            return NauMakeError("Missing container stream");
        std::vector<std::tuple<std::string, std::string>> httpHeader;
        auto headerResult = readHttpHeader(stream, httpHeader);
        NauCheckResult(headerResult);
        const size_t headerLength = *headerResult;
        size_t contentLength = 0;
        for (const auto& [name, value] : httpHeader)
        {
            if (name == "Content-Length")
            {
                if (contentLength != 0)
                    return NauMakeError("Duplicate container Content-Length");
                auto parsed = std::from_chars(value.data(), value.data() + value.size(), contentLength);
                if (parsed.ec != std::errc{} || parsed.ptr != value.data() + value.size() ||
                    contentLength == 0 || contentLength > 16 * 1024 * 1024)
                    return NauMakeError("Invalid container Content-Length");
            }
        }
        if (contentLength == 0)
            return NauMakeError("Missing container Content-Length");

        // Existing Nau writers emit three line breaks after the HTTP fields.
        // readHttpHeader consumes two; the remaining separator is part of this
        // read, preserving the established payload offset.
        ++contentLength;

        BytesBuffer buffer(contentLength);
        auto read = stream->read(buffer.data(), contentLength);
        NauCheckResult(read);
        if (*read != contentLength)
            return NauMakeError("Truncated container JSON header");
        auto parsed = serialization::jsonParseString(strings::toU8StringView(asStringView(buffer)));
        NauCheckResult(parsed);

        return std::make_tuple(*parsed, contentLength + headerLength);
    }
}  // namespace nau::io
