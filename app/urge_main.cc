// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "SDL3/SDL_main.h"
#include "SDL3_image/SDL_image.h"
#include "SDL3_ttf/SDL_ttf.h"

#include "binding/mri/mri_main.h"
#include "content/worker/content_runner.h"

int SDL_main(int argc, char* argv[]) {
  ::SetConsoleCP(CP_UTF8);

  SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO | SDL_INIT_AUDIO);
  TTF_Init();

  {
    std::unique_ptr<content::ContentProfile> profile =
        content::ContentProfile::MakeFrom(nullptr);

    std::unique_ptr<ui::Widget> widget(new ui::Widget);
    ui::Widget::InitParams widget_params;
    widget_params.size = base::Vec2i(640, 480);
    widget_params.resizable = true;
    widget->Init(std::move(widget_params));

    content::ContentRunner::InitParams content_params;
    content_params.profile = std::move(profile);
    content_params.window = widget->AsWeakPtr();
    content_params.entry = std::make_unique<binding::BindingEngineMri>();

    std::unique_ptr<content::ContentRunner> runner =
        content::ContentRunner::Create(std::move(content_params));
    for (;;) {
      if (!runner->RunMainLoop())
        break;
    }
  }

  TTF_Quit();
  SDL_Quit();

  return 0;
}
