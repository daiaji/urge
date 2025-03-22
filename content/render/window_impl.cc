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
      int horizon_count =
          CalculateQuadTileCount(64 * scale, bound.width - 2 * scale);
      int vertical_count =
          CalculateQuadTileCount(64 * scale, bound.height - 2 * scale);
      count += horizon_count * vertical_count;
    }

    // Frame tiles
    const int horizon_frame_size = bound.width - scale * 16;
    const int vertical_frame_size = bound.height - scale * 16;
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

    // Frame Corners
    base::Rect corner_left_top(64 * scale, 0, 8 * scale, 8 * scale);
    base::Rect corner_right_top(88 * scale, 0, 8 * scale, 8 * scale);
    base::Rect corner_right_bottom(88 * scale, 24 * scale, 8 * scale,
                                   8 * scale);
    base::Rect corner_left_bottom(64 * scale, 24 * scale, 8 * scale, 8 * scale);

    renderer::Quad::SetPositionRect(
        quad_ptr,
        base::Rect(display_offset.x, display_offset.y, 8 * scale, 8 * scale));
    renderer::Quad::SetTexCoordRect(quad_ptr, corner_left_top);
    renderer::Quad::SetColor(quad_ptr, base::Vec4(opacity_norm));
    ++quad_ptr;
    renderer::Quad::SetPositionRect(
        quad_ptr, base::Rect(display_offset.x + bound.width - 8 * scale,
                             display_offset.y, 8 * scale, 8 * scale));
    renderer::Quad::SetTexCoordRect(quad_ptr, corner_right_top);
    renderer::Quad::SetColor(quad_ptr, base::Vec4(opacity_norm));
    ++quad_ptr;
    renderer::Quad::SetPositionRect(
        quad_ptr, base::Rect(display_offset.x + bound.width - 8 * scale,
                             display_offset.y + bound.height - 8 * scale,
                             8 * scale, 8 * scale));
    renderer::Quad::SetTexCoordRect(quad_ptr, corner_right_bottom);
    renderer::Quad::SetColor(quad_ptr, base::Vec4(opacity_norm));
    ++quad_ptr;
    renderer::Quad::SetPositionRect(
        quad_ptr, base::Rect(display_offset.x,
                             display_offset.y + bound.height - 8 * scale,
                             8 * scale, 8 * scale));
    renderer::Quad::SetTexCoordRect(quad_ptr, corner_left_bottom);
    renderer::Quad::SetColor(quad_ptr, base::Vec4(opacity_norm));
    ++quad_ptr;

    // Frame tiles
    base::Rect frame_up(72 * scale, 0, 16 * scale, 8 * scale);
    base::Rect frame_down(72 * scale, 24 * scale, 16 * scale, 8 * scale);
    base::Rect frame_left(64 * scale, 8 * scale, 8 * scale, 16 * scale);
    base::Rect frame_right(88 * scale, 8 * scale, 8 * scale, 16 * scale);

    base::Vec2i frame_up_pos(display_offset.x + 8 * scale, display_offset.y);
    base::Vec2i frame_down_pos(display_offset.x + 8 * scale,
                               display_offset.y + bound.height - 8 * scale);
    base::Vec2i frame_left_pos(display_offset.x, display_offset.y + 8 * scale);
    base::Vec2i frame_right_pos(display_offset.x + bound.width - 8 * scale,
                                display_offset.y + 8 * scale);

    quad_ptr += BuildTilesAlongAxis(TileAxis::HORIZONTAL, frame_up,
                                    frame_up_pos, base::Vec4(opacity_norm),
                                    bound.width - 16 * scale, quad_ptr);
    quad_ptr += BuildTilesAlongAxis(TileAxis::HORIZONTAL, frame_down,
                                    frame_down_pos, base::Vec4(opacity_norm),
                                    bound.width - 16 * scale, quad_ptr);
    quad_ptr += BuildTilesAlongAxis(TileAxis::VERTICAL, frame_left,
                                    frame_left_pos, base::Vec4(opacity_norm),
                                    bound.height - 16 * scale, quad_ptr);
    quad_ptr += BuildTilesAlongAxis(TileAxis::VERTICAL, frame_right,
                                    frame_right_pos, base::Vec4(opacity_norm),
                                    bound.height - 16 * scale, quad_ptr);
  }

  agent->background_batch->QueueWrite(*encoder, quad_buffer.data(),
                                      quad_buffer.size());
}

