// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "SDL3/SDL_main.h"
#include "SDL3/SDL_messagebox.h"
#include "SDL3_image/SDL_image.h"
#include "SDL3_ttf/SDL_ttf.h"

#include "binding/mri/mri_main.h"
#include "components/filesystem/io_service.h"
#include "content/worker/content_runner.h"
#include "ui/widget/widget.h"

#if defined(OS_WIN)
#include <windows.h>
extern "C" {
__declspec(dllexport) DWORD NvOptimusEnablement = 1;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif  //! OS_WIN

namespace {

void ReplaceStringWidth(std::string& str, char before, char after) {
  for (size_t i = 0; i < str.size(); ++i)
    if (str[i] == before)
      str[i] = after;
}

}  // namespace

int SDL_main(int argc, char* argv[]) {
#if defined(OS_WIN)
  ::SetConsoleCP(CP_UTF8);
#endif

  std::string app(argv[0]);
  ReplaceStringWidth(app, '\\', '/');
  auto last_sep = app.find_last_of('/');
  if (last_sep != std::string::npos)
    app = app.substr(last_sep + 1);

  LOG(INFO) << "[App] Path: " << app;

  last_sep = app.find_last_of('.');
  if (last_sep != std::string::npos)
    app = app.substr(0, last_sep);
  std::string ini = app + ".ini";

  LOG(INFO) << "[App] Configure: " << ini;

  SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO | SDL_INIT_AUDIO);
  TTF_Init();

  std::unique_ptr<filesystem::IOService> iosystem =
      std::make_unique<filesystem::IOService>(argv[0]);
  iosystem->AddLoadPath(".");

  filesystem::IOState io_state;
  SDL_IOStream* inifile = iosystem->OpenReadRaw(ini, &io_state);
  if (io_state.error_count) {
    LOG(INFO) << "[App] Warning: " << io_state.error_message;
  }

  std::unique_ptr<content::ContentProfile> profile =
      content::ContentProfile::MakeFrom(inifile);
  profile->program_name = app;

  profile->LoadCommandLine(argc, argv);

  if (!profile->LoadConfigure(app)) {
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Engine",
                             "Error when parse configure file.", nullptr);
    return 1;
  }

  {
    std::unique_ptr<ui::Widget> widget(new ui::Widget);
    ui::Widget::InitParams widget_params;
    widget_params.size =
        profile->api_version == content::ContentProfile::APIVersion::RGSS1
            ? base::Vec2i(640, 480)
            : base::Vec2i(544, 416);
    widget_params.resizable = true;
    widget_params.hpixeldensity = true;
    widget_params.title = profile->window_title;
    widget->Init(std::move(widget_params));

    content::ContentRunner::InitParams content_params;
    content_params.profile = std::move(profile);
    content_params.window = widget->AsWeakPtr();
    content_params.entry = std::make_unique<binding::BindingEngineMri>();
    content_params.io_service = std::move(iosystem);

    std::unique_ptr<content::ContentRunner> runner =
        content::ContentRunner::Create(std::move(content_params));
    for (;;) {
      if (runner->RunMainLoop())
        break;
    }
  }

  TTF_Quit();
  SDL_Quit();

  return 0;
}
