// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/components/misc_system.h"

#include <iostream>

#include "SDL3/SDL_messagebox.h"
#include "SDL3/SDL_misc.h"
#include "SDL3/SDL_platform.h"

namespace content {

MiscSystem::MiscSystem(base::WeakPtr<ui::Widget> window) : window_(window) {}

MiscSystem::~MiscSystem() = default;

std::string MiscSystem::GetPlatform(ExceptionState& exception_state) {
  return SDL_GetPlatform();
}

void MiscSystem::OpenURL(const std::string& path,
                         ExceptionState& exception_state) {
  if (!SDL_OpenURL(path.c_str()))
    exception_state.ThrowError(ExceptionCode::CONTENT_ERROR, "OpenURL: %s",
                               SDL_GetError());
}

std::string MiscSystem::Gets(ExceptionState& exception_state) {
  std::string in;
  std::cin >> in;
  return in;
}

void MiscSystem::Puts(const std::string& message,
                      ExceptionState& exception_state) {
  LOG(INFO) << message;
}

void MiscSystem::Alert(const std::string& message,
                       ExceptionState& exception_state) {
  SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION,
                           window_->GetTitle().c_str(), message.c_str(),
                           window_->AsSDLWindow());
}

bool MiscSystem::Confirm(const std::string& message,
                         ExceptionState& exception_state) {
  std::string title = window_->GetTitle();

  SDL_MessageBoxData messagebox_data;
  messagebox_data.flags =
      SDL_MESSAGEBOX_INFORMATION | SDL_MESSAGEBOX_BUTTONS_RIGHT_TO_LEFT;
  messagebox_data.window = window_->AsSDLWindow();
  messagebox_data.title = title.c_str();
  messagebox_data.message = message.c_str();

  SDL_MessageBoxButtonData buttons[2];
  buttons[0].flags = SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT |
                     SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT;
  buttons[0].buttonID = 0;
  buttons[0].text = "Cancel";
  buttons[1].flags = 0;
  buttons[1].buttonID = 1;
  buttons[1].text = "OK";

  messagebox_data.numbuttons = std::size(buttons);
  messagebox_data.buttons = buttons;

  int button_id = 0;
  SDL_ShowMessageBox(&messagebox_data, &button_id);

  return button_id;
}

}  // namespace content
