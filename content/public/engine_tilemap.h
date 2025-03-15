// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_ENGINE_TILEMAP_H_
#define CONTENT_PUBLIC_ENGINE_TILEMAP_H_

#include "base/memory/ref_counted.h"
#include "content/content_config.h"
#include "content/context/exception_state.h"
#include "content/context/execution_context.h"
#include "content/public/engine_bitmap.h"
#include "content/public/engine_table.h"
#include "content/public/engine_viewport.h"

namespace content {

// IDL generator format:
// Inhert: refcounted only.
// Interface referrence: RGSS Referrence
/*--urge(name:Autotile)--*/
class URGE_RUNTIME_API TilemapAutotile
    : public virtual base::RefCounted<TilemapAutotile> {
 public:
  virtual ~TilemapAutotile() = default;

  /*--urge(name:[])--*/
  virtual scoped_refptr<Bitmap> Get(int32_t index,
                                    ExceptionState& exception_state) = 0;

  /*--urge(name:[]=)--*/
  virtual void Put(int32_t index,
                   scoped_refptr<Bitmap>,
                   ExceptionState& exception_state) = 0;
};

/*--urge(name:Tilemap)--*/
class URGE_RUNTIME_API Tilemap : public base::RefCounted<Tilemap> {
 public:
  virtual ~Tilemap() = default;

  /*--urge(name:initialize)--*/
  static scoped_refptr<Tilemap> New(ExecutionContext* execution_context,
                                    ExceptionState& exception_state);

  /*--urge(name:initialize)--*/
  static scoped_refptr<Tilemap> New(ExecutionContext* execution_context,
                                    scoped_refptr<Viewport> viewport,
                                    ExceptionState& exception_state);

  /*--urge(name:dispose)--*/
  virtual void Dispose(ExceptionState& exception_state) = 0;

  /*--urge(name:disposed?)--*/
  virtual bool IsDisposed(ExceptionState& exception_state) = 0;

  /*--urge(name:update)--*/
  virtual void Update(ExceptionState& exception_state) = 0;

  /*--urge(name:viewport)--*/
  URGE_EXPORT_ATTRIBUTE(Viewport, scoped_refptr<Viewport>);

  /*--urge(name:tileset)--*/
  URGE_EXPORT_ATTRIBUTE(Tileset, scoped_refptr<Bitmap>);

  /*--urge(name:autotiles)--*/
  URGE_EXPORT_ATTRIBUTE(Autotiles, scoped_refptr<TilemapAutotile>);

  /*--urge(name:map_data)--*/
  URGE_EXPORT_ATTRIBUTE(MapData, scoped_refptr<Table>);

  /*--urge(name:flash_data)--*/
  URGE_EXPORT_ATTRIBUTE(FlashData, scoped_refptr<Table>);

  /*--urge(name:priorities)--*/
  URGE_EXPORT_ATTRIBUTE(Priorities, scoped_refptr<Table>);

  /*--urge(name:visible)--*/
  URGE_EXPORT_ATTRIBUTE(Visible, bool);

  /*--urge(name:ox)--*/
  URGE_EXPORT_ATTRIBUTE(Ox, int32_t);

  /*--urge(name:oy)--*/
  URGE_EXPORT_ATTRIBUTE(Oy, int32_t);
};

}  // namespace content

#endif  //! CONTENT_PUBLIC_ENGINE_TILEMAP_H_
