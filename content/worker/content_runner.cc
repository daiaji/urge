// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/worker/content_runner.h"

#include "third_party/imgui/imgui.h"
#include "third_party/imgui/imgui_impl_sdl3.h"
#include "third_party/imgui/imgui_impl_wgpu.h"

#include "content/context/execution_context.h"
#include "content/profile/command_ids.h"

namespace content {

namespace {

void DrawEngineInfoGUI(I18NProfile* i18n_profile) {
  if (ImGui::CollapsingHeader(
          i18n_profile->GetI18NString(IDS_SETTINGS_ABOUT, "About").c_str())) {
    ImGui::Text("Universal Ruby Game Engine (URGE) Runtime");
    ImGui::Separator();
    ImGui::Text("Copyright (C) 2018-2025 Admenri Adev.");
    ImGui::TextWrapped(
        "The URGE is licensed under the BSD-2-Clause License, see LICENSE for "
        "more information.");

    if (ImGui::Button("Github"))
      SDL_OpenURL("https://github.com/Admenri/urge");
  }
}

}  // namespace

ContentRunner::ContentRunner(std::unique_ptr<ContentProfile> profile,
                             std::unique_ptr<filesystem::IOService> io_service,
                             std::unique_ptr<base::ThreadWorker> render_worker,
                             std::unique_ptr<EngineBindingBase> binding,
                             base::WeakPtr<ui::Widget> window)
    : profile_(std::move(profile)),
      render_worker_(std::move(render_worker)),
      window_(window),
      exit_code_(0),
      binding_quit_flag_(0),
      binding_reset_flag_(0),
      binding_(std::move(binding)),
      io_service_(std::move(io_service)),
      disable_gui_input_(false),
      show_settings_menu_(false) {}

void ContentRunner::InitializeContentInternal() {
  // Initialize CC
  cc_.reset(new CoroutineContext);
  cc_->primary_fiber = fiber_create(nullptr, 0, nullptr, nullptr);
  cc_->main_loop_fiber =
      fiber_create(cc_->primary_fiber, 0, EngineEntryFunctionInternal, this);

  // Graphics initialize settings
  int frame_rate = 40;
  base::Vec2i resolution(640, 480);
  if (profile_->api_version >= ContentProfile::APIVersion::RGSS2) {
    resolution = base::Vec2i(544, 416);
    frame_rate = 60;
  }

  // Create components instance
  auto* i18n_xml_stream =
      io_service_->OpenReadRaw(profile_->i18n_xml_path, nullptr);
  i18n_profile_ = I18NProfile::MakeForStream(i18n_xml_stream);
  scoped_font_.reset(
      new ScopedFontData(io_service_.get(), profile_->default_font_path));
  graphics_impl_.reset(new RenderScreenImpl(
      cc_.get(), profile_.get(), i18n_profile_.get(), io_service_.get(),
      scoped_font_.get(), resolution, frame_rate));
  input_impl_.reset(new KeyboardControllerImpl(
      window_->AsWeakPtr(), profile_.get(), i18n_profile_.get()));
  audio_impl_.reset(
      new AudioImpl(profile_.get(), io_service_.get(), i18n_profile_.get()));

  // Init all module workers
  graphics_impl_->InitWithRenderWorker(render_worker_.get(), window_,
                                       profile_->wgpu_backend);
  tick_observer_ = graphics_impl_->AddTickObserver(base::BindRepeating(
      &ContentRunner::TickHandlerInternal, base::Unretained(this)));
}

void ContentRunner::TickHandlerInternal() {
  if (binding_quit_flag_.load()) {
    binding_quit_flag_.store(0);
    binding_->ExitSignalRequired();
  } else if (binding_reset_flag_.load()) {
    binding_reset_flag_.store(0);
    binding_->ResetSignalRequired();
  }
}

void ContentRunner::GUICompositeHandlerInternal() {
  ImGui::SetNextWindowPos(ImVec2(), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(350, 400), ImGuiCond_FirstUseEver);

  bool enable_input_module = true;
  if (ImGui::Begin(i18n_profile_->GetI18NString(IDS_MENU_SETTINGS, "Settings")
                       .c_str())) {
    // GUI focus manage
    enable_input_module = !ImGui::IsWindowFocused();

    // Button settings
    disable_gui_input_ = input_impl_->CreateButtonGUISettings();

    // Graphics settings
    graphics_impl_->CreateButtonGUISettings();

    // Audio settings
    // TODO:

    // Engine Info
    DrawEngineInfoGUI(i18n_profile_.get());
  }

  // End window create
  ImGui::End();

  // Disable input if need
  input_impl_->SetUpdateEnable(enable_input_module);
}

ContentRunner::~ContentRunner() = default;

bool ContentRunner::RunMainLoop() {
  // Poll event queue
  SDL_Event queued_event;
  while (SDL_PollEvent(&queued_event)) {
    // Quit event
    if (queued_event.type == SDL_EVENT_QUIT)
      binding_quit_flag_.store(1);

    // Shortcut
    if (queued_event.type == SDL_EVENT_KEY_UP &&
        queued_event.key.windowID == window_->GetWindowID()) {
      if (queued_event.key.scancode == SDL_SCANCODE_F1) {
        show_settings_menu_ = !show_settings_menu_;
      } else if (queued_event.key.scancode == SDL_SCANCODE_F12) {
        binding_reset_flag_.store(1);
      }
    }

    // IMGUI Event
    if (show_settings_menu_ && !disable_gui_input_)
      ImGui_ImplSDL3_ProcessEvent(&queued_event);
  }

  if (show_settings_menu_) {
    // Start IMGUI frame
    ImGui_ImplSDL3_NewFrame();

    // Layout new frame
    ImGui::NewFrame();
    GUICompositeHandlerInternal();
    ImGui::EndFrame();
  } else {
    // Reset input state
    input_impl_->SetUpdateEnable(true);
  }

  // Present screen
  graphics_impl_->SetRenderGUI(show_settings_menu_);
  graphics_impl_->PresentScreen();

  return exit_code_.load();
}

std::unique_ptr<ContentRunner> ContentRunner::Create(InitParams params) {
  std::unique_ptr<base::ThreadWorker> render_worker =
      base::ThreadWorker::Create();

  auto* runner = new ContentRunner(
      std::move(params.profile), std::move(params.io_service),
      std::move(render_worker), std::move(params.entry), params.window);
  runner->InitializeContentInternal();

  return std::unique_ptr<ContentRunner>(runner);
}

void ContentRunner::EngineEntryFunctionInternal(fiber_t* fiber) {
  auto* self = static_cast<ContentRunner*>(fiber->userdata);

  // Before running loop handler
  self->binding_->PreEarlyInitialization(self->profile_.get(),
                                         self->io_service_.get());

  // Make script binding execution context
  // Call binding boot handler before running loop handler
  ExecutionContext execution_context;
  execution_context.font_context = self->scoped_font_.get();
  execution_context.canvas_scheduler =
      self->graphics_impl_->GetCanvasScheduler();
  execution_context.graphics = self->graphics_impl_.get();
  execution_context.input = self->input_impl_.get();

  // Make module context
  EngineBindingBase::ScopedModuleContext module_context;
  module_context.graphics = self->graphics_impl_.get();
  module_context.input = self->input_impl_.get();
  module_context.audio = self->audio_impl_.get();

  // Execute main loop
  self->binding_->OnMainMessageLoopRun(&execution_context, &module_context);

  // End of running
  self->binding_->PostMainLoopRunning();
  self->exit_code_.store(1);

  // Loop switch context to primary fiber
  for (;;)
    fiber_switch(self->cc_->primary_fiber);
}

}  // namespace content