void GPUCompositeControlLayerInternal(renderer::RenderDevice* device,
                                      wgpu::CommandEncoder* encoder,
                                      WindowAgent* agent,
                                      TextureAgent* windowskin,
                                      int32_t scale,
                                      const base::Rect& bound,
                                      const base::Rect& cursor_rect,
                                      const base::Vec2i& origin,
                                      TextureAgent* contents,
                                      bool pause,
                                      int32_t pause_index,
                                      int32_t cursor_opacity) {
  // Cursor render
  auto build_cursor_internal = [&](const base::Rect& rect,
                                   base::Rect quad_rects[9]) {
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
    base::Rect quad_rects[9];

    build_cursor_internal(src, quad_rects);
    for (int i = 0; i < 9; ++i)
      renderer::Quad::SetTexCoordRect(&vert[i], quad_rects[i]);

    build_cursor_internal(dst, quad_rects);
    for (int i = 0; i < 9; ++i)
      renderer::Quad::SetPositionRect(&vert[i], quad_rects[i]);

    const base::Vec4 cursor_opacity_norm(static_cast<float>(cursor_opacity) /
                                         255.0f);
    for (int i = 0; i < 9; ++i)
      renderer::Quad::SetColor(&vert[i], cursor_opacity_norm);

    return 9;
  };

  auto& quad_buffer = agent->control_cache;
  quad_buffer.reserve(9 + 4 + 1 + 1);
  quad_buffer.clear();
  base::Vec2i display_offset = bound.Position();

  if (windowskin) {
    // Cursor render (9 Quads)
    base::Rect cursor_dest_rect(display_offset.x + cursor_rect.x + 8 * scale,
                                display_offset.y + cursor_rect.y + 8 * scale,
                                cursor_rect.width, cursor_rect.height);
    if (cursor_dest_rect.width > 0 && cursor_dest_rect.height > 0) {
      base::Rect cursor_src(64 * scale, 32 * scale, 16 * scale, 16 * scale);

      renderer::Quad cursors_quads[9];
      build_cursor_quads(cursor_src, cursor_dest_rect, cursors_quads);
      quad_buffer.insert(quad_buffer.end(), std::begin(cursors_quads),
                         std::end(cursors_quads));
    }

    // Arrows render (0-4 Quads)
    const base::Vec2i scroll =
        display_offset +
        (bound.Size() - base::Vec2i(8 * scale)) / base::Vec2i(2);
    base::Rect scroll_arrow_up_pos =
        base::Rect(scroll.x, bound.y + 2 * scale, 8 * scale, 4 * scale);
    base::Rect scroll_arrow_down_pos = base::Rect(
        scroll.x, bound.y + bound.height - 6 * scale, 8 * scale, 4 * scale);
    base::Rect scroll_arrow_left_pos =
        base::Rect(bound.x + 2 * scale, scroll.y, 4 * scale, 8 * scale);
    base::Rect scroll_arrow_right_pos = base::Rect(
        bound.x + bound.width - 6 * scale, scroll.y, 4 * scale, 8 * scale);

    if (contents) {
      base::Rect scroll_arrow_up_src = {76 * scale, 8 * scale, 8 * scale,
                                        4 * scale};
      base::Rect scroll_arrow_down_src = {76 * scale, 20 * scale, 8 * scale,
                                          4 * scale};
      base::Rect scroll_arrow_left_src = {72 * scale, 12 * scale, 4 * scale,
                                          8 * scale};
      base::Rect scroll_arrow_right_src = {84 * scale, 12 * scale, 4 * scale,
                                           8 * scale};

      if (origin.x > 0) {
        renderer::Quad quad;
        renderer::Quad::SetPositionRect(&quad, scroll_arrow_left_pos);
        renderer::Quad::SetTexCoordRect(&quad, scroll_arrow_left_src);
        quad_buffer.push_back(quad);
      }

      if (origin.y > 0) {
        renderer::Quad quad;
        renderer::Quad::SetPositionRect(&quad, scroll_arrow_up_pos);
        renderer::Quad::SetTexCoordRect(&quad, scroll_arrow_up_src);
        quad_buffer.push_back(quad);
      }

      if ((bound.width - 16 * scale) < (contents->size.x - origin.x)) {
        renderer::Quad quad;
        renderer::Quad::SetPositionRect(&quad, scroll_arrow_right_pos);
        renderer::Quad::SetTexCoordRect(&quad, scroll_arrow_right_src);
        quad_buffer.push_back(quad);
      }

      if ((bound.height - 16 * scale) < (contents->size.y - origin.y)) {
        renderer::Quad quad;
        renderer::Quad::SetPositionRect(&quad, scroll_arrow_down_pos);
        renderer::Quad::SetTexCoordRect(&quad, scroll_arrow_down_src);
        quad_buffer.push_back(quad);
      }
    }

    // Pause render (0-1 Quads)
    base::Rect pause_animation[] = {
        {80 * scale, 32 * scale, 8 * scale, 8 * scale},
        {88 * scale, 32 * scale, 8 * scale, 8 * scale},
        {80 * scale, 40 * scale, 8 * scale, 8 * scale},
        {88 * scale, 40 * scale, 8 * scale, 8 * scale},
    };

    if (pause) {
      renderer::Quad quad;
      renderer::Quad::SetPositionRect(
          &quad, base::Rect(display_offset.x + (bound.width - 8 * scale) / 2,
                            display_offset.y + bound.height - 8 * scale,
                            8 * scale, 8 * scale));
      renderer::Quad::SetTexCoordRect(&quad, pause_animation[pause_index / 8]);
      quad_buffer.push_back(quad);
    }
  }

  // Setup control draw count
  agent->control_draw_count = quad_buffer.size();

  // Render contents (0-1 Quads)
  if (contents) {
    base::Vec2i content_position = display_offset;
    content_position.x += 8 * scale - origin.x;
    content_position.y += 8 * scale - origin.y;

    renderer::Quad quad;
    renderer::Quad::SetPositionRect(
        &quad, base::Rect(content_position, contents->size));
    renderer::Quad::SetTexCoordRect(&quad, base::Rect(contents->size));
    quad_buffer.push_back(quad);

    agent->contents_draw_count = 1;
  } else {
    // Disable contents render pass
    agent->contents_draw_count = 0;
  }

  agent->control_batch->QueueWrite(*encoder, quad_buffer.data(),
                                   quad_buffer.size());
}

