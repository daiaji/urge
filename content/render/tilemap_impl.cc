// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/render/tilemap_impl.h"
#include "tilemap_impl.h"

namespace content {

namespace {

void GPUCreateTilemapInternal(renderer::RenderDevice* device,
                              TilemapAgent* agent) {
  agent->batch = renderer::QuadBatch::Make(**device);
}

void GPUDestroyTilemapInternal(TilemapAgent* agent) {
  delete agent;
}

}  // namespace

TilemapAutotileImpl::TilemapAutotileImpl(base::WeakPtr<TilemapImpl> tilemap)
    : tilemap_(tilemap) {}

TilemapAutotileImpl::~TilemapAutotileImpl() = default;

scoped_refptr<Bitmap> TilemapAutotileImpl::Get(
    int32_t index,
    ExceptionState& exception_state) {
  auto& autotiles = tilemap_->autotiles_;
  if (index < 0 || index >= autotiles.size()) {
    exception_state.ThrowContentError(ExceptionCode::CONTENT_ERROR,
                                      "Out range of autotiles.");
    return nullptr;
  }

  return autotiles[index];
}

void TilemapAutotileImpl::Put(int32_t index,
                              scoped_refptr<Bitmap> texture,
                              ExceptionState& exception_state) {
  auto& autotiles = tilemap_->autotiles_;
  if (index < 0 || index >= autotiles.size())
    return exception_state.ThrowContentError(ExceptionCode::CONTENT_ERROR,
                                             "Out range of autotiles.");

  autotiles[index] = CanvasImpl::FromBitmap(texture);
  tilemap_->atlas_dirty_ = true;
}

scoped_refptr<Tilemap> Tilemap::New(ExecutionContext* execution_context,
                                    scoped_refptr<Viewport> viewport,
                                    int32_t tilesize,
                                    ExceptionState& exception_state) {
  return new TilemapImpl(execution_context->graphics,
                         ViewportImpl::From(viewport), std::min(16, tilesize));
}

TilemapImpl::TilemapImpl(RenderScreenImpl* screen,
                         scoped_refptr<ViewportImpl> parent,
                         int32_t tilesize)
    : GraphicsChild(screen),
      Disposable(screen),
      ground_node_(parent ? parent->GetDrawableController()
                          : screen->GetDrawableController(),
                   SortKey()),
      tilesize_(tilesize),
      viewport_(parent) {
  ground_node_.RegisterEventHandler(base::BindRepeating(
      &TilemapImpl::GroundNodeHandlerInternal, base::Unretained(this)));

  agent_ = new TilemapAgent;
  screen->PostTask(
      base::BindOnce(&GPUCreateTilemapInternal, screen->GetDevice(), agent_));
}

TilemapImpl::~TilemapImpl() {
  ExceptionState exception_state;
  Dispose(exception_state);
}

void TilemapImpl::Dispose(ExceptionState& exception_state) {
  Disposable::Dispose(exception_state);
}

bool TilemapImpl::IsDisposed(ExceptionState& exception_state) {
  return Disposable::IsDisposed(exception_state);
}

void TilemapImpl::Update(ExceptionState& exception_state) {}

scoped_refptr<TilemapAutotile> TilemapImpl::Autotile(
    ExceptionState& exception_state) {
  return scoped_refptr<TilemapAutotile>();
}

scoped_refptr<Viewport> TilemapImpl::Get_Viewport(
    ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return nullptr;

  return viewport_;
}

void TilemapImpl::Put_Viewport(const scoped_refptr<Viewport>& value,
                               ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  if (viewport_ == value)
    return;

  viewport_ = ViewportImpl::From(value);
  ground_node_.RebindController(viewport_->GetDrawableController());
  for (auto& it : above_nodes_)
    it.RebindController(viewport_->GetDrawableController());
}

scoped_refptr<Bitmap> TilemapImpl::Get_Tileset(
    ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return nullptr;

  return tileset_;
}

void TilemapImpl::Put_Tileset(const scoped_refptr<Bitmap>& value,
                              ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  tileset_ = CanvasImpl::FromBitmap(value);
}

scoped_refptr<Table> TilemapImpl::Get_MapData(ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return nullptr;

  return map_data_;
}

void TilemapImpl::Put_MapData(const scoped_refptr<Table>& value,
                              ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  map_data_ = TableImpl::From(value);
}

scoped_refptr<Table> TilemapImpl::Get_FlashData(
    ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return nullptr;

  return flash_data_;
}

void TilemapImpl::Put_FlashData(const scoped_refptr<Table>& value,
                                ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  flash_data_ = TableImpl::From(value);
}

scoped_refptr<Table> TilemapImpl::Get_Priorities(
    ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return nullptr;

  return priorities_;
}

void TilemapImpl::Put_Priorities(const scoped_refptr<Table>& value,
                                 ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  priorities_ = TableImpl::From(value);
}

bool TilemapImpl::Get_Visible(ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return 0;

  return ground_node_.GetVisibility();
}

void TilemapImpl::Put_Visible(const bool& value,
                              ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  ground_node_.SetNodeVisibility(value);
  for (auto& it : above_nodes_)
    it.SetNodeVisibility(value);
}

int32_t TilemapImpl::Get_Ox(ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return 0;

  return origin_.x;
}

void TilemapImpl::Put_Ox(const int32_t& value,
                         ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  origin_.x = value;
}

int32_t TilemapImpl::Get_Oy(ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return 0;

  return origin_.y;
}

void TilemapImpl::Put_Oy(const int32_t& value,
                         ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  origin_.y = value;
}

void TilemapImpl::OnObjectDisposed() {
  ground_node_.DisposeNode();
  above_nodes_.clear();

  screen()->PostTask(base::BindOnce(&GPUDestroyTilemapInternal, agent_));
  agent_ = nullptr;
}

void TilemapImpl::GroundNodeHandlerInternal(
    DrawableNode::RenderStage stage,
    DrawableNode::RenderControllerParams* params) {}

void TilemapImpl::AboveNodeHandlerInternal(
    DrawableNode::RenderStage stage,
    DrawableNode::RenderControllerParams* params) {}

}  // namespace content
