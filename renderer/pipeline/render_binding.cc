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

Binding_Color::Binding_Color(ShaderBinding binding)
    : RenderBindingBase(binding) {
  u_transform =
      RawPtr()->GetVariableByName(Diligent::SHADER_TYPE_VERTEX, "u_Transform");
}

Binding_Flat::Binding_Flat(ShaderBinding binding) : RenderBindingBase(binding) {
  u_transform =
      RawPtr()->GetVariableByName(Diligent::SHADER_TYPE_VERTEX, "u_Transform");
  u_texture =
      RawPtr()->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "u_Texture");
  u_params =
      RawPtr()->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "u_Effect");
}

Binding_Sprite::Binding_Sprite(ShaderBinding binding)
    : RenderBindingBase(binding) {
  u_transform =
      RawPtr()->GetVariableByName(Diligent::SHADER_TYPE_VERTEX, "u_Transform");
  u_texture =
      RawPtr()->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "u_Texture");
  u_vertices =
      RawPtr()->GetVariableByName(Diligent::SHADER_TYPE_VERTEX, "u_Vertices");
  u_params =
      RawPtr()->GetVariableByName(Diligent::SHADER_TYPE_VERTEX, "u_Params");
}

Binding_AlphaTrans::Binding_AlphaTrans(ShaderBinding binding)
    : RenderBindingBase(binding) {
  u_frozen_texture = RawPtr()->GetVariableByName(Diligent::SHADER_TYPE_PIXEL,
                                                 "u_FrozenTexture");
  u_current_texture = RawPtr()->GetVariableByName(Diligent::SHADER_TYPE_PIXEL,
                                                  "u_CurrentTexture");
}

Binding_VagueTrans::Binding_VagueTrans(ShaderBinding binding)
    : RenderBindingBase(binding) {
  u_frozen_texture = RawPtr()->GetVariableByName(Diligent::SHADER_TYPE_PIXEL,
                                                 "u_FrozenTexture");
  u_current_texture = RawPtr()->GetVariableByName(Diligent::SHADER_TYPE_PIXEL,
                                                  "u_CurrentTexture");
  u_trans_texture = RawPtr()->GetVariableByName(Diligent::SHADER_TYPE_PIXEL,
                                                "u_TransTexture");
}

Binding_Tilemap::Binding_Tilemap(ShaderBinding binding)
    : RenderBindingBase(binding) {
  u_transform =
      RawPtr()->GetVariableByName(Diligent::SHADER_TYPE_VERTEX, "u_Transform");
  u_texture =
      RawPtr()->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "u_Texture");
  u_params =
      RawPtr()->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "u_Params");
}

Binding_Tilemap2::Binding_Tilemap2(ShaderBinding binding)
    : RenderBindingBase(binding) {
  u_transform =
      RawPtr()->GetVariableByName(Diligent::SHADER_TYPE_VERTEX, "u_Transform");
  u_texture =
      RawPtr()->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "u_Texture");
  u_params =
      RawPtr()->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "u_Params");
}

}  // namespace renderer
