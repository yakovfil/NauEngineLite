// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "input_system_impl.h"

#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "input_action_impl.h"
#include "input_controller_impl.h"
#include "input_devices_impl.h"
#include "nau/dag_ioSys/dag_chainedMemIo.h"
#include "nau/input.h"
#include "nau/io/virtual_file_system.h"
#include "nau/memory/bytes_buffer.h"
#include "nau/service/service_provider.h"
#include "signals/input_signals_impl.h"

namespace nau
{
    InputSystemImpl::InputSystemImpl()
    {
        m_devices.push_back(std::make_shared<GAKeyboardDevice>(m_inputManager));
        m_devices.push_back(std::make_shared<GAMouseDevice>(m_inputManager));
        // Pseudo controller for signal logic processing
        m_controllers.emplace("gate", std::make_shared<InputControllerImpl>("gate", nullptr));
    }

    IInputSignal* InputSystemImpl::createSignal(const std::string& signalType)
    {
        InputSignalImpl* signal = InputSignalFactory::create(signalType);
        if (signal != nullptr)
        {
            signal->generateName();
        }
        return signal;
    }

    IInputSignal* InputSystemImpl::createSignal(const std::string& signalType, const std::string& controllerName, nau::Functor<void(IInputSignal*)> signalCallback)
    {
        IInputSignal* signal = createSignal(signalType);
        if (signal)
        {
            if (!controllerName.empty())
            {
                IInputController* controller = getController(controllerName);
                if (controller != nullptr)
                {
                    signal->setController(controller);
                }
            }
            signalCallback(signal);
        }
        return signal;
    }

    std::shared_ptr<IInputAction> InputSystemImpl::addAction(const std::string& name, IInputAction::Type type, IInputSignal* signal, nau::Functor<void(IInputSignal*)> actionCallback)
    {
        auto actionPtr = std::make_shared<InputActionImpl>(std::move(actionCallback));
        actionPtr->setName(name);
        actionPtr->setType(type);
        actionPtr->setSignal(signal);
        m_actions.emplace(name, actionPtr);
        return actionPtr;
    }

    std::shared_ptr<IInputAction> InputSystemImpl::addAction(const std::string& serialized, nau::Functor<void(IInputSignal*)> actionCallback)
    {
        DataBlock block;
        if (!block.loadText(serialized.c_str(), serialized.size()))
        {
            NAU_LOG_WARNING("addAction: DataBlock loadText failed");
            return nullptr;
        }
        auto actionPtr = std::make_shared<InputActionImpl>(std::move(actionCallback));
        if (actionPtr->deserialize(&block))
        {
            m_actions.emplace(actionPtr->getName(), actionPtr);
            return actionPtr;
        }
        NAU_LOG_WARNING("addAction: deserialization failed");
        return nullptr;
    }

    bool InputSystemImpl::removeAction(std::shared_ptr<IInputAction>&& action)
    {
        for (auto it = m_actions.cbegin(); it != m_actions.cend(); ++it)
        {
            if (action == (*it).second)
            {
                m_actions.erase(it);
                return true;
            }
        }
        return false;
    }

    std::shared_ptr<IInputAction> InputSystemImpl::loadAction(const io::FsPath& filePath, nau::Functor<void(IInputSignal*)> actionCallback)
    {
        using namespace nau::io;
        auto& fileSystem = nau::getServiceProvider().get<IFileSystem>();
        auto file = fileSystem.openFile(filePath, AccessMode::Read, OpenFileMode::OpenExisting);
        if (file == nullptr)
        {
            NAU_LOG_WARNING("loadAction: file not found ({})", filePath.getCStr());
            return nullptr;
        }
        auto size = file->getSize();
        auto inputStream = file->createStream(AccessMode::Read);
        auto* reader = inputStream->as<IStreamReader*>();
        if (reader != nullptr)
        {
            nau::BytesBuffer buffer;
            buffer.resize(size + 1);
            nau::DataBlock block;
            auto res = reader->read(buffer.data(), size);
            if (!res.isError())
            {
                char* str = (char*)buffer.data();
                str[size] = '\0';
                auto& insys = nau::getServiceProvider().get<nau::IInputSystem>();
                return insys.addAction(str, std::move(actionCallback));
            }
        }
        NAU_LOG_WARNING("loadAction: can't load from file ({})", filePath.getCStr());
        return nullptr;
    }

