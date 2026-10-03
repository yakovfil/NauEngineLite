// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once
#include <gainput/gainput.h>

#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "nau/app/main_loop/game_system.h"
#include "nau/input_system.h"
#include "nau/rtti/rtti_impl.h"
#include "nau/service/service.h"
#include "nau/utils/functor.h"
#include "signals/input_signals_impl.h"

namespace nau
{
    class InputSystemImpl final : public IServiceInitialization,
                                  public IInputSystem,
                                  public IGamePreUpdate
    {
        NAU_RTTI_CLASS(InputSystemImpl, IServiceInitialization, IInputSystem, IGamePreUpdate)
    public:
        InputSystemImpl();

        virtual std::shared_ptr<IInputAction> addAction(const std::string& serialized, nau::Functor<void(IInputSignal*)> actionCallback) override;
        virtual std::shared_ptr<IInputAction> addAction(const std::string& name, IInputAction::Type type, IInputSignal* signal, nau::Functor<void(IInputSignal*)> actionCallback) override;
        virtual bool removeAction(std::shared_ptr<IInputAction>&& action) override;

        virtual std::shared_ptr<IInputAction> loadAction(const io::FsPath& filePath, nau::Functor<void(IInputSignal*)> actionCallback) override;
        virtual bool saveAction(const std::shared_ptr<IInputAction> action, const std::string& filePath) override;
        virtual void getActions(std::vector<std::shared_ptr<IInputAction>>& actions) override;

        virtual IInputSignal* createSignal(const std::string& signalType) override;
        virtual IInputSignal* createSignal(const std::string& signalType, const std::string& controllerName, nau::Functor<void(IInputSignal*)> signal) override;

        virtual void setContext(const std::string& context) override;
        virtual void addContext(const std::string& context) override;
        virtual void removeContext(const std::string& context) override;

        IInputController* getController(const std::string& controllerDesc) override;
        std::vector<IInputDevice*> getDevices() override;

        void gamePreUpdate(std::chrono::milliseconds dt) override;

        gainput::InputManager& getGainput()
        {
            return m_inputManager;
        }
        void setInputSource(const std::string& source) override;

    private:
        class InputSignalFactory
        {
        public:
            static InputSignalImpl* create(const std::string& type);
        };
        gainput::InputManager m_inputManager;
        std::vector<std::shared_ptr<IInputDevice>> m_devices;
        std::unordered_map<std::string, std::shared_ptr<IInputController>> m_controllers;
        std::multimap<std::string, std::shared_ptr<IInputAction>> m_actions;
        std::set<std::string> m_contexts;

        std::string m_currentSource;
        std::set<std::string> m_sources;
    };
}  // namespace nau
