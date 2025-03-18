// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/render/window_impl.h"

#include "content/composite/tilequad.h"

namespace content {

namespace {

void GPUCreateWindowInternal(renderer::RenderDevice* device,
                             WindowAgent* agent) {
  agent->background_batch = renderer::QuadBatch::Make(**device);
  agent->control_batch = renderer::QuadBatch::Make(**device);
}

void GPUDestroyWindowInternal(WindowAgent* agent) {
  delete agent;
}

void GPUCompositeBackgroundLayerInternal(renderer::RenderDevice* device,
                                         wgpu::CommandEncoder* encoder,
                                         WindowAgent* agent,
                                         int32_t scale,
                                         const base::Rect& bound,
                                         bool stretch,
                                         int32_t opacity,
                                         int32_t back_opacity) {
  auto& quad_buffer = agent->background_cache;

  // Display offset
  base::Vec2i display_offset = bound.Position();

  // Calculate total quads count
  {
    // Corners
    int count = 4;

    // Background
    if (stretch) {
      count += 1;
    } else {
      int horizon_count = CalculateQuadTileCount(64 * scale, bound.width);
      int vertical_count = CalculateQuadTileCount(64 * scale, bound.width);
      count += horizon_count * vertical_count;
    }

    // Frame tiles
    int horizon_frame_size = bound.width - scale * 16;
    int vertical_frame_size = bound.height - scale * 16;
    count += CalculateQuadTileCount(16 * scale, horizon_frame_size) * 2;
    count += CalculateQuadTileCount(16 * scale, vertical_frame_size) * 2;

    // Setup vertex buffer
    quad_buffer.resize(count);
  }

  renderer::Quad* quad_ptr = quad_buffer.data();
  float opacity_norm = static_cast<float>(opacity) / 255.0f;
  float back_opacity_norm = static_cast<float>(back_opacity) / 255.0f;

  // Gen quadangle vertices
  {
    // Stretch / Tile layer
    base::Rect dest_rect(display_offset.x + scale, display_offset.y + scale,
                         bound.width - 2 * scale, bound.height - 2 * scale);
    const base::Rect background_src(0, 0, 64 * scale, 64 * scale);

    int background_quad_count = 0;
    if (stretch) {
      renderer::Quad::SetPositionRect(quad_ptr, dest_rect);
      renderer::Quad::SetTexCoordRect(quad_ptr, background_src);
      background_quad_count += 1;
    } else {
      // Build tiled background quads
      background_quad_count +=
          BuildTiles(background_src, dest_rect,
                     base::Vec4(opacity_norm * back_opacity_norm), quad_ptr);
    }

    // Apply background color
    float background_opacity_norm =
        opacity_norm * (static_cast<float>(back_opacity) / 255.0f);
    for (int i = 0; i < background_quad_count; ++i)
      renderer::Quad::SetColor(&quad_ptr[i], base::Vec4(opacity_norm));

    quad_ptr += background_quad_count;

    // Frame tiles
    base::Rect frame_up(72 * scale, 0, 16 * scale, 8 * scale);
    base::Rect frame_down(72 * scale, 24 * scale, 16 * scale, 8 * scale);
    base::Rect frame_left(64 * scale, 8 * scale, 8 * scale, 16 * scale);
    base::Rect frame_right(88 * scale, 8 * scale, 8 * scale, 16 * scale);

    int frame_quads_count = 0;
    frame_quads_count += BuildTilesAlongAxis<TileAxis::HORIZONTAL>(
        frame_up, base::Vec4(opacity_norm), bound.width - 4 * scale,
        display_offset.y, display_offset.x + 2 * scale, quad_ptr);
    quad_ptr += frame_quads_count;
    frame_quads_count += BuildTilesAlongAxis<TileAxis::HORIZONTAL>(
        frame_down, base::Vec4(opacity_norm), bound.width - 4 * scale,
        display_offset.y + bound.height - 2 * scale,
        display_offset.x + 2 * scale, quad_ptr);
    quad_ptr += frame_quads_count;
    frame_quads_count += BuildTilesAlongAxis<TileAxis::VERTICAL>(
        frame_left, base::Vec4(opacity_norm), bound.height - 4 * scale,
        display_offset.x + 2 * scale, display_offset.y, quad_ptr);
    quad_ptr += frame_quads_count;
    frame_quads_count += BuildTilesAlongAxis<TileAxis::VERTICAL>(
        frame_right, base::Vec4(opacity_norm), bound.height - 4 * scale,
        display_offset.x + 2 * scale,
        display_offset.y + bound.width - 2 * scale, quad_ptr);
    quad_ptr += frame_quads_count;

    // Frame Corners
    base::Rect corner_left_top(64 * scale, 0, 8 * scale, 8 * scale);
    base::Rect corner_right_top(88 * scale, 0, 8 * scale, 8 * scale);
    base::Rect corner_right_bottom(88 * scale, 24 * scale, 8 * scale,
                                   8 * scale);
    base::Rect corner_left_bottom(64 * scale, 24 * scale, 8 * scale, 8 * scale);

    renderer::Quad::SetPositionRect(
        quad_ptr,
        base::Rect(display_offset.x, display_offset.y, 2 * scale, 2 * scale));
    renderer::Quad::SetTexCoordRect(quad_ptr, corner_left_top);
    renderer::Quad::SetColor(quad_ptr, base::Vec4(opacity_norm));
    ++quad_ptr;
    renderer::Quad::SetPositionRect(
        quad_ptr, base::Rect(display_offset.x + bound.width - 2 * scale,
                             display_offset.y, 2 * scale, 2 * scale));
    renderer::Quad::SetTexCoordRect(quad_ptr, corner_right_top);
    renderer::Quad::SetColor(quad_ptr, base::Vec4(opacity_norm));
    ++quad_ptr;
    renderer::Quad::SetPositionRect(
        quad_ptr, base::Rect(display_offset.x + bound.width - 2 * scale,
                             display_offset.y + bound.height - 2 * scale,
                             2 * scale, 2 * scale));
    renderer::Quad::SetTexCoordRect(quad_ptr, corner_right_bottom);
    renderer::Quad::SetColor(quad_ptr, base::Vec4(opacity_norm));
    ++quad_ptr;
    renderer::Quad::SetPositionRect(
        quad_ptr, base::Rect(display_offset.x,
                             display_offset.y + bound.height - 2 * scale,
                             2 * scale, 2 * scale));
    renderer::Quad::SetTexCoordRect(quad_ptr, corner_left_bottom);
    renderer::Quad::SetColor(quad_ptr, base::Vec4(opacity_norm));
    ++quad_ptr;
  }

  agent->background_batch->QueueWrite(*encoder, quad_buffer.data(),
                                      quad_buffer.size());
}

void GPUCompositeControlLayerInternal(renderer::RenderDevice* device,
                                      wgpu::CommandEncoder* encoder,
                                      WindowAgent* agent,
                                      int32_t scale,
                                      const base::Rect& bound,
                                      const base::Rect& cursor_rect,
                                      const base::Vec2i& origin,
                                      TextureAgent* contents,
                                      bool pause,
                                      int32_t pause_index) {
  // Cursor render
  auto build_cursor_internal = [scale](const base::Rect& rect,
                                       base::RectF quad_rects[9]) {
    int w = rect.width;
    int h = rect.height;
    int x1 = rect.x;
    int x2 = x1 + w;
    int y1 = rect.y;
    int y2 = y1 + h;

    int i = 0;
    quad_rects[i++] = base::Rect(x1, y1, scale, scale);
    quad_rects[i++] = base::Rect(x2 - scale, y1, scale, scale);
    quad_rects[i++] = base::Rect(x2 - scale, y2 - scale, scale, scale);
    quad_rects[i++] = base::Rect(x1, y2 - scale, scale, scale);

    quad_rects[i++] = base::Rect(x1, y1 + scale, scale, h - scale * 2);
    quad_rects[i++] = base::Rect(x2 - scale, y1 + scale, scale, h - scale * 2);
    quad_rects[i++] = base::Rect(x1 + scale, y1, w - scale * scale, scale);
    quad_rects[i++] = base::Rect(x1 + scale, y2 - scale, w - scale * 2, scale);

    quad_rects[i++] =
        base::Rect(x1 + scale, y1 + scale, w - scale * 2, h - scale * 2);
  };

  auto build_cursor_quads = [&](const base::Rect& src, const base::Rect& dst,
                                renderer::Quad vert[9]) {
    base::RectF quad_rects[9];

    build_cursor_internal(src, quad_rects);
    for (int i = 0; i < 9; ++i)
      renderer::Quad::SetTexCoordRect(&vert[i], quad_rects[i]);

    build_cursor_internal(dst, quad_rects);
    for (int i = 0; i < 9; ++i)
      renderer::Quad::SetPositionRect(&vert[i], quad_rects[i]);

    return 9;
  };

  auto& quad_buffer = agent->control_cache;
  quad_buffer.resize(9 + 4 + 1);
  auto* quad_ptr = quad_buffer.data();
  base::Vec2i display_offset = bound.Position();

  base::Rect cursor_dest_rect(display_offset.x + cursor_rect.x + 8 * scale,
                              display_offset.y + cursor_rect.y + 8 * scale,
                              cursor_rect.width, cursor_rect.height);
  if (cursor_dest_rect.width > 0 && cursor_dest_rect.height > 0) {
    base::Rect cursor_src(64 * scale, 32 * scale, 16 * scale, 16 * scale);
    quad_ptr += build_cursor_quads(cursor_src, cursor_dest_rect, quad_ptr);
  }

  // Arrows render
  const base::Vec2i scroll =
      (bound.Size() - base::Vec2i(8 * scale)) / base::Vec2i(2);
  base::Rect scroll_arrow_up_pos =
      base::Rect(scroll.x, 2 * scale, 8 * scale, 4 * scale);
  base::Rect scroll_arrow_down_pos =
      base::Rect(scroll.x, bound.height - 6 * scale, 8 * scale, 4 * scale);
  base::Rect scroll_arrow_left_pos =
      base::Rect(2 * scale, scroll.y, 4 * scale, 8 * scale);
  base::Rect scroll_arrow_right_pos =
      base::Rect(bound.width - 6 * scale, scroll.y, 4 * scale, 8 * scale);

  if (contents) {
    base::Rect scroll_arrow_up_src = {152, 16, 16, 8};
    base::Rect scroll_arrow_down_src = {152, 40, 16, 8};
    base::Rect scroll_arrow_left_src = {144, 24, 8, 16};
    base::Rect scroll_arrow_right_src = {168, 24, 8, 16};

    if (origin.x > 0) {
      renderer::Quad::SetPositionRect(quad_ptr, scroll_arrow_left_pos);
      renderer::Quad::SetTexCoordRect(quad_ptr, scroll_arrow_left_src);
      ++quad_ptr;
    }

    if (origin.y > 0) {
      renderer::Quad::SetPositionRect(quad_ptr, scroll_arrow_up_pos);
      renderer::Quad::SetTexCoordRect(quad_ptr, scroll_arrow_up_src);
      ++quad_ptr;
    }

    if ((bound.width - 16 * scale) < (contents->data.GetWidth() - origin.x)) {
      renderer::Quad::SetPositionRect(quad_ptr, scroll_arrow_right_pos);
      renderer::Quad::SetTexCoordRect(quad_ptr, scroll_arrow_right_src);
      ++quad_ptr;
    }

    if ((bound.height - 16 * scale) < (contents->data.GetHeight() - origin.y)) {
      renderer::Quad::SetPositionRect(quad_ptr, scroll_arrow_down_pos);
      renderer::Quad::SetTexCoordRect(quad_ptr, scroll_arrow_down_src);
      ++quad_ptr;
    }
  }

  // Pause render
  base::Rect pause_animation[] = {
      {80 * scale, 32 * scale, 8 * scale, 8 * scale},
      {88 * scale, 32 * scale, 8 * scale, 8 * scale},
      {80 * scale, 40 * scale, 8 * scale, 8 * scale},
      {88 * scale, 40 * scale, 8 * scale, 8 * scale},
  };

  if (pause) {
    renderer::Quad::SetPositionRect(
        quad_ptr, base::RectF((bound.width - 8 * scale) / 2,
                              bound.height - 8 * scale, 8 * scale, 8 * scale));
    renderer::Quad::SetTexCoordRect(quad_ptr, pause_animation[pause_index / 8]);
  }
}

}  // namespace

WindowImpl::WindowImpl(RenderScreenImpl* screen,
                       scoped_refptr<ViewportImpl> parent)
    : GraphicsChild(screen),
      Disposable(screen),
      background_node_(parent ? parent->GetDrawableController()
                              : screen->GetDrawableController(),
                       SortKey()),
      control_node_(parent ? parent->GetDrawableController()
                           : screen->GetDrawableController(),
                    SortKey(0, 2)),
      viewport_(parent),
      cursor_rect_(new RectImpl(base::Rect())) {
  background_node_.RegisterEventHandler(base::BindRepeating(
      &WindowImpl::BackgroundNodeHandlerInternal, base::Unretained(this)));
  control_node_.RegisterEventHandler(base::BindRepeating(
      &WindowImpl::ControlNodeHandlerInternal, base::Unretained(this)));

  agent_ = new WindowAgent;
  screen->PostTask(
      base::BindOnce(&GPUCreateWindowInternal, screen->GetDevice(), agent_));
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

void WindowImpl::Update(ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;
}

scoped_refptr<Viewport> WindowImpl::Get_Viewport(
    ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return nullptr;

  return viewport_;
}

void WindowImpl::Put_Viewport(const scoped_refptr<Viewport>& value,
                              ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  if (viewport_ == value)
    return;

  viewport_ = ViewportImpl::From(value);
  background_node_.RebindController(viewport_->GetDrawableController());
  control_node_.RebindController(viewport_->GetDrawableController());
}

scoped_refptr<Bitmap> WindowImpl::Get_Windowskin(
    ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return nullptr;

  return windowskin_;
}

void WindowImpl::Put_Windowskin(const scoped_refptr<Bitmap>& value,
                                ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  windowskin_ = CanvasImpl::FromBitmap(value);
}

scoped_refptr<Bitmap> WindowImpl::Get_Contents(
    ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return nullptr;

  return contents_;
}

void WindowImpl::Put_Contents(const scoped_refptr<Bitmap>& value,
                              ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  contents_ = CanvasImpl::FromBitmap(value);
}

bool WindowImpl::Get_Stretch(ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return false;

  return stretch_;
}

void WindowImpl::Put_Stretch(const bool& value,
                             ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  stretch_ = value;
}

scoped_refptr<Rect> WindowImpl::Get_CursorRect(
    ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return nullptr;

  return cursor_rect_;
}

void WindowImpl::Put_CursorRect(const scoped_refptr<Rect>& value,
                                ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  *cursor_rect_ = *RectImpl::From(value);
}

bool WindowImpl::Get_Active(ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return false;

  return active_;
}

void WindowImpl::Put_Active(const bool& value,
                            ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  active_ = value;
}

bool WindowImpl::Get_Visible(ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return false;

  return visible_;
}

void WindowImpl::Put_Visible(const bool& value,
                             ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  visible_ = value;
  background_node_.SetNodeVisibility(visible_);
  control_node_.SetNodeVisibility(visible_);
}

bool WindowImpl::Get_Pause(ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return false;

  return pause_;
}

void WindowImpl::Put_Pause(const bool& value, ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  pause_ = value;
}

int32_t WindowImpl::Get_X(ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return 0;

  return bound_.x;
}

void WindowImpl::Put_X(const int32_t& value, ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  bound_.x = value;
}

int32_t WindowImpl::Get_Y(ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return 0;

  return bound_.y;
}

void WindowImpl::Put_Y(const int32_t& value, ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  bound_.y = value;
}

int32_t WindowImpl::Get_Width(ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return 0;

  return bound_.width;
}

void WindowImpl::Put_Width(const int32_t& value,
                           ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  bound_.width = value;
}

int32_t WindowImpl::Get_Height(ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return 0;

  return bound_.height;
}

void WindowImpl::Put_Height(const int32_t& value,
                            ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  bound_.height = value;
}

int32_t WindowImpl::Get_Z(ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return 0;

  return background_node_.GetSortKeys()->weight[0];
}

void WindowImpl::Put_Z(const int32_t& value, ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  background_node_.SetNodeSortWeight(value);
  control_node_.SetNodeSortWeight(value + 2);
}

int32_t WindowImpl::Get_Ox(ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return 0;

  return origin_.x;
}

void WindowImpl::Put_Ox(const int32_t& value, ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  origin_.x = value;
}

int32_t WindowImpl::Get_Oy(ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return 0;

  return origin_.y;
}

void WindowImpl::Put_Oy(const int32_t& value, ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  origin_.y = value;
}

int32_t WindowImpl::Get_Opacity(ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return 0;

  return opacity_;
}

void WindowImpl::Put_Opacity(const int32_t& value,
                             ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  opacity_ = value;
}

int32_t WindowImpl::Get_BackOpacity(ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return 0;

  return back_opacity_;
}

void WindowImpl::Put_BackOpacity(const int32_t& value,
                                 ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  back_opacity_ = value;
}

int32_t WindowImpl::Get_ContentsOpacity(ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return 0;

  return contents_opacity_;
}

void WindowImpl::Put_ContentsOpacity(const int32_t& value,
                                     ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  contents_opacity_ = value;
}

void WindowImpl::OnObjectDisposed() {
  background_node_.DisposeNode();
  control_node_.DisposeNode();

  screen()->PostTask(base::BindOnce(&GPUDestroyWindowInternal, agent_));
  agent_ = nullptr;
}

void WindowImpl::BackgroundNodeHandlerInternal(
    DrawableNode::RenderStage stage,
    DrawableNode::RenderControllerParams* params) {
  if (stage == DrawableNode::RenderStage::BEFORE_RENDER) {
  } else if (stage == DrawableNode::RenderStage::ON_RENDERING) {
  }
}

void WindowImpl::ControlNodeHandlerInternal(
    DrawableNode::RenderStage stage,
    DrawableNode::RenderControllerParams* params) {}

}  // namespace content
