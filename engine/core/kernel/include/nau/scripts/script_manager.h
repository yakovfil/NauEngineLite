// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once
#include <span>
#include <string_view>

#include "nau/dispatch/class_descriptor.h"
#include "nau/dispatch/dispatch.h"
#include "nau/io/fs_path.h"
#include "nau/rtti/type_info.h"
#include "nau/utils/functor.h"
#include "nau/utils/result.h"

namespace nau::scripts
{
    /**
     */
    struct NAU_ABSTRACT_TYPE ScriptManager
    {
        NAU_TYPEID(nau::scripts::ScriptManager)

        virtual ~ScriptManager() = default;

        virtual Result<Ptr<>> executeScriptFromBytes(const char* scriptName, std::span<const std::byte> scriptCode) = 0;

        virtual Result<Ptr<>> executeScriptFromFile(const io::FsPath& path) = 0;

        virtual void registerClass(IClassDescriptor::Ptr classDescriptor) = 0;

        virtual Result<> invokeGlobal(std::string_view method, DispatchArguments args, Functor<void(const nau::Ptr<>& result)>) = 0;

        virtual Result<Ptr<IDispatch>> createScriptInstance(std::string_view scriptClass) = 0;

        virtual void addScriptSearchPath(io::FsPath path) = 0;

        virtual void addScriptFileExtension(std::string_view ext) = 0;

        template <typename T>
        void registerNativeClass()
        {
            this->registerClass(getClassDescriptor<T>());
        }
    };

}  // namespace nau::scripts
