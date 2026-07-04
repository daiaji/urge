// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_CONTEXT_EXECUTION_CONTEXT_H_
#define CONTENT_CONTEXT_EXECUTION_CONTEXT_H_

#include <cstdio>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "components/audioservice/audio_service.h"
#include "components/filesystem/io_service.h"
#include "components/network/public/network_service.h"
#include "content/canvas/canvas_scheduler.h"
#include "content/canvas/font_context.h"
#include "content/context/disposable.h"
#include "content/profile/content_profile.h"
#include "content/profile/i18n_profile.h"
#include "content/render/drawable_controller.h"
#include "content/render/pipeline_collection.h"
#include "content/render/sprite_batch.h"
#include "content/worker/event_controller.h"
#include "renderer/device/render_device.h"
#include "renderer/resource/render_buffer.h"
#include "ui/widget/widget.h"

#include "base/debug/logging.h"

namespace content {

struct ExecutionContext {
  base::Vec2i resolution;
  base::WeakPtr<ui::Widget> window;
  renderer::RenderDevice* render_device;
  Diligent::IDeviceContext* primary_render_context;

  struct {
    std::unique_ptr<renderer::QuadIndexCache> quad_index;
    std::unique_ptr<renderer::PipelineSet> pipeline_loader;
    std::unique_ptr<PipelineCollection> pipeline_states;
  } render;

  ContentProfile* engine_profile = nullptr;
  ScopedFontData* font_context = nullptr;
  I18NProfile* i18n_profile = nullptr;
  filesystem::IOService* io_service = nullptr;
  CanvasScheduler* canvas_scheduler = nullptr;
  SpriteBatch* sprite_batcher = nullptr;
  EventController* event_controller = nullptr;
  audioservice::AudioService* audio_server = nullptr;
  DisposableCollection* disposable_parent = nullptr;
  DrawNodeController* screen_drawable_node = nullptr;
#if !defined(OS_EMSCRIPTEN)
  network::NetworkService* network_service = nullptr;
#endif

  // Console overlay state (shared between content runner and Ruby bindings)
  struct {
    enum { kMaxOutputLines = 2000 };

    std::vector<std::string> output;
    std::mutex output_mutex;
    bool show = false;
    bool scroll_to_bottom = false;

    // Push a user-facing message to the console overlay + console log.
    // Writes to: ImGui overlay + Console.log (file) + stdout (terminal)
    void Push(const std::string& line) {
      PushOutput(line);
      base::logging::ConsoleLog(line);
    }

    // Push a log message to the ImGui console overlay only.
    // Called from LogMessage callback with already-formatted message.
    void PushLog(const std::string& line) {
      PushOutput(line);
    }

   private:
    void PushOutput(const std::string& line) {
      std::lock_guard<std::mutex> lock(output_mutex);
      output.push_back(line);
      if (output.size() > kMaxOutputLines)
        output.erase(output.begin());
    }

   public:
  } console;

  // Ruby eval callback (set by MRI binding, called from content runner console)
  std::function<std::string(const std::string&)> eval_ruby;

  ExecutionContext();
  ~ExecutionContext();

  ExecutionContext(const ExecutionContext&) = delete;
  ExecutionContext& operator=(const ExecutionContext&) = delete;
};

}  // namespace content

#endif  //! CONTENT_CONTEXT_EXECUTION_CONTEXT_H_
