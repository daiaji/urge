// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "binding/unittests/engine_binding_unittests.h"

#include "SDL3_image/SDL_image.h"

#include "base/debug/logging.h"
#include "content/canvas/canvas_impl.h"
#include "content/render/plane_impl.h"
#include "content/render/sprite_impl.h"
#include "content/screen/renderscreen_impl.h"

EngineBindingUnittests::EngineBindingUnittests() : exit_flag_(0) {}

EngineBindingUnittests::~EngineBindingUnittests() {}

void EngineBindingUnittests::PreEarlyInitialization(
    content::ContentProfile* profile) {
  LOG(INFO) << "preload engine";
}

void EngineBindingUnittests::OnMainMessageLoopRun(
    content::ExecutionContext* execution) {
  content::ExceptionState exception_state;
  scoped_refptr<content::Bitmap> bmp =
      content::Bitmap::New(execution, "test.png", exception_state);
  if (exception_state.HadException()) {
    std::string msg;
    exception_state.FetchException(msg);
    LOG(INFO) << msg;
    return;
  }
  scoped_refptr<content::Bitmap> bmp2 =
      content::Bitmap::New(execution, "test2.png", exception_state);
  if (exception_state.HadException()) {
    std::string msg;
    exception_state.FetchException(msg);
    LOG(INFO) << msg;
    return;
  }

  bmp2->Blt(150, 50, bmp, bmp->GetRect(exception_state), 200, exception_state);
  if (exception_state.HadException()) {
    std::string msg;
    exception_state.FetchException(msg);
    LOG(INFO) << msg;
    return;
  }

  bmp2->GradientFillRect(0, 0, 100, 50,
                         content::Color::New(0, 0, 255, 255, exception_state),
                         content::Color::New(255, 255, 0, 255, exception_state),
                         false, exception_state);

  bmp2->DrawText(100, 100, 200, 50, "test draw text string", 0,
                 exception_state);

  auto* surf =
      static_cast<content::CanvasImpl*>(bmp2.get())->RequireMemorySurface();
  IMG_SavePNG(surf, "out.png");

  execution->graphics->Freeze(exception_state);

  auto root_vp =
      content::Viewport::New(execution, 50, 50, 400, 400, exception_state);
  root_vp->Put_Z(30, exception_state);

  auto vp = content::Viewport::New(execution, root_vp, exception_state);
  vp->Put_Rect(content::Rect::New(50, 50, 300, 300, exception_state),
               exception_state);
  vp->Put_Ox(-50, exception_state);
  vp->Put_Oy(-50, exception_state);
  vp->Put_Z(130, exception_state);

  auto spr = content::Sprite::New(execution, root_vp, exception_state);
  spr->Put_Bitmap(bmp, exception_state);
  spr->Put_X(0, exception_state);
  spr->Put_Y(0, exception_state);
  spr->Put_Z(100, exception_state);

  auto spr1 = content::Sprite::New(execution, vp, exception_state);
  spr1->Put_Bitmap(bmp2, exception_state);
  spr1->Put_X(0, exception_state);
  spr1->Put_Y(0, exception_state);
  spr1->Put_Z(200, exception_state);

  vp->Put_Tone(content::Tone::New(-68, -68, 0, 68, exception_state),
               exception_state);

  auto spr2 = content::Sprite::New(execution, nullptr, exception_state);
  spr2->Put_Bitmap(bmp2, exception_state);
  spr2->Put_X(250, exception_state);
  spr2->Put_Y(250, exception_state);
  spr2->Put_Z(20, exception_state);
  spr2->Put_WaveAmp(50, exception_state);
  spr2->Put_WaveSpeed(1000, exception_state);
  spr2->Put_ZoomY(2, exception_state);
  spr2->Put_Angle(20, exception_state);

  auto pl = content::Plane::New(execution, nullptr, exception_state);
  pl->Put_Bitmap(content::Bitmap::New(execution, "tile.png", exception_state),
                 exception_state);
  pl->Put_Z(10, exception_state);
  pl->Put_Opacity(100, exception_state);

  execution->graphics->Transition(180, scoped_refptr<content::Bitmap>(), 0,
                                  exception_state);

  auto snap = content::Bitmap::New(execution, 300, 300, exception_state);
  vp->Render(snap, exception_state);

  {
    auto* surf =
        static_cast<content::CanvasImpl*>(snap.get())->RequireMemorySurface();
    IMG_SavePNG(surf, "out_vp.png");
  }

  int32_t offset = 0;
  while (!exit_flag_) {
    spr2->Update(exception_state);

    offset += 5;
    pl->Put_Ox(offset, exception_state);
    pl->Put_Oy(offset, exception_state);
    execution->graphics->Update(exception_state);
  }
}

void EngineBindingUnittests::PostMainLoopRunning() {
  LOG(INFO) << "will close engine";
}

void EngineBindingUnittests::ExitSignalRequired() {
  exit_flag_.store(1);
}

void EngineBindingUnittests::ResetSignalRequired() {}
