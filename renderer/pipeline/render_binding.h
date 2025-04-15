// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef RENDERER_PIPELINE_RENDER_BINDING_H_
#define RENDERER_PIPELINE_RENDER_BINDING_H_

#include "Common/interface/RefCntAutoPtr.hpp"
#include "Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "Graphics/GraphicsEngine/interface/PipelineState.h"
#include "Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "Graphics/GraphicsTools/interface/GraphicsUtilities.h"
#include "Graphics/GraphicsTools/interface/MapHelper.hpp"

#include "base/math/rectangle.h"
#include "base/math/vector.h"

namespace renderer {

class RenderBindingBase {
 public:
  using ShaderBinding =
      Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding>;
  using ShaderVariable =
      Diligent::RefCntAutoPtr<Diligent::IShaderResourceVariable>;

  virtual ~RenderBindingBase() = default;

  RenderBindingBase(const RenderBindingBase&) = delete;
  RenderBindingBase& operator=(const RenderBindingBase&) = delete;

  template <typename Ty>
  static std::unique_ptr<Ty> Create(ShaderBinding binding) {
    return std::unique_ptr<Ty>(new Ty(binding));
  }

  // Raw ptr access
  inline Diligent::IShaderResourceBinding* operator->() { return binding_; }
  inline Diligent::IShaderResourceBinding* operator*() { return binding_; }

 protected:
  RenderBindingBase(ShaderBinding binding);

  Diligent::IShaderResourceBinding* RawPtr() const { return binding_; }

 private:
  ShaderBinding binding_;
};

///
// Binding Define
///

class Binding_Base : public RenderBindingBase {
 public:
  ShaderVariable u_transform;
  ShaderVariable u_texture;

 private:
  friend class RenderBindingBase;
  Binding_Base(ShaderBinding binding);
};

class Binding_Color : public RenderBindingBase {
 public:
  ShaderVariable u_transform;

 private:
  friend class RenderBindingBase;
  Binding_Color(ShaderBinding binding);
};

class Binding_Flat : public RenderBindingBase {
 public:
  struct Params {
    base::Vec4 color;
    base::Vec4 tone;
  };

  ShaderVariable u_transform;
  ShaderVariable u_texture;
  ShaderVariable u_params;

 private:
  friend class RenderBindingBase;
  Binding_Flat(ShaderBinding binding);
};

class Binding_Sprite : public RenderBindingBase {
 public:
  struct Vertex {
    base::Vec4 position;
    base::Vec2 texcoord;
  };

  struct Params {
    base::Vec4 Color;
    base::Vec4 Tone;
    base::Vec2 Position;
    base::Vec2 Origin;
    base::Vec2 Scale;
    float Rotation;
    float Opacity;
    float BushDepth;
    float BushOpacity;
  };

  ShaderVariable u_transform;
  ShaderVariable u_texture;
  ShaderVariable u_vertices;
  ShaderVariable u_params;

 private:
  friend class RenderBindingBase;
  Binding_Sprite(ShaderBinding binding);
};

class Binding_AlphaTrans : public RenderBindingBase {
 public:
  ShaderVariable u_frozen_texture;
  ShaderVariable u_current_texture;

 private:
  friend class RenderBindingBase;
  Binding_AlphaTrans(ShaderBinding binding);
};

class Binding_VagueTrans : public RenderBindingBase {
 public:
  ShaderVariable u_frozen_texture;
  ShaderVariable u_current_texture;
  ShaderVariable u_trans_texture;

 private:
  friend class RenderBindingBase;
  Binding_VagueTrans(ShaderBinding binding);
};

class Binding_Tilemap : public RenderBindingBase {
 public:
  struct Params {
    base::Vec2 offset;
    base::Vec2 tex_size;
    float animate_index;
    float tile_size;
  };

  ShaderVariable u_transform;
  ShaderVariable u_texture;
  ShaderVariable u_params;

 private:
  friend class RenderBindingBase;
  Binding_Tilemap(ShaderBinding binding);
};

class Binding_Tilemap2 : public RenderBindingBase {
 public:
  struct Params {
    base::Vec2 offset;
    base::Vec2 tex_size;
    base::Vec2 animation_offset;
    float tile_size;
  };

  ShaderVariable u_transform;
  ShaderVariable u_texture;
  ShaderVariable u_params;

 private:
  friend class RenderBindingBase;
  Binding_Tilemap2(ShaderBinding binding);
};

}  // namespace renderer

#endif  // !RENDERER_PIPELINE_RENDER_BINDING_H_
