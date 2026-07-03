// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <filesystem>

#include "SDL3/SDL_main.h"
#include "SDL3/SDL_messagebox.h"
#include "SDL3/SDL_stdinc.h"
#include "SDL3/SDL_video.h"
#include "SDL3_image/SDL_image.h"
#include "SDL3_ttf/SDL_ttf.h"
#include "spdlog/sinks/android_sink.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

#include "binding/mri/mri_main.h"
#include "components/filesystem/io_service.h"
#include "content/canvas/font_context.h"
#include "content/profile/i18n_profile.h"
#include "content/worker/content_runner.h"
#include "base/debug/crash_handler.h"
#include "ui/widget/widget.h"

#if HAVE_ARB_ENCRYPTO_SUPPORT
#include "admenri/encryption/encrypt_arb.h"
#endif

#if defined(OS_ANDROID)
#include <jni.h>
#include <sys/system_properties.h>
#include <unistd.h>

#if defined(OS_WIN)
#include <windows.h>
extern "C" {
__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif  //! OS_WIN

static int g_pfd[2];
static pthread_t g_android_stdio_thread;

static void* StdioTransferThreadFunc(void*) {
  ssize_t rdsz;
  char buf[128];
  while ((rdsz = read(g_pfd[0], buf, sizeof buf - 1)) > 0) {
    if (buf[rdsz - 1] == '\n')
      --rdsz;
    buf[rdsz] = 0; /* add null-terminator */
    __android_log_write(ANDROID_LOG_DEBUG, "urge-stdio", buf);
  }
  return 0;
}

int SetupAndroidStudioTransfer() {
  /* make stdout line-buffered and stderr unbuffered */
  setvbuf(stdout, 0, _IOLBF, 0);
  setvbuf(stderr, 0, _IONBF, 0);

  /* create the pipe and redirect stdout and stderr */
  pipe(g_pfd);
  dup2(g_pfd[1], 1);
  dup2(g_pfd[1], 2);

  /* spawn the logging thread */
  if (pthread_create(&g_android_stdio_thread, 0, StdioTransferThreadFunc, 0) ==
      -1)
    return -1;
  pthread_detach(g_android_stdio_thread);
  return 0;
}

#endif

#if defined(OS_WIN)
#include <windows.h>

void CreateConsoleWin() {
  if (::GetConsoleWindow())
    return;

  if (!::AttachConsole(ATTACH_PARENT_PROCESS)) {
    ::AllocConsole();
    ::SetConsoleCP(CP_UTF8);
    ::SetConsoleOutputCP(CP_UTF8);
    ::SetConsoleTitleW(L"URGE Debugging Console");
  }

  // Redirect std handle
  std::freopen("CONIN$", "rb", stdin);
  std::freopen("CONOUT$", "wb", stdout);
  std::freopen("CONOUT$", "wb", stderr);
}
#endif

int main(int argc, char* argv[]) {
#if defined(OS_WIN)
  // Allocate console if need
  for (int i = 0; i < argc; ++i) {
    if (!std::strcmp(argv[i], "console")) {
      CreateConsoleWin();
      break;
    }
  }
#endif  //! defined(OS_WIN)

#if defined(OS_ANDROID)
  SetupAndroidStudioTransfer();
#endif  // !defined(OS_ANDROID)

#if defined(OS_ANDROID)
  // Get GAME_PATH string field from JNI (MainActivity.java)
  JNIEnv* env = (JNIEnv*)SDL_GetAndroidJNIEnv();
  jobject activity = (jobject)SDL_GetAndroidActivity();
  jclass activity_klass = env->GetObjectClass(activity);
  jfieldID field_game_path =
      env->GetStaticFieldID(activity_klass, "GAME_PATH", "Ljava/lang/String;");
  jstring java_string_game_path =
      (jstring)env->GetStaticObjectField(activity_klass, field_game_path);
  const char* game_data_dir = env->GetStringUTFChars(java_string_game_path, 0);

  // Set and ensure current directory
  std::filesystem::path std_path(game_data_dir);
  if (!std::filesystem::exists(std_path) ||
      !std::filesystem::is_directory(std_path))
    std::filesystem::create_directories(std_path);

  std::filesystem::current_path(std_path);

  env->ReleaseStringUTFChars(java_string_game_path, game_data_dir);
  env->DeleteLocalRef(java_string_game_path);
  env->DeleteLocalRef(activity_klass);

  // Fixed configure file
  std::string app = "Game";
  std::string ini = app + ".ini";
#elif defined(OS_EMSCRIPTEN)
  std::string app = "Game";
  std::string ini = app + ".ini";
#else
  std::string app(argv[0]);
  for (size_t i = 0; i < app.size(); ++i)
    if (app[i] == '\\')
      app[i] = '/';

  auto last_sep = app.find_last_of('/');
  if (last_sep != std::string::npos)
    app = app.substr(last_sep + 1);

  last_sep = app.find_last_of('.');
  if (last_sep != std::string::npos)
    app = app.substr(0, last_sep);
  std::string ini = app + ".ini";
#endif  //! defined(OS_ANDROID)

  // Current path
  std::u8string current_path =
      std::filesystem::current_path().generic_u8string();

  // Initialize filesystem
  std::unique_ptr<filesystem::IOService> io_service =
      filesystem::IOService::Create(argv[0]);
  io_service->AddLoadPath(reinterpret_cast<const char*>(current_path.c_str()),
                          "", false);
  io_service->SetWritePath(reinterpret_cast<const char*>(current_path.c_str()));

  filesystem::IOState io_state;
  SDL_IOStream* inifile = io_service->OpenReadRaw(ini, &io_state);
  if (io_state.error_count) {
    std::string error_info = "Failed to load configure: ";
    error_info += ini;
    error_info += '\n';
    error_info += "Current path: ";
    error_info += reinterpret_cast<const char*>(current_path.c_str());

    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "URGE", error_info.c_str(),
                             nullptr);
    return 1;
  }

  // Initialize profile
  std::unique_ptr<content::ContentProfile> profile =
      std::make_unique<content::ContentProfile>(app, inifile);
  profile->LoadCommandLine(argc, argv);

  if (!profile->LoadConfigure(app)) {
    std::string error_message = "Error when parse configure file: \n";
    error_message += ini;
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "URGE",
                             error_message.c_str(), nullptr);
    return 1;
  }

