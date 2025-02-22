// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDER_WINDOW_IMPL_H_
#define CONTENT_RENDER_WINDOW_IMPL_H_

#include "content/canvas/canvas_impl.h"
#include "content/public/engine_window.h"
#include "content/screen/viewport_impl.h"

namespace content {

struct WindowAgent {
  
};

class WindowImpl : public Window, public GraphicsChild, public Disposable {
 public:
  WindowImpl(RenderScreenImpl* screen,
             const base::Rect& bound,
             scoped_refptr<ViewportImpl> parent);
  ~WindowImpl() override;

  WindowImpl(const WindowImpl&) = delete;
  WindowImpl& operator=(const WindowImpl&) = delete;

  void Dispose(ExceptionState& exception_state) override;
  bool IsDisposed(ExceptionState& exception_state) override;
  void Update(ExceptionState& exception_state) override;

  URGE_DECLARE_OVERRIDE_ATTRIBUTE(Viewport, scoped_refptr<Viewport>);
  URGE_DECLARE_OVERRIDE_ATTRIBUTE(Windowskin, scoped_refptr<Bitmap>);
  URGE_DECLARE_OVERRIDE_ATTRIBUTE(Contents, scoped_refptr<Bitmap>);
  URGE_DECLARE_OVERRIDE_ATTRIBUTE(Stretch, bool);
  URGE_DECLARE_OVERRIDE_ATTRIBUTE(CursorRect, scoped_refptr<Rect>);
  URGE_DECLARE_OVERRIDE_ATTRIBUTE(Active, bool);
  URGE_DECLARE_OVERRIDE_ATTRIBUTE(Visible, bool);
  URGE_DECLARE_OVERRIDE_ATTRIBUTE(Pause, bool);
  URGE_DECLARE_OVERRIDE_ATTRIBUTE(X, int32_t);
  URGE_DECLARE_OVERRIDE_ATTRIBUTE(Y, int32_t);
  URGE_DECLARE_OVERRIDE_ATTRIBUTE(Width, int32_t);
  URGE_DECLARE_OVERRIDE_ATTRIBUTE(Height, int32_t);
  URGE_DECLARE_OVERRIDE_ATTRIBUTE(Z, int32_t);
  URGE_DECLARE_OVERRIDE_ATTRIBUTE(Ox, int32_t);
  URGE_DECLARE_OVERRIDE_ATTRIBUTE(Oy, int32_t);
  URGE_DECLARE_OVERRIDE_ATTRIBUTE(Opacity, uint32_t);
  URGE_DECLARE_OVERRIDE_ATTRIBUTE(BackOpacity, uint32_t);
  URGE_DECLARE_OVERRIDE_ATTRIBUTE(ContentsOpacity, uint32_t);

 private:
  void OnObjectDisposed() override;
  std::string DisposedObjectName() override { return "Window"; }
  void DrawableNodeHandlerBaseLayerInternal(
      DrawableNode::RenderStage stage,
      DrawableNode::RenderControllerParams* params);
  void DrawableNodeHandlerControlLayerInternal(
      DrawableNode::RenderStage stage,
      DrawableNode::RenderControllerParams* params);

  DrawableNode node_base_;
  DrawableNode node_control_;

  WindowAgent* agent_;

  scoped_refptr<ViewportImpl> viewport_;
  scoped_refptr<CanvasImpl> windowskin_;
  scoped_refptr<CanvasImpl> contents_;

  bool stretch_ = true;
  scoped_refptr<RectImpl> cursor_rect_;
  bool active_ = false;
  bool pause_ = false;
  base::Rect bound_;
  base::Vec2i origin_;
  int32_t opacity_ = 255;
  int32_t back_opacity_ = 255;
  int32_t contents_opacity_ = 255;
};

}  // namespace content

#endif  //! CONTENT_RENDER_WINDOW_IMPL_H_
