// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/render/window_impl.h"

namespace content {

namespace {

void GPUCreateWindowAgentInternal(renderer::RenderDevice* device,
                                  WindowAgent* agent) {}

void GPUDestroyWindowAgentInternal(WindowAgent* agent) {
  delete agent;
}

}  // namespace

WindowImpl::WindowImpl(RenderScreenImpl* screen,
                       const base::Rect& bound,
                       scoped_refptr<ViewportImpl> parent)
    : GraphicsChild(screen),
      Disposable(screen),
      node_base_(SortKey()),
      node_control_(SortKey(2)),
      viewport_(parent),
      cursor_rect_(new RectImpl(base::Rect())) {
  node_base_.RebindController(parent ? parent->GetDrawableController()
                                     : screen->GetDrawableController());
  node_control_.RebindController(parent ? parent->GetDrawableController()
                                        : screen->GetDrawableController());

  node_base_.RegisterEventHandler(
      base::BindRepeating(&WindowImpl::DrawableNodeHandlerBaseLayerInternal,
                          base::Unretained(this)));
  node_control_.RegisterEventHandler(
      base::BindRepeating(&WindowImpl::DrawableNodeHandlerControlLayerInternal,
                          base::Unretained(this)));

  agent_ = new WindowAgent;
  screen->PostTask(base::BindOnce(&GPUCreateWindowAgentInternal,
                                  screen->GetDevice(), agent_));
}

WindowImpl::~WindowImpl() {
  ExceptionState exception_state;
  Dispose(exception_state);
}

void WindowImpl::Dispose(ExceptionState& exception_state) {
  Disposable::Dispose(exception_state);
}

bool WindowImpl::IsDisposed(ExceptionState& exception_state) {
  return Disposable::IsDisposed(exception_state);
}

void WindowImpl::Update(ExceptionState& exception_state) {}

scoped_refptr<Viewport> WindowImpl::Get_Viewport(
    ExceptionState& exception_state) {
  return viewport_;
}

void WindowImpl::Put_Viewport(const scoped_refptr<Viewport>& value,
                              ExceptionState& exception_state) {
  viewport_ = ViewportImpl::From(value);
  node_base_.RebindController(viewport_->GetDrawableController());
  node_control_.RebindController(viewport_->GetDrawableController());
}

scoped_refptr<Bitmap> WindowImpl::Get_Windowskin(
    ExceptionState& exception_state) {
  return windowskin_;
}

void WindowImpl::Put_Windowskin(const scoped_refptr<Bitmap>& value,
                                ExceptionState& exception_state) {
  windowskin_ = CanvasImpl::FromBitmap(value);
}

scoped_refptr<Bitmap> WindowImpl::Get_Contents(
    ExceptionState& exception_state) {
  return contents_;
}

void WindowImpl::Put_Contents(const scoped_refptr<Bitmap>& value,
                              ExceptionState& exception_state) {
  contents_ = CanvasImpl::FromBitmap(value);
}

bool WindowImpl::Get_Stretch(ExceptionState& exception_state) {
  return stretch_;
}

void WindowImpl::Put_Stretch(const bool& value,
                             ExceptionState& exception_state) {
  stretch_ = value;
}

scoped_refptr<Rect> WindowImpl::Get_CursorRect(
    ExceptionState& exception_state) {
  return cursor_rect_;
}

void WindowImpl::Put_CursorRect(const scoped_refptr<Rect>& value,
                                ExceptionState& exception_state) {
  cursor_rect_ = RectImpl::From(value);
}

bool WindowImpl::Get_Active(ExceptionState& exception_state) {
  return active_;
}

void WindowImpl::Put_Active(const bool& value,
                            ExceptionState& exception_state) {
  active_ = value;
}

bool WindowImpl::Get_Visible(ExceptionState& exception_state) {
  return node_base_.GetVisibility();
}

void WindowImpl::Put_Visible(const bool& value,
                             ExceptionState& exception_state) {
  node_base_.SetNodeVisibility(value);
  node_control_.SetNodeVisibility(value);
}

bool WindowImpl::Get_Pause(ExceptionState& exception_state) {
  return pause_;
}

void WindowImpl::Put_Pause(const bool& value, ExceptionState& exception_state) {
  pause_ = value;
}

int32_t WindowImpl::Get_X(ExceptionState& exception_state) {
  return bound_.x;
}

void WindowImpl::Put_X(const int32_t& value, ExceptionState& exception_state) {
  bound_.x = value;
}

int32_t WindowImpl::Get_Y(ExceptionState& exception_state) {
  return bound_.y;
}

void WindowImpl::Put_Y(const int32_t& value, ExceptionState& exception_state) {
  bound_.y = value;
}

int32_t WindowImpl::Get_Width(ExceptionState& exception_state) {
  return bound_.width;
}

void WindowImpl::Put_Width(const int32_t& value,
                           ExceptionState& exception_state) {
  bound_.width = value;
}

int32_t WindowImpl::Get_Height(ExceptionState& exception_state) {
  return bound_.height;
}

void WindowImpl::Put_Height(const int32_t& value,
                            ExceptionState& exception_state) {
  bound_.height = value;
}

int32_t WindowImpl::Get_Z(ExceptionState& exception_state) {
  return node_base_.GetSortKeys()->weight[0];
}

void WindowImpl::Put_Z(const int32_t& value, ExceptionState& exception_state) {
  node_base_.SetNodeSortWeight(value);
  node_control_.SetNodeSortWeight(value + 2);
}

int32_t WindowImpl::Get_Ox(ExceptionState& exception_state) {
  return origin_.x;
}

void WindowImpl::Put_Ox(const int32_t& value, ExceptionState& exception_state) {
  origin_.x = value;
}

int32_t WindowImpl::Get_Oy(ExceptionState& exception_state) {
  return origin_.y;
}

void WindowImpl::Put_Oy(const int32_t& value, ExceptionState& exception_state) {
  origin_.y = value;
}

uint32_t WindowImpl::Get_Opacity(ExceptionState& exception_state) {
  return opacity_;
}

void WindowImpl::Put_Opacity(const uint32_t& value,
                             ExceptionState& exception_state) {
  opacity_ = value;
}

uint32_t WindowImpl::Get_BackOpacity(ExceptionState& exception_state) {
  return back_opacity_;
}

void WindowImpl::Put_BackOpacity(const uint32_t& value,
                                 ExceptionState& exception_state) {
  back_opacity_ = value;
}

uint32_t WindowImpl::Get_ContentsOpacity(ExceptionState& exception_state) {
  return contents_opacity_;
}

void WindowImpl::Put_ContentsOpacity(const uint32_t& value,
                                     ExceptionState& exception_state) {
  contents_opacity_ = value;
}

void WindowImpl::OnObjectDisposed() {
  node_base_.DisposeNode();
  node_control_.DisposeNode();

  screen()->PostTask(base::BindOnce(&GPUDestroyWindowAgentInternal, agent_));
  agent_ = nullptr;
}

void WindowImpl::DrawableNodeHandlerBaseLayerInternal(
    DrawableNode::RenderStage stage,
    DrawableNode::RenderControllerParams* params) {
  if (stage == DrawableNode::RenderStage::kBeforeRender) {
  } else if (stage == DrawableNode::RenderStage::kOnRendering) {
  }
}

void WindowImpl::DrawableNodeHandlerControlLayerInternal(
    DrawableNode::RenderStage stage,
    DrawableNode::RenderControllerParams* params) {
  if (stage == DrawableNode::RenderStage::kBeforeRender) {
  } else if (stage == DrawableNode::RenderStage::kOnRendering) {
  }
}

}  // namespace content
