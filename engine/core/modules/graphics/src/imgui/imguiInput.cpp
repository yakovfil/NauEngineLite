// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved

#include "nau/gui/imguiInput.h"

#include <imgui.h>

#include <memory>
#include <utility>

#include "imguiInputHandler.h"
#include "nau/3d/dag_drv3d.h"
#include "nau/diag/logging.h"
#include "nau/gui/dag_imgui.h"
#include "nau/input.h"

void register_hid_event_handler(DearImGuiInputHandler* handler)
{
    gainput::InputManager* inputManager = reinterpret_cast<gainput::InputManager*>(nau::input::getGaInputManager());
    handler->listenerId = inputManager->AddListener(handler);
}


void unregister_hid_event_handler(DearImGuiInputHandler* handler)
{
    gainput::InputManager* inputManager = reinterpret_cast<gainput::InputManager*>(nau::input::getGaInputManager());
    inputManager->RemoveListener(handler->listenerId);
    handler->listenerId = -1;
}

static std::unique_ptr<DearImGuiInputHandler> imgui_input_handler;
static GlobalInputHandler imgui_global_input_handler;
static nau::math::IVector2 saved_mouse_pos = nau::math::IVector2(0, 0);
static bool saved_draw_cursor = false;
static bool hybrid_input_mode = false;


static void on_imgui_state_change(ImGuiState old_state, ImGuiState new_state)
{
  if (!imgui_input_handler)
  {
      imgui_input_handler = std::make_unique<DearImGuiInputHandler>();
      imgui_input_handler->hybridInput = hybrid_input_mode;
      int w, h;
      d3d::get_screen_size(w, h);
      saved_mouse_pos.setX(w);
      saved_mouse_pos.setY(h);
      saved_mouse_pos /= 2;
  }

  if (new_state == ImGuiState::ACTIVE)
  {
    register_hid_event_handler(imgui_input_handler.get());
    ImGui::GetIO().MouseDrawCursor = saved_draw_cursor;
    ImGui::GetIO().MousePos = ImVec2(saved_mouse_pos.getX(), saved_mouse_pos.getY());
  }
  else if (old_state == ImGuiState::ACTIVE)
  {
    saved_draw_cursor = ImGui::GetIO().MouseDrawCursor;
    ImGui::GetIO().MouseDrawCursor = false;
    saved_mouse_pos = nau::math::IVector2(ImGui::GetMousePos().x, ImGui::GetMousePos().y);
    unregister_hid_event_handler(imgui_input_handler.get());
  }
}


static void request_imgui_state(ImGuiState new_state)
{
  static bool registered = false;
  if (!registered)
  {
    imgui_register_on_state_change_handler(on_imgui_state_change);
    registered = true;
  }
  imgui_request_state_change(new_state);
}


void imgui_switch_state()
{
    request_imgui_state(imgui_get_state() != ImGuiState::ACTIVE ? ImGuiState::ACTIVE : ImGuiState::OFF);
}


void imgui_switch_overlay()
{
    request_imgui_state(imgui_get_state() != ImGuiState::OVERLAY ? ImGuiState::OVERLAY : ImGuiState::OFF);
}


const nau::math::IVector2& imgui_get_saved_mouse_pos()
{
    return saved_mouse_pos;
}


bool imgui_handle_special_keys_down(bool ctrl, bool shift, bool alt, int btn_idx, unsigned int key_modif)
{
  if (imgui_get_state() == ImGuiState::ACTIVE && imgui_global_input_handler != nullptr)
  {
    if (imgui_global_input_handler(/*key_down*/ true, btn_idx, key_modif))
      return true;
  }
  return false;
}


bool imgui_handle_special_keys_up(bool, bool, bool, int btn_idx, unsigned key_modif)
{
  if (imgui_get_state() == ImGuiState::ACTIVE && imgui_global_input_handler != nullptr)
  {
    if (imgui_global_input_handler(/*key_down*/ false, btn_idx, key_modif))
      return true;
  }
  return false;
}


void imgui_register_global_input_handler(GlobalInputHandler handler)
{
    imgui_global_input_handler = std::move(handler);
}


bool imgui_in_hybrid_input_mode()
{
    return hybrid_input_mode;
}


void imgui_use_hybrid_input_mode(bool value)
{
  hybrid_input_mode = value;
  if (imgui_input_handler)
    imgui_input_handler->hybridInput = value;
}


void imgui_set_viewport_offset(int offsetX, int offsetY)
{
  if (imgui_input_handler)
  {
    imgui_input_handler->viewPortOffsetX = offsetX;
    imgui_input_handler->viewPortOffsetY = offsetY;
  }
  else
  {
    NAU_LOG_ERROR("imgui_input_handler == null, call imgui initialization");
  }
}


void imgui_draw_mouse_cursor(bool draw_mouse_cursor)
{
  if (imgui_input_handler)
  {
    imgui_input_handler->drawMouseCursor = draw_mouse_cursor;
  }
  else
  {
    NAU_LOG_ERROR("imgui_input_handler == null, call imgui initialization");
  }
}