void GPURenderBackgroundLayerInternal(renderer::RenderDevice* device,
                                      wgpu::RenderPassEncoder* encoder,
                                      wgpu::BindGroup* world_binding,
                                      const base::Rect& last_viewport,
                                      const base::Rect& bound,
                                      WindowAgent* agent,
                                      TextureAgent* windowskin) {
  if (agent->background_cache.size()) {
    auto& pipeline_set = device->GetPipelines()->base;
    auto* pipeline = pipeline_set.GetPipeline(renderer::BlendType::NORMAL);

    auto interact_region = base::MakeIntersect(last_viewport, bound);
    encoder->SetScissorRect(interact_region.x, interact_region.y,
                            interact_region.width, interact_region.height);

    encoder->SetPipeline(*pipeline);
    encoder->SetVertexBuffer(0, **agent->background_batch);
    encoder->SetIndexBuffer(**device->GetQuadIndex(),
                            device->GetQuadIndex()->format());
    encoder->SetBindGroup(0, *world_binding);
    encoder->SetBindGroup(1, windowskin->binding);
    encoder->DrawIndexed(agent->background_cache.size() * 6);

    encoder->SetScissorRect(last_viewport.x, last_viewport.y,
                            last_viewport.width, last_viewport.height);
  }
}

void GPURenderControlLayerInternal(renderer::RenderDevice* device,
                                   wgpu::RenderPassEncoder* encoder,
                                   wgpu::BindGroup* world_binding,
                                   const base::Rect& last_viewport,
                                   const base::Rect& bound,
                                   WindowAgent* agent,
                                   TextureAgent* windowskin,
                                   TextureAgent* contents,
                                   int32_t scale) {
  if (agent->control_draw_count || agent->contents_draw_count) {
    auto& pipeline_set = device->GetPipelines()->base;
    auto* pipeline = pipeline_set.GetPipeline(renderer::BlendType::NORMAL);

    base::Rect inbox_region = bound;
    inbox_region.x += 8 * scale;
    inbox_region.y += 8 * scale;
    inbox_region.width -= 16 * scale;
    inbox_region.height -= 16 * scale;

    encoder->SetPipeline(*pipeline);
    encoder->SetVertexBuffer(0, **agent->control_batch);
    encoder->SetIndexBuffer(**device->GetQuadIndex(),
                            device->GetQuadIndex()->format());
    encoder->SetBindGroup(0, *world_binding);
    if (windowskin && agent->control_draw_count) {
      encoder->SetBindGroup(1, windowskin->binding);
      encoder->DrawIndexed(agent->control_draw_count * 6);
    }

    auto interact_region = base::MakeIntersect(last_viewport, inbox_region);
    encoder->SetScissorRect(interact_region.x, interact_region.y,
                            interact_region.width, interact_region.height);

    if (contents && agent->contents_draw_count) {
      encoder->SetBindGroup(1, contents->binding);
      encoder->DrawIndexed(agent->contents_draw_count * 6, 1,
                           agent->control_draw_count * 6);
    }

    encoder->SetScissorRect(last_viewport.x, last_viewport.y,
                            last_viewport.width, last_viewport.height);
  }
}

}  // namespace

