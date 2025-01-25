// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "binding/unittests/engine_binding_unittests.h"

#include "SDL3_image/SDL_image.h"

#include "base/debug/logging.h"
#include "content/canvas/canvas_impl.h"
#include "content/render/sprite_impl.h"
#include "content/screen/renderscreen_impl.h"

EngineBindingUnittests::EngineBindingUnittests() {}

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

  auto vp =
      content::Viewport::New(execution, 100, 100, 300, 300, exception_state);
  vp->Put_Ox(-100, exception_state);
  vp->Put_Oy(100, exception_state);

  auto spr = content::Sprite::New(execution, vp, exception_state);
  spr->Put_Bitmap(bmp, exception_state);
  spr->Put_X(0, exception_state);
  spr->Put_Y(0, exception_state);
  spr->Put_Z(100, exception_state);

  auto spr1 = content::Sprite::New(execution, vp, exception_state);
  spr1->Put_Bitmap(bmp2, exception_state);
  spr1->Put_X(100, exception_state);
  spr1->Put_Y(100, exception_state);
  spr1->Put_Z(0, exception_state);

  for (;;) {
    execution->graphics->Update(exception_state);
  }
}

void EngineBindingUnittests::PostMainLoopRunning() {
  LOG(INFO) << "will close engine";
}
