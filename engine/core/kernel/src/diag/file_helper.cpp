// Copyright(c) 2015-present, Gabi Melman & spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#include "file_helper.h"
#include "nau/io/special_paths.h"
#include "nau/string/string_conv.h"

namespace spdlog
{
    namespace details
    {

        file_helper::file_helper(eastl::string_view fname, const file_event_handlers& event_handlers) :
            filename_(fname),
            event_handlers_(event_handlers)
        {
        }

        file_helper::~file_helper()
        {
            close();
        }

        bool file_helper::isOpen()
        {
            return sd_ != nullptr;
        }

        void file_helper::open()
        {
            using namespace nau;
            namespace fs = std::filesystem;

            if (filename_is_broken_)
            {
                return;
            }

            close();
            scope_on_leave
            {
                filename_is_broken_ = (sd_ == nullptr);
            };

            if (event_handlers_.before_open)
            {
                event_handlers_.before_open(filename_);
            }

            fs::path logFilePath(strings::toStringView(filename_));
            if (!logFilePath.is_absolute())
            {
                // Currently log will be saved at %LOCALAPPDATA%\nau\logs\log_filename.txt
                // (i.e.: c:\Users\[current_user]\AppData\Local\nau\logs\logs.2024-09-22.20-38-57.log)
                const auto nauAppDataPath = io::getKnownFolderPath(io::KnownFolder::LocalAppData) / "nau";
                logFilePath = nauAppDataPath / logFilePath;
            }
            else
            {
                logFilePath = fs::absolute(logFilePath);
            }
            
            if (const auto logDirPath = logFilePath.parent_path(); !fs::exists(logDirPath))
            {
                std::error_code ec;
                if (!fs::create_directories(logDirPath, ec))
                {
                    filename_is_broken_ = true;
                    return;
                }
            }

            logFilePath.make_preferred();

            for (int tries = 0; tries < open_tries_; ++tries)
            {
                // TODO: use fd_ when it support file writing.
                // fd_ = nau::getServiceProvider().get<nau::io::IFileSystem>().openFile(filename_,
                //                                                                     nau::io::AccessMode::Write | nau::io::AccessMode::Async,
                //                                                                     nau::io::OpenFileMode::CreateAlways);
                auto utf8Path = logFilePath.u8string();
                sd_ = nau::io::createNativeFileStream(reinterpret_cast<const char*>(utf8Path.c_str()),
                                                      nau::io::AccessMode::Write | nau::io::AccessMode::Async,
                                                      nau::io::OpenFileMode::CreateAlways);

                if (sd_)
                {
                    if (event_handlers_.after_open)
                    {
                        event_handlers_.after_open(filename_, sd_);
                    }
                    return;
                }
            }

            // throw temporary is disabled: there is no need to stop application if file creation is failed
            const bool isCritical = false;
            if (isCritical)
            {
                throw_spdlog_ex("Failed opening file " + filename_ + " for writing", errno);
            }
        }

        void file_helper::flush()
        {
            if (sd_)
            {
                sd_->flush();
            }
        }

        void file_helper::sync()
        {
            if (sd_)
            {
                sd_->flush();  // TODO: redo when async read/write implemented
            }
        }

        void file_helper::close()
        {
            if (sd_ != nullptr)
            {
                if (event_handlers_.before_close)
                {
                    event_handlers_.before_close(filename_, sd_);
                }
                sd_->flush();
                sd_ = nullptr;
                // TODO: use fd_ when it support file writing.
                // fd_ = nullptr;

                if (event_handlers_.after_close)
                {
                    event_handlers_.after_close(filename_);
                }
            }
        }

        void file_helper::write(const memory_buf_t& buf)
        {
            if (sd_ == nullptr)
                return;
            size_t msg_size = buf.size();
            auto data = buf.data();
            if (*sd_->write(reinterpret_cast<const std::byte*>(data), msg_size) != msg_size)
            {
                throw_spdlog_ex("Failed writing to file " + filename_, errno);
            }
        }

        size_t file_helper::size() const
        {
            if (sd_ == nullptr)
            {
                throw_spdlog_ex("Cannot use size() on closed file " + filename_);
            }
            // TODO: use fd_ when it support file writing.
            return sd_->getPosition();
        }

        eastl::string_view file_helper::filename() const
        {
            return filename_;
        }

        constexpr static const eastl::string::value_type folder_seps_filename[] = "\\/";

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
        std::tuple<eastl::string, eastl::string> file_helper::split_by_extension(
            eastl::string_view fname)
        {
            auto ext_index = fname.rfind('.');

            // no valid extension found - return whole path and empty string as
            // extension
            if (ext_index == eastl::string::npos || ext_index == 0 || ext_index == fname.size() - 1)
            {
                return std::make_tuple(eastl::string(fname), eastl::string());
            }

            // treat cases like "/etc/rc.d/somelogfile or "/abc/.hiddenfile"
            auto folder_index = fname.find_last_of(folder_seps_filename);
            if (folder_index != eastl::string::npos && folder_index >= ext_index - 1)
            {
                return std::make_tuple(eastl::string(fname), eastl::string());
            }

            // finally - return a valid base and extension tuple
            return std::make_tuple(eastl::string(fname.substr(0, ext_index)), eastl::string(fname.substr(ext_index)));
        }

    }  // namespace details
}  // namespace spdlog
