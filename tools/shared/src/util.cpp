// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#include "nau/shared/util.h"

#ifdef _WIN32
    #include "nau/shared/platform/win/process.h"
    #include "nau/shared/platform/win/utils.h"
#elif defined(__linux__) || defined(__linux)
    #include "nau/shared/platform/linux/process.h"
    #include "nau/shared/platform/linux/utils.h"
#endif

#include <unordered_map>

#include "nau/shared/logger.h"

namespace nau
{
    namespace util
    {
        static const std::unordered_map<std::string, std::string> g_presetToFileExtLookup = {
            {    "win_vs2026_x64", "sln"},
            {"win_vs2026_x64_dll", "sln"}
        };

        std::string unescape(const std::string& input)
        {
            std::string output;
            output.reserve(input.size());

            for (size_t index = 0; index < input.length(); index++)
            {
                if (input[index] == '\\' && index + 1 < input.length())
                {
                    switch (input[index + 1])
                    {
                        case 'n':
                            output += '\n';
                            index++;
                            break;

                        case 't':
                            output += '\t';
                            index++;
                            break;

                        case '\\':
                            output += '\\';
                            index++;
                            break;

                        case '"':
                            output += '"';
                            index++;
                            break;

                        default:
                            output += input[index];
                            break;
                    }
                }
                else
                {
                    output += input[index];
                }
            }

            return output;
        }

        std::string getIdeExtension(const std::string& presetName)
        {
            if (g_presetToFileExtLookup.find(presetName) == g_presetToFileExtLookup.end())
            {
#ifdef _WIN32
                return ".sln";
#elif defined(__linux__) || defined(__linux)
                throw std::runtime_error("Not implemented");
#endif
            }

            return g_presetToFileExtLookup.at(presetName);
        }

        void mergeJsonRecursive(nlohmann::json& target, const nlohmann::json& source)
        {
            for (auto it = source.begin(); it != source.end(); ++it)
            {
                if (target.find(it.key()) == target.end())
                {
                    target[it.key()] = it.value();
                }
                else if (!target[it.key()].is_object())
                {
                    target[it.key()] = it.value();
                }
                else
                {
                    if (it.value().is_object() && target[it.key()].is_object())
                    {
                        mergeJsonRecursive(target[it.key()], it.value());
                    }
                }
            }
        }

        bool SHARED_API validateEnvironment()
        {
            return isVisualStudioInstalled() && checkEnvironmentVariables();
        }

        bool isVisualStudioInstalled()
        {
#ifdef _WIN32
            constexpr const char* vsWherePath = "C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe";

            if (!std::filesystem::exists(vsWherePath))
            {
                LOG_WARN("vswhere.exe cant be found at path {}!", vsWherePath);

                return false;
            }

            const std::string cmd = std::format("{} -latest -property installationPath", vsWherePath);

            IProcessWorker process;

            const int processResult = process.runProcess(cmd);
            const std::string resultPaths = process.getProcessOutput();

            bool existOnDisk = false;

            // Check is resultPaths has new line simbol
            if (resultPaths.find('\n') == std::string::npos)
            {
                existOnDisk = std::filesystem::exists(resultPaths);
            }
            else
            {
                std::stringstream ss(resultPaths);
                if (ss.good())
                {
                    std::string substr;

                    while (std::getline(ss, substr, '\n'))
                    {
                        if (std::filesystem::exists(substr))
                        {
                            existOnDisk = true;
                            break;
                        }
                    }
                }
            }

            const bool result = processResult == 0 && existOnDisk;

            if (!result)
            {
                LOG_WARN("Visual Studio is not installed!");
            }

            return result;
#elif defined(__linux__) || defined(__linux)
            throw std::runtime_error("Not implemented");
#endif
        }

        bool checkEnvironmentVariables()
        {
            const char* engineSdkEnv = std::getenv("NAU_ENGINE_SDK_DIR");

            if (engineSdkEnv != nullptr)
            {
                LOG_INFO("Environment variable NAU_ENGINE_SDK_DIR is set");

                if (std::filesystem::exists(engineSdkEnv))
                {
                    LOG_INFO("NAU_ENGINE_SDK_DIR:{}", engineSdkEnv);
                    return true;
                }
                else
                {
                    LOG_WARN("Environment variable NAU_ENGINE_SDK_DIR is not valid path {}!", engineSdkEnv);
                }
            }
            else
            {
                LOG_WARN("Environment variable NAU_ENGINE_SDK_DIR is not set");
            }

            const char* engineSourceEnv = std::getenv("NAU_ENGINE_SOURCE_DIR");

            if (engineSourceEnv != nullptr)
            {
                LOG_INFO("Environment variable NAU_ENGINE_SOURCE_DIR is set");

                if (std::filesystem::exists(engineSourceEnv))
                {
                    LOG_INFO("NAU_ENGINE_SOURCE_DIR:{}", engineSourceEnv);
                    return true;
                }
                else
                {
                    LOG_WARN("Environment variable NAU_ENGINE_SOURCE_DIR is not valid path {}!", engineSourceEnv);
                }
            }
            else
            {
                LOG_WARN("Environment variable NAU_ENGINE_SOURCE_DIR is not set");
            }

            return false;
        }
    }  // namespace util
}  // namespace nau