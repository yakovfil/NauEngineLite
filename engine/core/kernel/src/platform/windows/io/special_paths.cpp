// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nau/io/special_paths.h"

#include <ShlObj.h>
#include <tchar.h>

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

#include "nau/diag/assertion.h"
#include "nau/diag/logging.h"
#include "nau/memory/standard_aliases.h"
#include "nau/memory/stack_allocator.h"
#include "nau/platform/windows/diag/win_error.h"
#include "nau/string/string_conv.h"

namespace nau::io
{
    namespace
    {
        /**
            see: https://learn.microsoft.com/en-us/windows/win32/api/objbase/nf-objbase-coinitialize
         */
        struct CoInitializeThreadGuard
        {
            CoInitializeThreadGuard()
            {
                const HRESULT coInitRes = ::CoInitialize(nullptr);
                NAU_VERIFY(coInitRes == S_OK || coInitRes == S_FALSE);
            }

            ~CoInitializeThreadGuard()
            {
                ::CoUninitialize();
            }
        };

        std::optional<KNOWNFOLDERID> knownFolderEnum2Id(KnownFolder folder)
        {
            if (folder == KnownFolder::UserDocuments)
            {
                return FOLDERID_Documents;
            }
            else if (folder == KnownFolder::LocalAppData)
            {
                return FOLDERID_LocalAppData;
            }
            else if (folder == KnownFolder::UserHome)
            {
                return FOLDERID_Profile;
            }

            return std::nullopt;
        }

        std::filesystem::path getKnownFolderPathById(KNOWNFOLDERID folderId)
        {
            // CO must be initialized per thread.
            static thread_local const CoInitializeThreadGuard coInitGuard;

            IKnownFolderManager* folderManager = nullptr;
            IKnownFolder* knownFolder = nullptr;
            wchar_t* folderPathBuffer = nullptr;

            scope_on_leave
            {
                if (folderManager)
                {
                    folderManager->Release();
                }

                if (knownFolder)
                {
                    knownFolder->Release();
                }

                if (folderPathBuffer)
                {
                    ::CoTaskMemFree(folderPathBuffer);
                }
            };

            HRESULT hr = ::CoCreateInstance(CLSID_KnownFolderManager, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&folderManager));
            if (FAILED(hr))
            {
                return {};
            }

            hr = folderManager->GetFolder(folderId, &knownFolder);
            if (FAILED(hr))
            {
                return {};
            }

            hr = knownFolder->GetPath(KF_FLAG_DEFAULT, &folderPathBuffer);
            if (FAILED(hr))
            {
                return {};
            }

            const size_t len = wcslen(folderPathBuffer);
            return std::wstring_view{folderPathBuffer, len};
        }

        std::filesystem::path getExecutableLocation()
        {
            [[maybe_unused]] constexpr size_t MaxModulePathLen = 2048;

            StackAllocatorUnnamed;

            StackVector<wchar_t> exeModulePath;
            exeModulePath.resize(MAX_PATH);

            DWORD len = 0;
            DWORD err = 0;

            do
            {
                len = GetModuleFileNameW(nullptr, exeModulePath.data(), static_cast<DWORD>(exeModulePath.size()));
                err = GetLastError();
                NAU_ASSERT(err == 0 || err == ERROR_INSUFFICIENT_BUFFER);

                if (err == ERROR_INSUFFICIENT_BUFFER)
                {
                    NAU_FATAL(exeModulePath.size() < MaxModulePathLen);
                    exeModulePath.resize(exeModulePath.size() * 2);
                }
            } while (err != 0);

            std::wstring_view pathStr{exeModulePath.data(), static_cast<size_t>(len)};

            const auto sepPos = pathStr.rfind(std::filesystem::path::preferred_separator);
            NAU_FATAL(sepPos != std::wstring_view::npos);

            pathStr.remove_suffix(pathStr.size() - sepPos);

            return pathStr;
        }

    }  // namespace

    std::u8string getNativeTempFilePath(std::u8string_view prefixFileName)
    {
        wchar_t tempDirectoryPathBuffer[MAX_PATH];
        wchar_t tempFilePathBuffer[MAX_PATH];

        DWORD tempDirectoryPathLength = GetTempPathW(MAX_PATH, tempDirectoryPathBuffer);
        if (tempDirectoryPathLength > MAX_PATH || (tempDirectoryPathLength == 0))
        {
            NAU_ASSERT("GetTempPath failed");
            return {};
        }

        UINT result = GetTempFileNameW(tempDirectoryPathBuffer, TEXT(strings::utf8ToWString(prefixFileName).data()), 0, tempFilePathBuffer);
        if (result == 0)
        {
            NAU_ASSERT("GetTempFileName failed");
            return {};
        }

        return strings::wstringToUtf8(tempFilePathBuffer);
    }

    std::filesystem::path getKnownFolderPath(KnownFolder folder)
    {
        namespace fs = std::filesystem;

        static std::shared_mutex knownFoldersMutex;
        static std::unordered_map<KnownFolder, fs::path> knownFolders;

        {
            shared_lock_(knownFoldersMutex);
            if (auto folderPath = knownFolders.find(folder); folderPath != knownFolders.end())
            {
                return folderPath->second;
            }
        }

        lock_(knownFoldersMutex);
        if (auto folderPath = knownFolders.find(folder); folderPath != knownFolders.end())
        {
            return folderPath->second;
        }

        if (folder == KnownFolder::Temp)
        {
            wchar_t tempFolderPath[MAX_PATH];
            const DWORD len = ::GetTempPathW(MAX_PATH, tempFolderPath);
            if (len == 0)
            {
                NAU_LOG_ERROR("Fail to get temp path:{}", diag::getWinErrorMessageA(diag::getAndResetLastErrorCode()));
                return {};
            }

            return std::wstring_view{tempFolderPath, static_cast<size_t>(len)};
        }
        else if (folder == KnownFolder::Current)
        {
            return fs::current_path();
        }
        else if (folder == KnownFolder::ExecutableLocation)
        {
            return getExecutableLocation();
        }

        const std::optional<KNOWNFOLDERID> folderId = knownFolderEnum2Id(folder);
        if (!folderId)
        {
            return {};
        }

        fs::path folderPath = getKnownFolderPathById(*folderId);
        if (!folderPath.empty())
        {
            knownFolders[folder] = folderPath;
        }

        return folderPath;
    }
}  // namespace nau::io
