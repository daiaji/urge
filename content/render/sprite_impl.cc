// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/render/sprite_impl.h"

#include "content/screen/renderscreen_impl.h"
#include "content/screen/viewport_impl.h"

namespace content {

scoped_refptr<Sprite> Sprite::New(ExecutionContext* execution_context,
                                  scoped_refptr<Viewport> viewport,
                                  ExceptionState& exception_state) {
  DrawNodeController* parent =
      execution_context->graphics->GetDrawableController();

  if (viewport)
    parent = ViewportImpl::From(viewport)->GetDrawableController();

  return new SpriteImpl(parent);
}

SpriteImpl::SpriteImpl(DrawNodeController* parent) : node_(SortKey()) {
  node_.RebindController(parent);
}

SpriteImpl::~SpriteImpl() {
  ExceptionState exception_state;
  Dispose(exception_state);
}

void SpriteImpl::Dispose(ExceptionState& exception_state) {}

bool SpriteImpl::IsDisposed(ExceptionState& exception_state) {
  return false;
}

void SpriteImpl::Flash(scoped_refptr<Color> color,
                       uint32_t duration,
                       ExceptionState& exception_state) {}

void SpriteImpl::Update(ExceptionState& exception_state) {}

uint32_t SpriteImpl::Width(ExceptionState& exception_state) {
  return 0;
}

uint32_t SpriteImpl::Height(ExceptionState& exception_state) {
  return 0;
}

scoped_refptr<Bitmap> SpriteImpl::Get_Bitmap(ExceptionState& exception_state) {
  return nullptr;
}

void SpriteImpl::Put_Bitmap(const scoped_refptr<Bitmap>& value,
                            ExceptionState& exception_state) {}

scoped_refptr<Rect> SpriteImpl::Get_SrcRect(ExceptionState& exception_state) {
  return nullptr;
}

void SpriteImpl::Put_SrcRect(const scoped_refptr<Rect>& value,
                             ExceptionState& exception_state) {}

scoped_refptr<Viewport> SpriteImpl::Get_Viewport(
    ExceptionState& exception_state) {
  return nullptr;
}

void SpriteImpl::Put_Viewport(const scoped_refptr<Viewport>& value,
                              ExceptionState& exception_state) {}

bool SpriteImpl::Get_Visible(ExceptionState& exception_state) {
  return false;
}

void SpriteImpl::Put_Visible(const bool& value,
                             ExceptionState& exception_state) {}

int32_t SpriteImpl::Get_X(ExceptionState& exception_state) {
  return 0;
}

void SpriteImpl::Put_X(const int32_t& value, ExceptionState& exception_state) {}

int32_t SpriteImpl::Get_Y(ExceptionState& exception_state) {
  return 0;
}

void SpriteImpl::Put_Y(const int32_t& value, ExceptionState& exception_state) {}

int32_t SpriteImpl::Get_Z(ExceptionState& exception_state) {
  return 0;
}

void SpriteImpl::Put_Z(const int32_t& value, ExceptionState& exception_state) {}

int32_t SpriteImpl::Get_Ox(ExceptionState& exception_state) {
  return 0;
}

void SpriteImpl::Put_Ox(const int32_t& value, ExceptionState& exception_state) {
}

int32_t SpriteImpl::Get_Oy(ExceptionState& exception_state) {
  return 0;
}

void SpriteImpl::Put_Oy(const int32_t& value, ExceptionState& exception_state) {
}

float SpriteImpl::Get_ZoomX(ExceptionState& exception_state) {
  return 0.0f;
}

void SpriteImpl::Put_ZoomX(const float& value,
                           ExceptionState& exception_state) {}

float SpriteImpl::Get_ZoomY(ExceptionState& exception_state) {
  return 0.0f;
}

void SpriteImpl::Put_ZoomY(const float& value,
                           ExceptionState& exception_state) {}

int32_t SpriteImpl::Get_Angle(ExceptionState& exception_state) {
  return 0;
}

void SpriteImpl::Put_Angle(const int32_t& value,
                           ExceptionState& exception_state) {}

int32_t SpriteImpl::Get_WaveAmp(ExceptionState& exception_state) {
  return 0;
}

void SpriteImpl::Put_WaveAmp(const int32_t& value,
                             ExceptionState& exception_state) {}

int32_t SpriteImpl::Get_WaveLength(ExceptionState& exception_state) {
  return 0;
}

void SpriteImpl::Put_WaveLength(const int32_t& value,
                                ExceptionState& exception_state) {}

int32_t SpriteImpl::Get_WaveSpeed(ExceptionState& exception_state) {
  return 0;
}

void SpriteImpl::Put_WaveSpeed(const int32_t& value,
                               ExceptionState& exception_state) {}

int32_t SpriteImpl::Get_WavePhase(ExceptionState& exception_state) {
  return 0;
}

void SpriteImpl::Put_WavePhase(const int32_t& value,
                               ExceptionState& exception_state) {}

bool SpriteImpl::Get_Mirror(ExceptionState& exception_state) {
  return false;
}

void SpriteImpl::Put_Mirror(const bool& value,
                            ExceptionState& exception_state) {}

int32_t SpriteImpl::Get_BushDepth(ExceptionState& exception_state) {
  return 0;
}

void SpriteImpl::Put_BushDepth(const int32_t& value,
                               ExceptionState& exception_state) {}

int32_t SpriteImpl::Get_BushOpacity(ExceptionState& exception_state) {
  return 0;
}

void SpriteImpl::Put_BushOpacity(const int32_t& value,
                                 ExceptionState& exception_state) {}

uint32_t SpriteImpl::Get_Opacity(ExceptionState& exception_state) {
  return 0;
}

void SpriteImpl::Put_Opacity(const uint32_t& value,
                             ExceptionState& exception_state) {}

uint32_t SpriteImpl::Get_BlendType(ExceptionState& exception_state) {
  return 0;
}

void SpriteImpl::Put_BlendType(const uint32_t& value,
                               ExceptionState& exception_state) {}

scoped_refptr<Color> SpriteImpl::Get_Color(ExceptionState& exception_state) {
  return nullptr;
}

void SpriteImpl::Put_Color(const scoped_refptr<Color>& value,
                           ExceptionState& exception_state) {}

scoped_refptr<Tone> SpriteImpl::Get_Tone(ExceptionState& exception_state) {
  return nullptr;
}

void SpriteImpl::Put_Tone(const scoped_refptr<Tone>& value,
                          ExceptionState& exception_state) {}

}  // namespace content
