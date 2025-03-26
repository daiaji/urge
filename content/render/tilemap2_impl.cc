// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/render/tilemap2_impl.h"

namespace content {

namespace {

void GPUCreateTilemapInternal(renderer::RenderDevice* device,
                              Tilemap2Agent* agent) {
  agent->batch = renderer::QuadBatch::Make(**device);
}

void GPUDestroyTilemapInternal(Tilemap2Agent* agent) {
  delete agent;
}

}  // namespace

TilemapBitmapImpl::TilemapBitmapImpl(base::WeakPtr<Tilemap2Impl> tilemap)
    : tilemap_(tilemap) {}

TilemapBitmapImpl::~TilemapBitmapImpl() = default;

scoped_refptr<Bitmap> TilemapBitmapImpl::Get(int32_t index,
                                             ExceptionState& exception_state) {
  if (!tilemap_)
    return nullptr;

  auto& bitmaps = tilemap_->bitmaps_;
  if (index < 0 || index >= bitmaps.size()) {
    exception_state.ThrowContentError(ExceptionCode::CONTENT_ERROR,
                                      "Out range of bitmaps.");
    return nullptr;
  }

  return bitmaps[index].bitmap;
}

void TilemapBitmapImpl::Put(int32_t index,
                            scoped_refptr<Bitmap> texture,
                            ExceptionState& exception_state) {
  if (!tilemap_)
    return;

  auto& bitmaps = tilemap_->bitmaps_;
  if (index < 0 || index >= bitmaps.size())
    return exception_state.ThrowContentError(ExceptionCode::CONTENT_ERROR,
                                             "Out range of bitmaps.");

  bitmaps[index].bitmap = CanvasImpl::FromBitmap(texture);
  bitmaps[index].observer = bitmaps[index].bitmap->AddCanvasObserver(
      base::BindRepeating(&Tilemap2Impl::AtlasModifyHandlerInternal, tilemap_));
  tilemap_->atlas_dirty_ = true;
}

Tilemap2Impl::Tilemap2Impl(RenderScreenImpl* screen,
                           scoped_refptr<ViewportImpl> parent,
                           int32_t tilesize)
    : GraphicsChild(screen),
      Disposable(screen),
      ground_node_(parent ? parent->GetDrawableController()
                          : screen->GetDrawableController(),
                   SortKey()),
      above_node_(parent ? parent->GetDrawableController()
                         : screen->GetDrawableController(),
                  SortKey(200)),
      tilesize_(tilesize),
      viewport_(parent) {
  ground_node_.RegisterEventHandler(base::BindRepeating(
      &Tilemap2Impl::GroundNodeHandlerInternal, base::Unretained(this)));
  above_node_.RegisterEventHandler(base::BindRepeating(
      &Tilemap2Impl::AboveNodeHandlerInternal, base::Unretained(this)));

  agent_ = new Tilemap2Agent;
  screen->PostTask(
      base::BindOnce(&GPUCreateTilemapInternal, screen->GetDevice(), agent_));
}

Tilemap2Impl::~Tilemap2Impl() {
  ExceptionState exception_state;
  Dispose(exception_state);
}

void Tilemap2Impl::Dispose(ExceptionState& exception_state) {
  Disposable::Dispose(exception_state);
}

bool Tilemap2Impl::IsDisposed(ExceptionState& exception_state) {
  return Disposable::IsDisposed(exception_state);
}

void Tilemap2Impl::Update(ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;
}

scoped_refptr<TilemapBitmap> Tilemap2Impl::Bitmaps(
    ExceptionState& exception_state) {
  return new TilemapBitmapImpl(weak_ptr_factory_.GetWeakPtr());
}

scoped_refptr<Table> Tilemap2Impl::Get_MapData(
    ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return nullptr;

  return map_data_;
}

void Tilemap2Impl::Put_MapData(const scoped_refptr<Table>& value,
                               ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  map_data_ = TableImpl::From(value);
}

scoped_refptr<Table> Tilemap2Impl::Get_FlashData(
    ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return nullptr;

  return flash_data_;
}

void Tilemap2Impl::Put_FlashData(const scoped_refptr<Table>& value,
                                 ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  flash_data_ = TableImpl::From(value);
}

scoped_refptr<Table> Tilemap2Impl::Get_Passages(
    ExceptionState& exception_state) {
  return Get_Flags(exception_state);
}

void Tilemap2Impl::Put_Passages(const scoped_refptr<Table>& value,
                                ExceptionState& exception_state) {
  Put_Flags(value, exception_state);
}

scoped_refptr<Table> Tilemap2Impl::Get_Flags(ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return nullptr;

  return flags_;
}

void Tilemap2Impl::Put_Flags(const scoped_refptr<Table>& value,
                             ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  flags_ = TableImpl::From(value);
}

scoped_refptr<Viewport> Tilemap2Impl::Get_Viewport(
    ExceptionState& exception_state) {
  return viewport_;
}

void Tilemap2Impl::Put_Viewport(const scoped_refptr<Viewport>& value,
                                ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  if (viewport_ == value)
    return;

  viewport_ = ViewportImpl::From(value);
  ground_node_.RebindController(viewport_->GetDrawableController());
  above_node_.RebindController(viewport_->GetDrawableController());
}

bool Tilemap2Impl::Get_Visible(ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return false;

  return ground_node_.GetVisibility();
}

void Tilemap2Impl::Put_Visible(const bool& value,
                               ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  ground_node_.SetNodeVisibility(value);
  above_node_.SetNodeVisibility(value);
}

int32_t Tilemap2Impl::Get_Ox(ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return 0;

  return origin_.x;
}

void Tilemap2Impl::Put_Ox(const int32_t& value,
                          ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  origin_.x = value;
}

int32_t Tilemap2Impl::Get_Oy(ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return 0;

  return origin_.y;
}

void Tilemap2Impl::Put_Oy(const int32_t& value,
                          ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  origin_.y = value;
}

void Tilemap2Impl::OnObjectDisposed() {
  ground_node_.DisposeNode();
  above_node_.DisposeNode();

  screen()->PostTask(base::BindOnce(&GPUDestroyTilemapInternal, agent_));
  agent_ = nullptr;
}

void Tilemap2Impl::GroundNodeHandlerInternal(
    DrawableNode::RenderStage stage,
    DrawableNode::RenderControllerParams* params) {}

void Tilemap2Impl::AboveNodeHandlerInternal(
    DrawableNode::RenderStage stage,
    DrawableNode::RenderControllerParams* params) {}

void Tilemap2Impl::AtlasModifyHandlerInternal() {
  atlas_dirty_ = true;
}

void Tilemap2Impl::MapDataModifyHandlerInternal() {
  map_buffer_dirty_ = true;
}

}  // namespace content
