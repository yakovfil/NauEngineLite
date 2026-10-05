// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

#include "nau/app/application.h"
#include "nau/app/application_delegate.h"
#include "nau/app/background_work_service.h"
#include "nau/app/run_application.h"
#include "nau/async/work_queue.h"
#include "nau/diag/log_subscribers.h"
#include "nau/io/virtual_file_system.h"
#include "nau/runtime/internal/runtime_component.h"
#include "nau/service/service_provider.h"

namespace
{
    class Background final : public nau::BackgroundWorkService,
                             public nau::IRttiObject
    {
        NAU_RTTI_CLASS(Background, nau::BackgroundWorkService, nau::IRttiObject)
    public:
        nau::async::Executor::Ptr getExecutor() override
        {
            return queue;
        }
        nau::WorkQueue::Ptr queue = nau::WorkQueue::create();
    };
    class LoggingDelegate final : public nau::ApplicationDelegate
    {
    public:
        nau::Result<> configureApplication() override
        {
            return {};
        }
        void onApplicationInitialized() override
        {
        }
        nau::async::Task<> startupApplication() override
        {
            co_return;
        }
        std::string getModulesListString() const override
        {
            return NAU_MODULES_LIST;
        }
        nau::Result<> initializeServices() override
        {
            auto service = std::make_unique<Background>();
            background = service.get();
            nau::getServiceProvider().addService(std::move(service));
            nau::getServiceProvider().addService(nau::io::createVirtualFileSystem());
            return {};
        }
        void onApplicationPreUpdate(std::chrono::milliseconds) override
        {
            EXPECT_TRUE(nau::getServiceProvider().has<nau::io::IFileSystem>());
            EXPECT_TRUE(nau::getApplication().hasExecutor());
            const auto directory = std::filesystem::absolute("file-log-regression-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
            std::filesystem::create_directories(directory);
            auto subscriber = nau::diag::createFileOutputLogSubscriber((directory / "writes").string());
            auto caller = nau::WorkQueue::create();
            {
                nau::async::Executor::InvokeGuard guard(*caller);
                for (int i = 0; i < 50; ++i)
                    subscriber->processMessage({uint32_t(i), 0, nau::diag::LogLevel::Info, {}, {}, "accepted-before-stop"});
            }
            background->queue->poll();
            EXPECT_FALSE(caller->as<nau::IRuntimeComponent&>().hasWorks());
            // A disposed background service remains discoverable briefly.
            background->queue = nullptr;
            subscriber->processMessage({51, 0, nau::diag::LogLevel::Info, {}, {}, "late-after-background-stop"});
            subscriber.reset();
            bool found = false;
            for (const auto& file : std::filesystem::directory_iterator(directory))
            {
                if (!file.path().filename().string().starts_with("writes."))
                    continue;
                std::ifstream input(file.path());
                std::string contents((std::istreambuf_iterator<char>(input)), {});
                found |= contents.find("accepted-before-stop") != contents.npos && contents.find("late-after-background-stop") != contents.npos;
            }
            EXPECT_TRUE(found);
            nau::getApplication().stop();
        }
        Background* background = nullptr;
    };
}  // namespace

TEST(FileLogging, DrainsWithoutResumingStoppedCaller)
{
    EXPECT_EQ(nau::runApplication(std::make_unique<LoggingDelegate>()), 0);
}
