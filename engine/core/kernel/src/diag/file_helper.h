// Copyright 2024 N-GINN LLC. All rights reserved.
// Copyright(c) 2015-present, Gabi Melman & spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once

#include <cerrno>
#include <chrono>
#include <cstdio>
#include <functional>
#include <string>
#include <string_view>
#include <thread>
#include <tuple>

#include "nau/io/file_system.h"
#include "nau/service/service_provider.h"

namespace spdlog
{
    using memory_buf_t = std::string;
    using stream_t = nau::io::IStreamWriter::Ptr;
    // TODO: use nau::io::IFile::Ptr when it support file writing.
    using file_t = stream_t;

    struct file_event_handlers
    {
        file_event_handlers() :
            before_open(nullptr),
            after_open(nullptr),
            before_close(nullptr),
            after_close(nullptr)
        {
        }

        std::function<void(std::string_view filename)> before_open;
        std::function<void(std::string_view filename, file_t file_stream)> after_open;
        std::function<void(std::string_view filename, file_t file_stream)> before_close;
        std::function<void(std::string_view filename)> after_close;
    };

    namespace details
    {

        // Helper class for file sinks.
        // When failing to open a file, retry several times(5) with a delay interval(10 ms).
        // Throw spdlog_ex exception on errors.

        NAU_FORCE_INLINE std::string spdlog_ex(const std::string& msg, int last_errno)
        {
            fmt::basic_memory_buffer<char, 250> outbuf;
            fmt::format_system_error(outbuf, last_errno, msg.c_str());
            return std::string(outbuf.data(), outbuf.size());
        }

        NAU_FORCE_INLINE void throw_spdlog_ex(const std::string& msg, int last_errno)
        {
            NAU_FATAL_FAILURE(spdlog_ex(msg, last_errno));
        }

        NAU_FORCE_INLINE void throw_spdlog_ex(const std::string& msg)
        {
            NAU_FATAL_FAILURE(msg);
        }

        class NAU_KERNEL_EXPORT file_helper
        {
        public:
            explicit file_helper(std::string_view fname, const file_event_handlers& event_handlers = {});

            file_helper(const file_helper&) = delete;
            file_helper& operator=(const file_helper&) = delete;
            ~file_helper();

            bool isOpen();
            void open();
            void flush();
            void sync();
            void close();
            void write(const memory_buf_t& buf);
            size_t size() const;
            std::string_view filename() const;

            //
            // return file path and its extension:
            //
            // "mylog.txt" => ("mylog", ".txt")
            // "mylog" => ("mylog", "")
            // "mylog." => ("mylog.", "")
            // "/dir1/dir2/mylog.txt" => ("/dir1/dir2/mylog", ".txt")
            //
            // the starting dot in filenames is ignored (hidden files):
            //
            // ".mylog" => (".mylog". "")
            // "my_folder/.mylog" => ("my_folder/.mylog", "")
            // "my_folder/.mylog.txt" => ("my_folder/.mylog", ".txt")
            static std::tuple<std::string, std::string> split_by_extension(std::string_view fname);

        private:
            const int open_tries_ = 1;
            // TODO: use file_t and VFS when they support file writing.
            // file_t fd_ = nullptr;
            stream_t sd_ = nullptr;
            std::string filename_;
            file_event_handlers event_handlers_;
            std::atomic<bool> filename_is_broken_ = false;
        };

    }  // namespace details
}  // namespace spdlog
