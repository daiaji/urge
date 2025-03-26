// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDER_TILEMAP2_IMPL_H_
#define CONTENT_RENDER_TILEMAP2_IMPL_H_

#include <array>
#include <list>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "content/canvas/canvas_impl.h"
#include "content/common/color_impl.h"
#include "content/common/rect_impl.h"
#include "content/common/table_impl.h"
#include "content/common/tone_impl.h"
#include "content/components/disposable.h"
#include "content/public/engine_tilemap2.h"
#include "content/public/engine_tilemapbitmap.h"
#include "content/render/drawable_controller.h"
#include "content/screen/viewport_impl.h"
#include "renderer/device/render_device.h"

namespace content {

class Tilemap2Impl;

struct Tilemap2Agent {
  std::unique_ptr<renderer::QuadBatch> batch;
};

class TilemapBitmapImpl : public TilemapBitmap {
 public:
  TilemapBitmapImpl(base::WeakPtr<Tilemap2Impl> tilemap);
  ~TilemapBitmapImpl() override;

  TilemapBitmapImpl(const TilemapBitmapImpl&) = delete;
  TilemapBitmapImpl& operator=(const TilemapBitmapImpl&) = delete;

  scoped_refptr<Bitmap> Get(int32_t index,
                            ExceptionState& exception_state) override;
  void Put(int32_t index,
           scoped_refptr<Bitmap> texture,
           ExceptionState& exception_state) override;

 private:
  base::WeakPtr<Tilemap2Impl> tilemap_;
};

class Tilemap2Impl : public Tilemap2, public GraphicsChild, public Disposable {
 public:
  Tilemap2Impl(RenderScreenImpl* screen,
               scoped_refptr<ViewportImpl> parent,
               int32_t tilesize);
  ~Tilemap2Impl() override;

  Tilemap2Impl(const Tilemap2Impl&) = delete;
  Tilemap2Impl& operator=(const Tilemap2Impl&) = delete;

  void Dispose(ExceptionState& exception_state) override;
  bool IsDisposed(ExceptionState& exception_state) override;
  void Update(ExceptionState& exception_state) override;
  scoped_refptr<TilemapBitmap> Bitmaps(
      ExceptionState& exception_state) override;

  URGE_DECLARE_OVERRIDE_ATTRIBUTE(MapData, scoped_refptr<Table>);
  URGE_DECLARE_OVERRIDE_ATTRIBUTE(FlashData, scoped_refptr<Table>);
  URGE_DECLARE_OVERRIDE_ATTRIBUTE(Passages, scoped_refptr<Table>);
  URGE_DECLARE_OVERRIDE_ATTRIBUTE(Flags, scoped_refptr<Table>);
  URGE_DECLARE_OVERRIDE_ATTRIBUTE(Viewport, scoped_refptr<Viewport>);
  URGE_DECLARE_OVERRIDE_ATTRIBUTE(Visible, bool);
  URGE_DECLARE_OVERRIDE_ATTRIBUTE(Ox, int32_t);
  URGE_DECLARE_OVERRIDE_ATTRIBUTE(Oy, int32_t);

 private:
  friend class TilemapBitmapImpl;
  void OnObjectDisposed() override;
  std::string DisposedObjectName() override { return "Tilemap2"; }
  void GroundNodeHandlerInternal(DrawableNode::RenderStage stage,
                                 DrawableNode::RenderControllerParams* params);
  void AboveNodeHandlerInternal(DrawableNode::RenderStage stage,
                                DrawableNode::RenderControllerParams* params);

  void AtlasModifyHandlerInternal();
  void MapDataModifyHandlerInternal();

  struct BitmapInfo {
    scoped_refptr<CanvasImpl> bitmap;
    base::CallbackListSubscription observer;
  };

  DrawableNode ground_node_;
  DrawableNode above_node_;
  Tilemap2Agent* agent_;
  int32_t tilesize_ = 32;
  bool atlas_dirty_ = false;
  bool map_buffer_dirty_ = false;

  std::array<BitmapInfo, 9> bitmaps_;
  scoped_refptr<TableImpl> map_data_;
  scoped_refptr<TableImpl> flash_data_;
  scoped_refptr<TableImpl> flags_;
  scoped_refptr<ViewportImpl> viewport_;
  base::Vec2i origin_;

  base::CallbackListSubscription map_data_observer_;
  base::CallbackListSubscription flash_data_observer_;
  base::CallbackListSubscription flags_observer_;

  base::WeakPtrFactory<Tilemap2Impl> weak_ptr_factory_{this};
};

}  // namespace content

#endif  //! CONTENT_RENDER_TILEMAP2_IMPL_H_
