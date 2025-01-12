// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "renderer/context/device_context.h"

namespace renderer {

DeviceContext::DeviceContext(const wgpu::Device& device)
    : immediate_encoder_(device.CreateCommandEncoder()) {}

DeviceContext::~DeviceContext() {}

std::unique_ptr<DeviceContext> DeviceContext::MakeContextFor(
    RenderDevice* device) {
  return std::unique_ptr<DeviceContext>(new DeviceContext(**device));
}

wgpu::CommandEncoder* DeviceContext::GetImmediateEncoder() {
  return &immediate_encoder_;
}

void DeviceContext::Flush() {
  // Submit pending encoder
  wgpu::CommandBuffer commands = immediate_encoder_.Finish();
  device_.GetQueue().Submit(1, &commands);

  // Rebuild command encoder for next command
  immediate_encoder_ = device_.CreateCommandEncoder();
}

}  // namespace renderer
