// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <memory>
#include <unordered_map>
#include <vector>

#include "nau/input_system.h"
#include "nau/platform/windows/app/window_message_handler.h"
#include "nau/rtti/rtti_impl.h"

namespace nau::input
{
    class InputSystemImplWin final : public IWindowMessageHandler,
                                     public IWindowsApplicationMessageHandler,
                                     public IInputSourceManager,
                                     public IRttiObject
    {
        NAU_RTTI_CLASS(InputSystemImplWin, IWindowMessageHandler, IWindowsApplicationMessageHandler, IInputSourceManager, IRttiObject)

        virtual void setGetSources(nau::Functor<void(std::vector<std::shared_ptr<InputSource>>&)>) override;

    private:
        virtual bool handleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) override;
        virtual PreDispatchMsgResult preDispatchMsg(MSG& msg) override;
        virtual void postDispatchMsg(const MSG& msg) override;

        std::unordered_map<HWND, std::shared_ptr<InputSource>> m_windowMapping;
        HWND m_currentWindow = 0;
        nau::Functor<bool(InputSource*)> m_onInputSource;
        nau::Functor<void(std::vector<std::shared_ptr<InputSource>>&)> m_getSources;
    };
}  // namespace nau::input
