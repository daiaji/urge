// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "renderer/pipeline/render_binding.h"

namespace renderer {

RenderBindingBase::RenderBindingBase(ShaderBinding binding)
    : binding_(binding) {}

Binding_Base::Binding_Base(ShaderBinding binding) : RenderBindingBase(binding) {
  u_transform =
      RawPtr()->GetVariableByName(Diligent::SHADER_TYPE_VERTEX, "u_Transform");
  u_texture =
      RawPtr()->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "u_Texture");
}

}  // namespace renderer
