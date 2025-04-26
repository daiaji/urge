// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "SDL3/SDL_main.h"
#include "SDL3/SDL_messagebox.h"
#include "SDL3/SDL_stdinc.h"
#include "SDL3_image/SDL_image.h"
#include "SDL3_ttf/SDL_ttf.h"

#include "binding/mri/mri_main.h"
#include "components/filesystem/io_service.h"
#include "content/components/font_context.h"
#include "content/profile/i18n_profile.h"
#include "content/worker/content_runner.h"
#include "ui/widget/widget.h"

namespace {

void ReplaceStringWidth(std::string& str, char before, char after) {
  for (size_t i = 0; i < str.size(); ++i)
    if (str[i] == before)
      str[i] = after;
}

#if defined(OS_WIN)
std::string AsciiToUtf8(const std::string& asciiStr) {
  if (asciiStr.empty()) {
    return "";
  }

  // ASCII -> UTF-16
  int wcharsCount =
      MultiByteToWideChar(CP_ACP, 0, asciiStr.c_str(), -1, NULL, 0);
  if (wcharsCount == 0) {
    throw std::runtime_error("MultiByteToWideChar failed: " +
                             std::to_string(GetLastError()));
  }

  std::vector<wchar_t> wstr(wcharsCount);
  if (MultiByteToWideChar(CP_ACP, 0, asciiStr.c_str(), -1, &wstr[0],
                          wcharsCount) == 0) {
    throw std::runtime_error("MultiByteToWideChar failed: " +
                             std::to_string(GetLastError()));
  }

  // UTF-16 -> UTF-8
  int utf8Count =
      WideCharToMultiByte(CP_UTF8, 0, &wstr[0], -1, NULL, 0, NULL, NULL);
  if (utf8Count == 0) {
    throw std::runtime_error("WideCharToMultiByte failed: " +
                             std::to_string(GetLastError()));
  }

  std::vector<char> utf8str(utf8Count);
  if (WideCharToMultiByte(CP_UTF8, 0, &wstr[0], -1, &utf8str[0], utf8Count,
                          NULL, NULL) == 0) {
    throw std::runtime_error("WideCharToMultiByte failed: " +
                             std::to_string(GetLastError()));
  }

  return std::string(&utf8str[0]);
}
#endif

}  // namespace

int main(int argc, char* argv[]) {
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

  {
    // Initialize filesystem
    std::unique_ptr<filesystem::IOService> io_service =
        std::make_unique<filesystem::IOService>(argv[0]);
    io_service->AddLoadPath(".");

    filesystem::IOState io_state;
    SDL_IOStream* inifile = io_service->OpenReadRaw(ini, &io_state);
    if (io_state.error_count) {
      LOG(INFO) << "[App] Warning: " << io_state.error_message;
    }

    // Initialize profile
    std::unique_ptr<content::ContentProfile> profile =
        content::ContentProfile::MakeFrom(app, inifile);
    profile->LoadCommandLine(argc, argv);

    if (!profile->LoadConfigure(app)) {
      SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Engine",
                               "Error when parse configure file.", nullptr);
      return 1;
    }

    // Initialize i18n profile
    auto* i18n_xml_stream =
        io_service->OpenReadRaw(profile->i18n_xml_path, nullptr);
    auto i18n_profile = content::I18NProfile::MakeForStream(i18n_xml_stream);

    // Initialize font context
    auto font_context = std::make_unique<content::ScopedFontData>(
        io_service.get(), profile->default_font_path);

    {
      // Initialize engine main widget
      std::unique_ptr<ui::Widget> widget(new ui::Widget);
      ui::Widget::InitParams widget_params;
      widget_params.opengl = true;
      widget_params.size = profile->window_size;
      widget_params.resizable = true;
      widget_params.hpixeldensity = true;
      widget_params.fullscreen = profile->fullscreen;
      widget_params.title =
#if defined(OS_WIN)
          AsciiToUtf8(profile->window_title);
#else
          profile->window_title;
#endif
      widget->Init(std::move(widget_params));

      // Create worker thread
      auto render_worker = base::ThreadWorker::Create();

      // Setup content runner module
      content::ContentRunner::InitParams content_params;
      content_params.profile = profile.get();
      content_params.io_service = io_service.get();
      content_params.font_context = font_context.get();
      content_params.i18n_profile = i18n_profile.get();
      content_params.window = widget->AsWeakPtr();
      content_params.render_worker = render_worker.get();
      content_params.entry = std::make_unique<binding::BindingEngineMri>();

      std::unique_ptr<content::ContentRunner> runner =
          content::ContentRunner::Create(std::move(content_params));
      runner->RunMainLoop();

      // Finalize modules at end
      render_worker.reset();
      widget.reset();
    }

    // Release resources
    font_context.reset();
    i18n_profile.reset();
    profile.reset();
    io_service.reset();
  }

  TTF_Quit();
  SDL_Quit();

  return 0;
}