#if defined(OS_WIN)
  if (profile->debugging_console)
    CreateConsoleWin();
#endif

  // Create spdlog logger
#if defined(OS_ANDROID)
  auto android_sink =
      std::make_shared<spdlog::sinks::android_sink_mt>("urgecore");
  android_sink->set_pattern("[%^%l%$] %v");

  auto file_sink =
      std::make_shared<spdlog::sinks::basic_file_sink_mt>(app + ".log", true);
  file_sink->set_level(spdlog::level::trace);
#else
  auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  console_sink->set_pattern("[%^%l%$] %v");

  std::vector<spdlog::sink_ptr> logger_sinks;
#if defined(OS_ANDROID)
  logger_sinks.push_back(android_sink);
#else
  logger_sinks.push_back(console_sink);
#endif

  // Conditionally add file sink
  if (profile->save_log) {
    auto file_sink =
        std::make_shared<spdlog::sinks::basic_file_sink_mt>("Engine.log", true);
    file_sink->set_pattern("[%^%l%$] %v");
    file_sink->set_level(spdlog::level::trace);
    logger_sinks.push_back(file_sink);
  }
#endif

  spdlog::logger logger_sink("urgecore", logger_sinks.begin(), logger_sinks.end());
  logger_sink.flush_on(spdlog::level::info);
  base::logging::InitWithLogger(&logger_sink);

  // Open crash-safe log file and install signal handlers
  base::debug::OpenCrashLogFile("crash.log");
  base::debug::InstallCrashHandlers();

  // Create console logger (user-facing output: Console.log + terminal)
  {
    auto console_file_sink =
        std::make_shared<spdlog::sinks::basic_file_sink_mt>("Console.log", true);
    console_file_sink->set_pattern("[%^%l%$] %v");
    auto console_stdout_sink =
        std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_stdout_sink->set_pattern("[%^%l%$] %v");
    static spdlog::logger console_logger("console",
                                         {console_stdout_sink, console_file_sink});
    console_logger.flush_on(spdlog::level::info);
    base::logging::InitConsoleLogger(&console_logger);
  }

  LOG(INFO) << "[App] Current Path: "
            << reinterpret_cast<const char*>(current_path.c_str());
  LOG(INFO) << "[App] Configure File: " << ini;

  if (profile->game_debug)
    LOG(INFO) << "[App] Running debug test.";
  if (profile->game_battle_test)
    LOG(INFO) << "[App] Running battle test.";

