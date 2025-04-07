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

namespace renderer {

class Binding_Base;

class RenderBindingBase {
 public:
  using ShaderBinding =
      Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding>;
  using ShaderVariable =
      Diligent::RefCntAutoPtr<Diligent::IShaderResourceVariable>;

  virtual ~RenderBindingBase() = default;

  RenderBindingBase(const RenderBindingBase&) = delete;
  RenderBindingBase& operator=(const RenderBindingBase&) = delete;

  template <class Ty>
  static std::unique_ptr<Ty> Create(ShaderBinding binding);

  Diligent::IShaderResourceBinding* RawPtr() const { return binding_; }

 protected:
  RenderBindingBase(ShaderBinding binding);

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

///
// Specific Create Function
///

template <>
inline std::unique_ptr<Binding_Base> RenderBindingBase::Create(
    ShaderBinding binding) {
  return std::unique_ptr<Binding_Base>(new Binding_Base(binding));
}

}  // namespace renderer

#endif  // !RENDERER_PIPELINE_RENDER_BINDING_H_
