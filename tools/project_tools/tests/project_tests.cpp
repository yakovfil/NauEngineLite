// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#include <gmock/gmock.h>
#include <gtest/gtest-param-test.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <string>

#include "nau/diag/logging.h"

#include "nau/shared/args.h"
#include "nau/shared/file_system.h"
#include "nau/shared/logger.h"
#include "nau/shared/util.h"

#include "nlohmann/json.hpp"
#include "nau/project_tools/project_api.h"
#include "nau/project_tools/project_template.h"
#include "nau/project_tools/string_processor.h"

namespace nau::test
{
    constexpr static char g_pattern[] = "%%\\((.*?)\\)%%";

    TEST(ProjectToolTests, EnsureTemplateExsitTest)
    {
        FileSystem fs;
        const std::filesystem::path toolsPath = FileSystemExtensions::findDirectoryInUpperRecursive(std::filesystem::current_path().string(), "project_templates");
        ASSERT_FALSE(toolsPath.empty());
        const std::filesystem::path pathToTemplate = toolsPath / "project_templates" / "empty";
        ASSERT_TRUE(fs.exist(pathToTemplate));
    }

    TEST(ProjectToolTests, EnsureConfigExsitTest)
    {
        FileSystem fs;
        const std::filesystem::path toolsPath = FileSystemExtensions::findDirectoryInUpperRecursive(std::filesystem::current_path().string(), "project_templates");
        ASSERT_FALSE(toolsPath.empty());
        const std::filesystem::path configPath = toolsPath / "project_templates" / "empty" / "project.nauproject";
        nlohmann::json config;
        std::stringstream ss;
        fs.readFile(configPath, ss);
        config = nlohmann::json::parse(ss.str());
        ASSERT_TRUE(config.is_object());
    }

    TEST(ProjectToolTests, StringProcessorTest)
    {
        FileSystem fs;
        const std::filesystem::path toolsPath = FileSystemExtensions::findDirectoryInUpperRecursive(std::filesystem::current_path().string(), "project_templates");
        const std::filesystem::path path = toolsPath / "project_templates" / "empty";

        ASSERT_TRUE(fs.exist(path));

        NauProjectParams projectParams = {
            { "ProjectPath",    path.string()},
            { "ProjectName",           "TEST"},
            { "Version",              "0.0.1"},
            {"TemplateName",          "empty"},
            { "CMakePreset", "win_vs2026_x64"},
            { "ContentOnly",          "false"}
        };

        const NauProjectTemplate projectTemplate = NauProjectTemplate("empty", path, projectParams, &fs);

        ASSERT_TRUE(projectTemplate.isValid());

        StringProcessor processor;

        for(auto& fileInfo : projectTemplate.getFiles())
        {
            if(fileInfo.updateContent)
            {
                std::stringstream content;

                ASSERT_TRUE(fs.readFile(fileInfo.path, content));

                int counter = 0;

                try
                {
                    [[maybe_unused]] std::string processedContent = processor.processRegexMatches(g_pattern, content.str(), projectParams, counter);
                }
                catch(const std::exception& e)
                {
                    FAIL() << e.what();
                }

                ASSERT_TRUE(counter == 0);
            }
        }
    }

    class VersionsFixture : public ::testing::TestWithParam<std::string>
    {
    };

    TEST(ProjectToolTests, VersionsEqualsHashTest)
    {
        NauVersion version = NauVersion("1.0");
        NauVersion compareVer = NauVersion("1.0.0");

        ASSERT_TRUE(version.getHash() == compareVer.getHash());
    }

    TEST_P(VersionsFixture, VersionTest)
    {
        NauVersion version = NauVersion("1.0");
        NauVersion compareVer = NauVersion(GetParam());
        ASSERT_TRUE(version < compareVer);
        ASSERT_TRUE(version.getHash() != compareVer.getHash());
    }

    INSTANTIATE_TEST_SUITE_P(
        VersionsTests,
        VersionsFixture,
        ::testing::Values(
            "1.1",
            "1.2",
            "1.34.3",
            "1.5",
            "1.08.2",
            "2.0"));

    TEST_P(VersionsFixture, ProjectUpgradeVersionTest)
    {
        FileSystem fs;
        const std::filesystem::path toolsPath = FileSystemExtensions::findDirectoryInUpperRecursive(std::filesystem::current_path().string(), "project_templates");
        ASSERT_FALSE(toolsPath.empty());
        const std::filesystem::path configPath = toolsPath / "project_templates" / "empty" / "project.nauproject";
        nlohmann::json config;
        std::stringstream ss;
        ASSERT_TRUE(fs.readFile(configPath, ss));
        config = nlohmann::json::parse(ss.str());
        ASSERT_TRUE(config.is_object());
        NauVersion projectVersion = NauVersion(config["ProjectVersion"].get<std::string>());
        NauVersion compareVer = NauVersion(GetParam());
        ASSERT_FALSE(compareVer < projectVersion);
    }

    class JsonCfgFixture : public ::testing::TestWithParam<std::string>
    {
    };

    TEST_P(JsonCfgFixture, JsonMergeTest)
    {
        FileSystem fs;
        const std::filesystem::path toolsPath = FileSystemExtensions::findDirectoryInUpperRecursive(std::filesystem::current_path().string(), "project_templates");
        ASSERT_FALSE(toolsPath.empty());
        const std::filesystem::path configPath = toolsPath / "project_templates" / "empty" / "project.nauproject";
        nlohmann::json config;
        std::stringstream ss;
        ASSERT_TRUE(fs.readFile(configPath, ss));
        config = nlohmann::json::parse(ss.str());
        ASSERT_TRUE(config.is_object());

        nlohmann::json newConfig = nlohmann::json::parse(GetParam());

        if(newConfig.contains("Project"))
        {
            ASSERT_TRUE(config["Project"] == newConfig["Project"]);
        }

        // Prohibited downgrade test
        if(newConfig.contains("ProjectVersion"))
        {
            NauVersion currentVersion = NauVersion(config["ProjectVersion"].get<std::string>());
            NauVersion newVersion = NauVersion(newConfig["ProjectVersion"].get<std::string>());

            ASSERT_TRUE(currentVersion < newVersion);
        }

        util::mergeJsonRecursive(config, newConfig);

        // Verify merging

        for(auto& [key, value] : newConfig.items())
        {
            ASSERT_TRUE(config.contains(key));
            ASSERT_TRUE(config[key] == value);
        }
    }

    INSTANTIATE_TEST_SUITE_P(
        JsonTests,
        JsonCfgFixture,
        ::testing::Values(
            "{\"Engine\": {\r\n    \"Version\": \"0.1\"\r\n  },\r\n\r\n  \"ProjectName\": \"%%(ProjectName)%%\"}",
            "{\"Engine\": {\r\n    \"Version\": \"1.1\"\r\n  },\r\n\r\n  \"Test2\": \"SomeField\"}",
            "{\"Engine\": {\r\n    \"Version\": \"1.12\"\r\n  },\r\n\r\n  \"ProjectVersion\": \"4.42.2.1653\"}",
            "{\"Engine\": {\r\n    \"Version\": \"0.1\",\r\n    \"Sub\": \"2\"\r\n  },\r\n  \"Field\": \"A fancy field\"}",
            "{\"Engine\": {\r\n    \"Version\": \"0.1\",\r\n    \"SubObj\": [\"1\", \"2\"]\r\n  },\r\n  \"FieldObj\": {\"Test\":\"1\"}}"));
}  // namespace nau::test