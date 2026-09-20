// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <cstdio>

#include "./application_impl.h"
#include "./global_properties_impl.h"
#include "./logging_service.h"
#include "nau/app/application_services.h"
#include "nau/app/background_work_service.h"
#include "nau/diag/device_error.h"
#include "nau/io/special_paths.h"
#include "nau/io/virtual_file_system.h"
#include "nau/string/string_conv.h"

namespace nau
{
    Result<> initAndApplyConfiguration(ApplicationInitDelegate& initDelegate);

    eastl::unique_ptr<IRttiObject> createBackgroundWorkService();

    Result<> setupCoreServicesAndConfigure(ApplicationInitDelegate& initDelegate)
    {
        // diagnostics:
        diag::setDeviceError(diag::createDefaultDeviceError());
        eastl::unique_ptr<LoggingService> loggingService = eastl::make_unique<LoggingService>();

        setDefaultServiceProvider(createServiceProvider());

        // core basic services:
        ServiceProvider& serviceProvider = getServiceProvider();

        serviceProvider.addService(createBackgroundWorkService());
        serviceProvider.addService(std::move(loggingService));
        serviceProvider.addService(io::createVirtualFileSystem());
        serviceProvider.addService(AsyncMessageSource::create());
        serviceProvider.addService(eastl::make_unique<GlobalPropertiesImpl>());

        return initAndApplyConfiguration(initDelegate);
    }

    Result<> initAndApplyConfiguration(ApplicationInitDelegate& initDelegate)
    {
        auto& globalProps = getServiceProvider().get<GlobalProperties>();

        globalProps.addVariableResolver("folder", [](eastl::string_view folderStr) -> eastl::optional<eastl::string>
        {
            using namespace nau::strings;
            namespace fs = std::filesystem;

            io::KnownFolder folder = io::KnownFolder::Current;
            if (!parse(toStringView(folderStr), folder))
            {
                NAU_FAILURE("Bad known_folder value ({})", folderStr);
                return eastl::string{"BAD_FOLDER"};
            }

            const fs::path folderPath = io::getKnownFolderPath(folder);
            NAU_ASSERT(!folderPath.empty());

            const std::wstring wcsPath = folderPath.wstring();

            auto utf8String = wstringToUtf8(eastl::wstring_view{wcsPath.data(), wcsPath.size()});

            return eastl::string{reinterpret_cast<const char*>(utf8String.data()), utf8String.size()};
        });

        return initDelegate.configureApplication();
    }

    namespace
    {
        class TempAppInitDelegate : public ApplicationInitDelegate
        {
        public:
            TempAppInitDelegate(Functor<Result<>()>&& initServicesCallback) :
                m_initServicesCallback(std::move(initServicesCallback))
            {
            }

            Result<> configureApplication() override
            {
                return ResultSuccess;
            }

            Result<> initializeApplication() override
            {
                if (m_initServicesCallback)
                {
                    return m_initServicesCallback();
                }

                return ResultSuccess;
            }

        private:
            Functor<Result<>()> m_initServicesCallback;
        };

    }  // namespace

    Result<eastl::unique_ptr<Application>> createApplicationChecked(ApplicationInitDelegate& initDelegate)
    {
        if (applicationExists() || hasServiceProvider() || hasModuleManager())
        {
            return NauMakeError("Creation: application resources already exist");
        }

        auto runtime = RuntimeState::create();

        if (auto configuration = setupCoreServicesAndConfigure(initDelegate); !configuration)
        {
            IModuleManager::Ptr modules;
            const auto cleanup = cleanupFailedApplication(*runtime, modules);
            auto message = eastl::string("Configuration: ") + configuration.getError()->getDiagMessage();
            if (!cleanup)
            {
                message += "\nCleanup: ";
                message += cleanup.getError()->getDiagMessage();
            }
            return NauMakeError(message);
        }

        auto application = eastl::make_unique<ApplicationImpl>(std::move(runtime));

        if (Result<> initRes = initDelegate.initializeApplication(); !initRes)
        {
            const auto cleanup = application->abortCreation();
            auto message = eastl::string("Application setup: ") + initRes.getError()->getDiagMessage();
            if (!cleanup)
            {
                message += "\nCleanup: ";
                message += cleanup.getError()->getDiagMessage();
            }
            return NauMakeError(message);
        }

        return eastl::unique_ptr<Application>{std::move(application)};
    }

    eastl::unique_ptr<Application> createApplication(ApplicationInitDelegate& initDelegate)
    {
        auto result = createApplicationChecked(initDelegate);
        if (!result)
        {
            std::fprintf(stderr, "%s\n", result.getError()->getDiagMessage().c_str());
            return nullptr;
        }
        return std::move(*result);
    }

    eastl::unique_ptr<Application> createApplication(Functor<Result<>()> preInitCallback)
    {
        TempAppInitDelegate initDelegate(std::move(preInitCallback));
        return createApplication(initDelegate);
    }

}  // namespace nau
