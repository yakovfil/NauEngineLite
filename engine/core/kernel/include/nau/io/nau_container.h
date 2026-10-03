// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <string_view>
#include <tuple>

#include "nau/io/memory_stream.h"
#include "nau/serialization/runtime_value.h"

/**
 * @brief Provides functions for writing and reading container headers for serialization.
 */

namespace nau::io
{
    /**
     * @brief Writes the header for a container to the output stream.
     *
     * This function writes metadata about a container, including its type and data, to the provided output stream.
     * The header is used during deserialization to correctly interpret the container data.
     *
     * @param outputStream A smart pointer to the `IStreamWriter` used for writing the header.
     * @param kind A string view representing the type of the container.
     * @param containerData A shared pointer to `RuntimeValue` containing the container data.
     */
    NAU_KERNEL_EXPORT
    void writeContainerHeader(IStreamWriter::Ptr outputStream, std::string_view kind, const RuntimeValue::Ptr& containerData);

    /**
     * @brief Reads the header for a container from the input stream.
     *
     * This function reads metadata about a container from the provided input stream. The header includes the type of the container
     * and a size indicating the offset of the header data. This information is used to correctly deserialize the container data.
     *
     * @param stream A smart pointer to the `IStreamReader` used for reading the header.
     * @return A `Result` containing a tuple with:
     *         - `RuntimeValue::Ptr`: A shared pointer to `RuntimeValue` representing the container data.
     *         - `size_t`: The offset of the header data.
     */
    NAU_KERNEL_EXPORT
    Result<std::tuple<RuntimeValue::Ptr, size_t>> readContainerHeader(IStreamReader::Ptr stream);
}  // namespace nau::io