    bool InputSystemImpl::saveAction(const std::shared_ptr<IInputAction> action, const std::string& filePath)
    {
        using namespace io;
        IStreamWriter::Ptr stream = io::createNativeFileStream(filePath.c_str(), io::AccessMode::Write, io::OpenFileMode::CreateAlways);
        if (stream)
        {
            DataBlock blk;
            action->serialize(&blk);
            nau::iosys::MemorySaveCB save;
            blk.saveToTextStream(save);
            auto* mem = save.getMem();
            while (mem != nullptr)
            {
                stream->write(reinterpret_cast<const std::byte*>(mem->data), mem->used).ignore();
                mem = mem->next;
            }
            return true;
        }
        return false;
    }

    void InputSystemImpl::getActions(std::vector<std::shared_ptr<IInputAction>>& actions)
    {
        actions.clear();
        for (auto& action : m_actions)
        {
            actions.push_back(action.second);
        }
    }

    void InputSystemImpl::setContext(const std::string& context)
    {
        m_contexts.clear();
        m_contexts.emplace(context);
    }

    void InputSystemImpl::addContext(const std::string& context)
    {
        m_contexts.emplace(context);
    }

    void InputSystemImpl::removeContext(const std::string& context)
    {
        m_contexts.erase(context);
    }

    IInputController* InputSystemImpl::getController(const std::string& controllerDesc)
    {
        for (auto& controller : m_controllers)
        {
            if (controller.first == controllerDesc)
            {
                return controller.second.get();
            }
        }
        for (auto& device : m_devices)
        {
            if (device->getName() == controllerDesc)
            {
                auto controller = std::make_shared<InputControllerImpl>(controllerDesc, device.get());
                m_controllers.emplace(controllerDesc, controller);
                return controller.get();
            }
        }
        return nullptr;
    }

    std::vector<IInputDevice*> InputSystemImpl::getDevices()
    {
        std::vector<IInputDevice*> devices;
        for (auto& device : m_devices)
        {
            devices.push_back(device.get());
        }
        return devices;
    }

    void InputSystemImpl::gamePreUpdate(std::chrono::milliseconds dtMs)
    {
        const float dt = static_cast<float>(dtMs.count()) / 1000.f;
        m_inputManager.Update();
        for (auto& controller : m_controllers)
        {
            controller.second->update(dt);
        }
        // Actions with no context tag first
        for (auto& action : m_actions)
        {
            if (action.second->isContextTag(""))
            {
                action.second->update(dt);
            }
        }
        if (!m_contexts.empty())
        {
            // Contexts can be changed in actions
            std::set<std::string> contexts = m_contexts;
            // Action with context tag processed once, even it have multiple tags
            std::set<IInputAction*> processed;
            for (auto& context : contexts)
            {
                for (auto& action : m_actions)
                {
                    auto* actionPtr = action.second.get();
                    if (processed.count(actionPtr) > 0)
                    {
                        continue;
                    }
                    if (actionPtr->isContextTag(context))
                    {
                        actionPtr->update(dt);
                        processed.emplace(actionPtr);
                    }
                }
            }
        }
    }

    void InputSystemImpl::setInputSource(const std::string& source)
    {
        if (m_currentSource != source)
        {
            removeContext(m_currentSource);
            addContext(source);
            m_currentSource = source;
            m_sources.emplace(source);
        }
    }

}  // namespace nau
