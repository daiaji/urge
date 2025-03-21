// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/render/tilemap_impl.h"

#include "renderer/utils/texture_utils.h"

namespace content {

namespace {

// Referrence: RPGXP Editor - Autotile double click
// Format: Left Top -> Right Top -> Left Bottom -> Right Bottom
const base::RectF kAutotileSrcRects[][4] = {
    {
        {1, 2, 0.5, 0.5},
        {1.5, 2, 0.5, 0.5},
        {1, 2.5, 0.5, 0.5},
        {1.5, 2.5, 0.5, 0.5},
    },
    {
        {2, 0, 0.5, 0.5},
        {1.5, 2, 0.5, 0.5},
        {1, 2.5, 0.5, 0.5},
        {1.5, 2.5, 0.5, 0.5},
    },
    {
        {1, 2, 0.5, 0.5},
        {2.5, 0, 0.5, 0.5},
        {1, 2.5, 0.5, 0.5},
        {1.5, 2.5, 0.5, 0.5},
    },
    {
        {2, 0, 0.5, 0.5},
        {2.5, 0, 0.5, 0.5},
        {1, 2.5, 0.5, 0.5},
        {1.5, 2.5, 0.5, 0.5},
    },
    {
        {1, 2, 0.5, 0.5},
        {1.5, 2, 0.5, 0.5},
        {1, 2.5, 0.5, 0.5},
        {2.5, 0.5, 0.5, 0.5},
    },
    {
        {2, 0, 0.5, 0.5},
        {1.5, 2, 0.5, 0.5},
        {1, 2.5, 0.5, 0.5},
        {2.5, 0.5, 0.5, 0.5},
    },
    {
        {1, 2, 0.5, 0.5},
        {2.5, 0, 0.5, 0.5},
        {1, 2.5, 0.5, 0.5},
        {2.5, 0.5, 0.5, 0.5},
    },
    {
        {2, 0, 0.5, 0.5},
        {2.5, 0, 0.5, 0.5},
        {1, 2.5, 0.5, 0.5},
        {2.5, 0.5, 0.5, 0.5},
    },
    {
        {1, 2, 0.5, 0.5},
        {1.5, 2, 0.5, 0.5},
        {2, 0.5, 0.5, 0.5},
        {1.5, 2.5, 0.5, 0.5},
    },
    {
        {2, 0, 0.5, 0.5},
        {1.5, 2, 0.5, 0.5},
        {2, 0.5, 0.5, 0.5},
        {1.5, 2.5, 0.5, 0.5},
    },
    {
        {1, 2, 0.5, 0.5},
        {2.5, 0, 0.5, 0.5},
        {2, 0.5, 0.5, 0.5},
        {1.5, 2.5, 0.5, 0.5},
    },
    {
        {2, 0, 0.5, 0.5},
        {2.5, 0, 0.5, 0.5},
        {2, 0.5, 0.5, 0.5},
        {1.5, 2.5, 0.5, 0.5},
    },
    {
        {1, 2, 0.5, 0.5},
        {1.5, 2, 0.5, 0.5},
        {2, 0.5, 0.5, 0.5},
        {2.5, 0.5, 0.5, 0.5},
    },
    {
        {2, 0, 0.5, 0.5},
        {1.5, 2, 0.5, 0.5},
        {2, 0.5, 0.5, 0.5},
        {2.5, 0.5, 0.5, 0.5},
    },
    {
        {1, 2, 0.5, 0.5},
        {2.5, 0, 0.5, 0.5},
        {2, 0.5, 0.5, 0.5},
        {2.5, 0.5, 0.5, 0.5},
    },
    {
        {2, 0, 0.5, 0.5},
        {2.5, 0, 0.5, 0.5},
        {2, 0.5, 0.5, 0.5},
        {2.5, 0.5, 0.5, 0.5},
    },
    {
        {0, 2, 0.5, 0.5},
        {0.5, 2, 0.5, 0.5},
        {0, 2.5, 0.5, 0.5},
        {0.5, 2.5, 0.5, 0.5},
    },
    {
        {0, 2, 0.5, 0.5},
        {2.5, 0, 0.5, 0.5},
        {0, 2.5, 0.5, 0.5},
        {0.5, 2.5, 0.5, 0.5},
    },
    {
        {0, 2, 0.5, 0.5},
        {0.5, 2, 0.5, 0.5},
        {0, 2.5, 0.5, 0.5},
        {2.5, 0.5, 0.5, 0.5},
    },
    {
        {0, 2, 0.5, 0.5},
        {2.5, 0, 0.5, 0.5},
        {0, 2.5, 0.5, 0.5},
        {2.5, 0.5, 0.5, 0.5},
    },
    {
        {1, 1, 0.5, 0.5},
        {1.5, 1, 0.5, 0.5},
        {1, 1.5, 0.5, 0.5},
        {1.5, 1.5, 0.5, 0.5},
    },
    {
        {1, 1, 0.5, 0.5},
        {1.5, 1, 0.5, 0.5},
        {1, 1.5, 0.5, 0.5},
        {2.5, 0.5, 0.5, 0.5},
    },
    {
        {1, 1, 0.5, 0.5},
        {1.5, 1, 0.5, 0.5},
        {2, 0.5, 0.5, 0.5},
        {1.5, 1.5, 0.5, 0.5},
    },
    {
        {1, 1, 0.5, 0.5},
        {1.5, 1, 0.5, 0.5},
        {2, 0.5, 0.5, 0.5},
        {2.5, 0.5, 0.5, 0.5},
    },
    {
        {2, 2, 0.5, 0.5},
        {2.5, 2, 0.5, 0.5},
        {2, 2.5, 0.5, 0.5},
        {2.5, 2.5, 0.5, 0.5},
    },
    {
        {2, 2, 0.5, 0.5},
        {2.5, 2, 0.5, 0.5},
        {2, 0.5, 0.5, 0.5},
        {2.5, 2.5, 0.5, 0.5},
    },
    {
        {2, 0, 0.5, 0.5},
        {2.5, 2, 0.5, 0.5},
        {2, 2.5, 0.5, 0.5},
        {2.5, 2.5, 0.5, 0.5},
    },
    {
        {2, 0, 0.5, 0.5},
        {2.5, 2, 0.5, 0.5},
        {2, 0.5, 0.5, 0.5},
        {2.5, 2.5, 0.5, 0.5},
    },
    {
        {1, 3, 0.5, 0.5},
        {1.5, 3, 0.5, 0.5},
        {1, 3.5, 0.5, 0.5},
        {1.5, 3.5, 0.5, 0.5},
    },
    {
        {2, 0, 0.5, 0.5},
        {1.5, 3, 0.5, 0.5},
        {1, 3.5, 0.5, 0.5},
        {1.5, 3.5, 0.5, 0.5},
    },
    {
        {1, 3, 0.5, 0.5},
        {2.5, 0, 0.5, 0.5},
        {1, 3.5, 0.5, 0.5},
        {1.5, 3.5, 0.5, 0.5},
    },
    {
        {2, 0, 0.5, 0.5},
        {2.5, 0, 0.5, 0.5},
        {1, 3.5, 0.5, 0.5},
        {1.5, 3.5, 0.5, 0.5},
    },
    {
        {0, 2, 0.5, 0.5},
        {2.5, 2, 0.5, 0.5},
        {0, 2.5, 0.5, 0.5},
        {2.5, 2.5, 0.5, 0.5},
    },
    {
        {1, 1, 0.5, 0.5},
        {1.5, 1, 0.5, 0.5},
        {1, 3.5, 0.5, 0.5},
        {1.5, 3.5, 0.5, 0.5},
    },
    {
        {0, 1, 0.5, 0.5},
        {0.5, 1, 0.5, 0.5},
        {0, 1.5, 0.5, 0.5},
        {0.5, 1.5, 0.5, 0.5},
    },
    {
        {0, 1, 0.5, 0.5},
        {0.5, 1, 0.5, 0.5},
        {0, 1.5, 0.5, 0.5},
        {2.5, 0.5, 0.5, 0.5},
    },
    {
        {2, 1, 0.5, 0.5},
        {2.5, 1, 0.5, 0.5},
        {2, 1.5, 0.5, 0.5},
        {2.5, 1.5, 0.5, 0.5},
    },
    {
        {2, 1, 0.5, 0.5},
        {2.5, 1, 0.5, 0.5},
        {2, 0.5, 0.5, 0.5},
        {2.5, 1.5, 0.5, 0.5},
    },
    {
        {2, 3, 0.5, 0.5},
        {2.5, 3, 0.5, 0.5},
        {2, 3.5, 0.5, 0.5},
        {2.5, 3.5, 0.5, 0.5},
    },
    {
        {2, 0, 0.5, 0.5},
        {2.5, 3, 0.5, 0.5},
        {2, 3.5, 0.5, 0.5},
        {2.5, 3.5, 0.5, 0.5},
    },
    {
        {0, 3, 0.5, 0.5},
        {0.5, 3, 0.5, 0.5},
        {0, 3.5, 0.5, 0.5},
        {0.5, 3.5, 0.5, 0.5},
    },
    {
        {0, 3, 0.5, 0.5},
        {2.5, 0, 0.5, 0.5},
        {0, 3.5, 0.5, 0.5},
        {0.5, 3.5, 0.5, 0.5},
    },
    {
        {0, 1, 0.5, 0.5},
        {2.5, 1, 0.5, 0.5},
        {0, 1.5, 0.5, 0.5},
        {2.5, 1.5, 0.5, 0.5},
    },
    {
        {0, 1, 0.5, 0.5},
        {0.5, 1, 0.5, 0.5},
        {0, 3.5, 0.5, 0.5},
        {0.5, 3.5, 0.5, 0.5},
    },
    {
        {0, 3, 0.5, 0.5},
        {2.5, 3, 0.5, 0.5},
        {0, 3.5, 0.5, 0.5},
        {2.5, 3.5, 0.5, 0.5},
    },
    {
        {2, 1, 0.5, 0.5},
        {2.5, 1, 0.5, 0.5},
        {2, 3.5, 0.5, 0.5},
        {2.5, 3.5, 0.5, 0.5},
    },
    {
        {0, 1, 0.5, 0.5},
        {2.5, 1, 0.5, 0.5},
        {0, 3.5, 0.5, 0.5},
        {2.5, 3.5, 0.5, 0.5},
    },
    {
        {0, 0, 0.5, 0.5},
        {0.5, 0, 0.5, 0.5},
        {0, 0.5, 0.5, 0.5},
        {0.5, 0.5, 0.5, 0.5},
    },
};

void GPUCreateTilemapInternal(renderer::RenderDevice* device,
                              TilemapAgent* agent) {
  agent->batch = renderer::QuadBatch::Make(**device);
}

void GPUDestroyTilemapInternal(TilemapAgent* agent) {
  delete agent;
}

void GPUMakeAtlasInternal(renderer::RenderDevice* device,
                          wgpu::CommandEncoder* encoder,
                          TilemapAgent* agent,
                          const base::Vec2i& atlas_size,
                          std::list<AtlasCompositeCommand> make_commands) {
  agent->atlas_texture = renderer::CreateTexture2D(
      **device, "tilemap.atlas",
      wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst,
      atlas_size);

  renderer::TextureBindingUniform atlas_uniform;
  wgpu::Buffer atlas_uniform_buffer = renderer::CreateUniformBuffer(
      **device, "tilemap.atlas", wgpu::BufferUsage::None, &atlas_uniform);
  agent->atlas_binding = renderer::TextureBindingUniform::CreateGroup(
      **device, agent->atlas_texture.CreateView(), (*device)->CreateSampler(),
      atlas_uniform_buffer);

  for (auto& it : make_commands) {
    wgpu::TexelCopyTextureInfo copy_src, copy_dst;
    wgpu::Extent3D copy_size;

    copy_src.texture = it.texture->data;
    copy_src.origin.x = it.src_rect.x;
    copy_src.origin.y = it.src_rect.y;

    copy_dst.texture = agent->atlas_texture;
    copy_dst.origin.x = it.dst_pos.x;
    copy_dst.origin.y = it.dst_pos.y;

    copy_size.width = it.src_rect.width;
    copy_size.height = it.src_rect.height;

    encoder->CopyTextureToTexture(&copy_src, &copy_dst, &copy_size);
  }
}

void GPUUploadTilesBatchInternal(
    renderer::RenderDevice* device,
    wgpu::CommandEncoder* encoder,
    TilemapAgent* agent,
    std::vector<renderer::Quad> ground_cache,
    std::vector<std::vector<renderer::Quad>> aboves_cache) {
  agent->batch->QueueWrite(*encoder, ground_cache.data(), ground_cache.size());

  int32_t offset = ground_cache.size();
  for (auto it : aboves_cache) {
    agent->batch->QueueWrite(*encoder, it.data(), it.size(), offset);
    offset += it.size();
  }
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

  return autotiles[index].bitmap;
}

void TilemapAutotileImpl::Put(int32_t index,
                              scoped_refptr<Bitmap> texture,
                              ExceptionState& exception_state) {
  auto& autotiles = tilemap_->autotiles_;
  if (index < 0 || index >= autotiles.size())
    return exception_state.ThrowContentError(ExceptionCode::CONTENT_ERROR,
                                             "Out range of autotiles.");

  autotiles[index].bitmap = CanvasImpl::FromBitmap(texture);
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
    DrawableNode::RenderControllerParams* params) {
  if (stage == DrawableNode::RenderStage::BEFORE_RENDER) {
    UpdateViewportInternal(params->viewport, params->origin);

    if (atlas_dirty_) {
      std::list<AtlasCompositeCommand> commands;
      base::Vec2i size = MakeAtlasInternal(commands);

      screen()->PostTask(base::BindOnce(&GPUMakeAtlasInternal, params->device,
                                        params->command_encoder, agent_, size,
                                        std::move(commands)));
      atlas_dirty_ = false;
    }

    if (map_buffer_dirty_) {
      std::vector<renderer::Quad> ground_cache;
      std::vector<std::vector<renderer::Quad>> aboves_cache;
      ParseMapDataInternal(ground_cache, aboves_cache);

      screen()->PostTask(base::BindOnce(
          &GPUUploadTilesBatchInternal, params->device, params->command_encoder,
          agent_, std::move(ground_cache), std::move(aboves_cache)));
      map_buffer_dirty_ = false;
    }
  } else if (stage == DrawableNode::RenderStage::ON_RENDERING) {
  }
}

void TilemapImpl::AboveNodeHandlerInternal(
    DrawableNode::RenderStage stage,
    DrawableNode::RenderControllerParams* params) {
  if (stage == DrawableNode::RenderStage::ON_RENDERING) {
  }
}

base::Vec2i TilemapImpl::MakeAtlasInternal(
    std::list<AtlasCompositeCommand>& commands) {
  int atlas_height = 28 * tilesize_;
  if (tileset_ || tileset_->GetAgent())
    atlas_height = std::max(atlas_height, tileset_->AsBaseSize().y);

  // Setup commands
  commands.clear();

  // Autotile part
  int offset = 0;
  for (auto& it : autotiles_) {
    if (!it.bitmap || !it.bitmap->GetAgent()) {
      ++offset;
      continue;
    }

    auto autotile_size = it.bitmap->AsBaseSize();
    base::Vec2i dst_pos(0, offset * tilesize_ * 4);

    if (autotile_size.x > 3 * tilesize_ && autotile_size.y > tilesize_) {
      // Animated autotile
      it.type = AutotileType::Animated;
    } else if (autotile_size.x <= 3 * tilesize_ &&
               autotile_size.y > tilesize_) {
      // Static autotile
      dst_pos.x += tilesize_ * 3;
      it.type = AutotileType::Static;
    } else if (autotile_size.x <= 4 * tilesize_ &&
               autotile_size.y <= tilesize_) {
      // Single animated tile
      it.type = AutotileType::SingleAnimated;

      for (int i = 0; i < 4; ++i) {
        base::Rect single_src(base::Vec2i(tilesize_ * i, 0),
                              base::Vec2i(tilesize_));
        base::Vec2i single_dst(base::Vec2i(tilesize_ * i * 3, 0));

        commands.push_back({it.bitmap->GetAgent(), single_src, single_dst});
      }

      ++offset;
      continue;
    } else {
      NOTREACHED();
    }

    commands.push_back({it.bitmap->GetAgent(), autotile_size, dst_pos});

    ++offset;
  }

  // Tileset part
  if (tileset_ && tileset_->GetAgent()) {
    auto tileset_size = tileset_->AsBaseSize();
    base::Vec2i dst_pos(12 * tilesize_, 0);

    commands.push_back({tileset_->GetAgent(), tileset_size, dst_pos});
  }

  return base::Vec2i(tilesize_ * 20, atlas_height);
}

void TilemapImpl::UpdateViewportInternal(const base::Rect& viewport,
                                         const base::Vec2i& viewport_origin) {
  const base::Vec2i tilemap_origin = origin_ + viewport_origin;
  const base::Vec2i viewport_size = viewport.Size();

  base::Rect new_viewport;
  new_viewport.x = tilemap_origin.x / tilesize_;
  new_viewport.y = tilemap_origin.y / tilesize_ - 1;
  new_viewport.width =
      (viewport_size.x / tilesize_) + !!(viewport_size.x % tilesize_) + 1;
  new_viewport.height =
      (viewport_size.y / tilesize_) + !!(viewport_size.y % tilesize_) + 2;

  if (!(new_viewport == render_rect_)) {
    render_rect_ = new_viewport;
    map_buffer_dirty_ = true;
  }

  const base::Vec2i display_offset = tilemap_origin % base::Vec2i(tilesize_);
  render_offset_ =
      viewport.Position() - display_offset - base::Vec2i(0, tilesize_);
}

void TilemapImpl::ParseMapDataInternal(
    std::vector<renderer::Quad>& ground_cache,
    std::vector<std::vector<renderer::Quad>>& aboves_cache) {
  auto set_autotile_pos = [&](base::RectF& pos, int32_t index) {
    switch (index) {
      case 0:  // Left Top
        break;
      case 1:  // Right Top
        pos.x += tilesize_ / 2.0f;
        break;
      case 2:  // Left Bottom
        pos.y += tilesize_ / 2.0f;
        break;
      case 3:  // Right Bottom
        pos.x += tilesize_ / 2.0f;
        pos.y += tilesize_ / 2.0f;
        break;
      default:
        break;
    }
  };

  auto get_priority = [&](int16_t tile_id) -> int {
    if (!priorities_ || tile_id >= priorities_->x_size())
      return 0;

    int16_t value = priorities_->value(tile_id, 0, 0);
    if (value > 5)
      return -1;

    return value;
  };

  auto process_autotile = [&](const base::Vec2i& pos, int16_t tile_id,
                              std::vector<renderer::Quad>* target) {
    // Autotile (0-7)
    int32_t autotile_id = tile_id / 48 - 1;
    // Pattern (0-47)
    int32_t pattern_id = tile_id % 48;

    // Autotile invalid check
    AutotileInfo& info = autotiles_[autotile_id];
    if (!info.bitmap || !info.bitmap->GetAgent())
      return;

    // Generate from autotile type
    switch (info.type) {
      case AutotileType::Animated:
      case AutotileType::Static: {
        const base::RectF* autotile_src = kAutotileSrcRects[pattern_id * 4];
        for (int i = 0; i < 4; ++i) {
          base::RectF tex_src = autotile_src[i];
          tex_src.x = tex_src.x * tilesize_ + 0.5f;
          tex_src.y = tex_src.y * tilesize_ + 0.5f;
          tex_src.width = tex_src.width * tilesize_ - 1.0f;
          tex_src.height = tex_src.height * tilesize_ - 1.0f;

          if (info.type == AutotileType::Static)
            tex_src.x += 3 * tilesize_;
          tex_src.y += autotile_id * 4 * tilesize_;

          base::RectF chunk_pos(pos.x * tilesize_, pos.y * tilesize_,
                                tilesize_ / 2.0f, tilesize_ / 2.0f);
          set_autotile_pos(chunk_pos, i);

          renderer::Quad quad;
          renderer::Quad::SetPositionRect(&quad, chunk_pos);
          renderer::Quad::SetTexCoordRect(&quad, tex_src);
          target->push_back(quad);
        }
      } break;
      case AutotileType::SingleAnimated: {
        const base::RectF single_tex(0.5f,
                                     0.5f + autotile_id * tilesize_ * 4.0f,
                                     tilesize_ - 1.0f, tilesize_ - 1.0f);
        const base::RectF single_pos(pos.x * tilesize_, pos.y * tilesize_,
                                     tilesize_, tilesize_);

        renderer::Quad quad;
        renderer::Quad::SetPositionRect(&quad, single_pos);
        renderer::Quad::SetTexCoordRect(&quad, single_tex);
        target->push_back(quad);
      } break;
      default:
        NOTREACHED();
        break;
    }
  };

  auto get_map_data = [&](int32_t x, int32_t y, int32_t z) {
    auto value_wrap = [&](int32_t value, int32_t range) {
      int32_t res = value % range;
      return res < 0 ? res + range : res;
    };

    return map_data_->value(value_wrap(x, map_data_->x_size()),
                            value_wrap(y, map_data_->y_size()), z);
  };

  auto process_tile = [&](const base::Vec2i& pos, int32_t z) {
    int tile_id =
        get_map_data(pos.x + render_rect_.x, pos.y + render_rect_.y, z);

    if (tile_id < 48)
      return;

    int priority = get_priority(tile_id);
    if (priority == -1)
      return;

    std::vector<renderer::Quad>* target;
    if (!priority) {
      // Ground layer
      target = &ground_cache;
    } else {
      // Above multi layers
      target = &aboves_cache[pos.y + priority];
    }

    if (tile_id < 48 * 8)
      return process_autotile(pos, tile_id, target);

    int tileset_id = tile_id - 48 * 8;
    int tile_x = tileset_id % 8;
    int tile_y = tileset_id / 8;

    base::Vec2 atlas_offset(12 + tile_x, tile_y);
    base::RectF quad_tex(atlas_offset.x * tilesize_ + 0.5f,
                         atlas_offset.y * tilesize_ + 0.5f, tilesize_,
                         tilesize_);
    base::RectF quad_pos(pos.x * tilesize_, pos.y * tilesize_, tilesize_,
                         tilesize_);

    renderer::Quad quad;
    renderer::Quad::SetPositionRect(&quad, quad_pos);
    renderer::Quad::SetTexCoordRect(&quad, quad_tex);
    target->push_back(quad);
  };

  auto process_buffer = [&]() {
    for (int32_t x = 0; x < render_rect_.width; ++x)
      for (int32_t y = 0; y < render_rect_.height; ++y)
        for (int32_t z = 0; z < map_data_->z_size(); ++z)
          process_tile(base::Vec2i(x, y), z);
  };

  // Process map buffer
  int above_layers_count = render_rect_.height + 5;
  aboves_cache.resize(above_layers_count);

  // Begin to parse buffer data
  process_buffer();
}

}  // namespace content