scoped_refptr<Window> Window::New(ExecutionContext* execution_context,
                                  scoped_refptr<Viewport> viewport,
                                  int32_t scale,
                                  ExceptionState& exception_state) {
  return new WindowImpl(execution_context->graphics,
                        ViewportImpl::From(viewport), std::max(1, scale));
}

WindowImpl::WindowImpl(RenderScreenImpl* screen,
                       scoped_refptr<ViewportImpl> parent,
                       int32_t scale)
    : GraphicsChild(screen),
      Disposable(screen),
      background_node_(parent ? parent->GetDrawableController()
                              : screen->GetDrawableController(),
                       SortKey()),
      control_node_(parent ? parent->GetDrawableController()
                           : screen->GetDrawableController(),
                    SortKey(0, 2)),
      scale_(scale),
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

  ++pause_index_;
  if (pause_index_ >= 32)
    pause_index_ = 0;

  cursor_opacity_ += cursor_fade_ ? -8 : 8;
  if (cursor_opacity_ > 255) {
    cursor_opacity_ = 255;
    cursor_fade_ = true;
  } else if (cursor_opacity_ < 128) {
    cursor_opacity_ = 128;
    cursor_fade_ = false;
  }
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

  return background_node_.GetVisibility();
}

void WindowImpl::Put_Visible(const bool& value,
                             ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  background_node_.SetNodeVisibility(value);
  control_node_.SetNodeVisibility(value);
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
  if (windowskin_ && windowskin_->GetAgent()) {
    if (stage == DrawableNode::RenderStage::BEFORE_RENDER) {
      screen()->PostTask(base::BindOnce(&GPUCompositeBackgroundLayerInternal,
                                        params->device, params->command_encoder,
                                        agent_, scale_, bound_, stretch_,
                                        opacity_, back_opacity_));
    } else if (stage == DrawableNode::RenderStage::ON_RENDERING) {
      screen()->PostTask(base::BindOnce(
          &GPURenderBackgroundLayerInternal, params->device,
          params->renderpass_encoder, params->world_binding, params->viewport,
          bound_, agent_, windowskin_->GetAgent()));
    }
  }
}

void WindowImpl::ControlNodeHandlerInternal(
    DrawableNode::RenderStage stage,
    DrawableNode::RenderControllerParams* params) {
  TextureAgent* windowskin_agent =
      windowskin_ ? windowskin_->GetAgent() : nullptr;
  TextureAgent* contents_agent = contents_ ? contents_->GetAgent() : nullptr;

  if (stage == DrawableNode::RenderStage::BEFORE_RENDER) {
    screen()->PostTask(
        base::BindOnce(&GPUCompositeControlLayerInternal, params->device,
                       params->command_encoder, agent_, windowskin_agent,
                       scale_, bound_, cursor_rect_->AsBaseRect(), origin_,
                       contents_agent, pause_, pause_index_, cursor_opacity_));
  } else if (stage == DrawableNode::RenderStage::ON_RENDERING) {
    screen()->PostTask(base::BindOnce(
        &GPURenderControlLayerInternal, params->device,
        params->renderpass_encoder, params->world_binding, params->viewport,
        bound_, agent_, windowskin_agent, contents_agent, scale_));
  }
}

}  // namespace content