// Setup encryption resource package
#if HAVE_ARB_ENCRYPTO_SUPPORT
  std::string app_package = app + ".arb";
  if (admenri::LoadCryptoPackage(app_package))
    LOG(INFO) << "[IOService] Encrypto pack \"" << app_package << "\" added.";
#endif

  // Disable IME on Windows
#if defined(OS_WIN)
  if (profile->disable_ime) {
    LOG(INFO) << "[Windows] Disable process IME.";
    ::ImmDisableIME(TRUE);
  }
#endif

  // Setup SDL init params
  SDL_SetHint(SDL_HINT_ORIENTATIONS, profile->orientation.c_str());
  SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");
  SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "1");
  SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "1");

#if defined(OS_LINUX)
  // Diligent Engine requires X11 on Linux; prefer X11 driver
  SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "x11");
#endif

  if (!SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO | SDL_INIT_AUDIO |
                SDL_INIT_GAMEPAD)) {
    LOG(ERROR) << "[App] Failed to initialize SDL: " << SDL_GetError();
    return 1;
  }
  TTF_Init();

  {
    // Initialize i18n profile
    auto* i18n_xml_stream =
        io_service->OpenReadRaw(profile->i18n_xml_path, nullptr);
    auto i18n_profile = std::make_unique<content::I18NProfile>(i18n_xml_stream);

    // Initialize font context
    auto font_context = std::make_unique<content::ScopedFontData>(
        io_service.get(), profile->default_font_path);

    // Apply font rendering preferences from INI
    font_context->font_scale = profile->font_scale;
    font_context->font_kerning = profile->font_kerning;
    font_context->font_hinting =
        static_cast<TTF_HintingFlags>(profile->font_hinting);
    font_context->font_outline_crop = profile->font_outline_crop;

    // Parse font substitution entries ("From>To") from INI
    for (const auto& entry : profile->font_subs) {
      size_t sep = entry.find('>');
      if (sep != std::string::npos) {
        std::string from = entry.substr(0, sep);
        std::string to = entry.substr(sep + 1);
        std::transform(from.begin(), from.end(), from.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        font_context->font_subs[from] = to;
      }
    }

    {
      // Initialize engine main widget
      std::unique_ptr<ui::Widget> widget(new ui::Widget);
      ui::Widget::InitParams widget_params;
#if defined(OS_LINUX)
      // Linux fallback chain ends at OPENGL, always enable GLX window support
      widget_params.opengl = true;
#endif
      widget_params.size = profile->window_size;
      widget_params.resizable = profile->win_resizable;
      widget_params.hpixeldensity =
#if !defined(OS_EMSCRIPTEN)
          true;
#else
          false;
#endif
      widget_params.fullscreen =
#if defined(OS_ANDROID)
          true;
#else
          profile->fullscreen;
#endif
      widget_params.title = profile->window_title;
      widget->Init(std::move(widget_params));

      // Apply fixed aspect ratio from config
      if (profile->fixed_aspect_ratio) {
        float game_ratio = static_cast<float>(profile->resolution.x) /
                           static_cast<float>(profile->resolution.y);
        SDL_SetWindowAspectRatio(widget->AsSDLWindow(), game_ratio, game_ratio);
      }

      // Get display refresh rate for syncToRefreshrate
      float display_refresh_rate = 0.0f;
      {
        SDL_DisplayID display =
            SDL_GetDisplayForWindow(widget->AsSDLWindow());
        const SDL_DisplayMode* mode = SDL_GetCurrentDisplayMode(display);
        if (mode)
          display_refresh_rate = mode->refresh_rate;
      }

      // Setup content runner module
      content::ContentRunner::InitParams content_params;
      content_params.profile = profile.get();
      content_params.io_service = io_service.get();
      content_params.font_context = font_context.get();
      content_params.i18n_profile = i18n_profile.get();
      content_params.window = widget->AsWeakPtr();
      content_params.display_refresh_rate = display_refresh_rate;
      content_params.entry = std::make_unique<binding::BindingEngineMri>();

      std::unique_ptr<content::ContentRunner> runner =
          content::ContentRunner::Create(std::move(content_params));
      if (runner) {
        // Run main loop if no exception
        runner->RunMainLoop();
      } else {
        // Throw exception when initializing
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "URGE",
                                 "Failed to load engine.", nullptr);
      }

      // Finalize modules at end
      runner.reset();
      widget.reset();
    }

    // Release resources
    font_context.reset();
    i18n_profile.reset();
  }

  // Release objects
  profile.reset();
  io_service.reset();

  TTF_Quit();
  SDL_Quit();

  return 0;
}
