
#include "SDL3/SDL.h"
#include "SDL3/SDL_main.h"
#include "SDL_image.h"

#include "base/debug/logging.h"
#include "base/math/rectangle.h"
#include "ui/widget/widget.h"

#include <iostream>
#include <random>

#include "webgpu/webgpu_cpp.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

inline void MakeProjectionMatrix(float* out, const base::Vec2& size) {
  const float aa = 2.0f / size.x;
  const float bb = -2.0f / size.y;
  const float cc = 1.0f;

  memset(out, 0, sizeof(float) * 16);
  out[0] = aa;
  out[5] = bb;
  out[10] = cc;

  out[12] = -1.0f;
  out[13] = 1.0f;
  out[15] = 1.0f;
}

struct Vertex {
  base::Vec4 pos{0, 0, 0, 1};
  base::Vec2 uv;
};

struct UniformParams {
  float projMat[16];
  base::Vec4 texSize;
};

static const char* shaderCode = R"(
struct UniformParams {
  transform: mat4x4<f32>,
  texSize: vec4<f32>,
};

struct VertexOutput {
  @location(0) uv: vec2<f32>,
  @builtin(position) pos: vec4<f32>,
};

@group(0) @binding(0) var<uniform> u_uniform: UniformParams;
@group(0) @binding(1) var u_texture: texture_2d<f32>;
@group(0) @binding(2) var u_sampler: sampler;

@vertex fn vertexMain(
    @builtin(vertex_index) VertexIndex : u32,
    @location(0) pos: vec4<f32>,
    @location(1) uv: vec2<f32>) -> VertexOutput {
  var result: VertexOutput;
  result.pos = u_uniform.transform * pos;
  result.uv = uv * u_uniform.texSize.xy;
  return result;
}

@fragment fn fragmentMain(vertex: VertexOutput) -> @location(0) vec4f {
  let tex = textureSample(u_texture, u_sampler, vertex.uv);
  return tex;
}

)";

void set_rect(Vertex* vert, base::RectF pos, base::RectF tex) {
  vert[0].pos.x = pos.x;
  vert[0].pos.y = pos.y;
  vert[1].pos.x = pos.x + pos.width;
  vert[1].pos.y = pos.y;
  vert[2].pos.x = pos.x + pos.width;
  vert[2].pos.y = pos.y + pos.height;
  vert[3].pos.x = pos.x + pos.width;
  vert[3].pos.y = pos.y + pos.height;
  vert[4].pos.x = pos.x;
  vert[4].pos.y = pos.y + pos.height;
  vert[5].pos.x = pos.x;
  vert[5].pos.y = pos.y;

  vert[0].uv = base::Vec2(tex.x, tex.y);
  vert[1].uv = base::Vec2(tex.x + tex.width, tex.y);
  vert[2].uv = base::Vec2(tex.x + tex.width, tex.y + tex.height);
  vert[3].uv = base::Vec2(tex.x + tex.width, tex.y + tex.height);
  vert[4].uv = base::Vec2(tex.x, tex.y + tex.height);
  vert[5].uv = base::Vec2(tex.x, tex.y);

  vert += 6;
};

wgpu::RenderPipeline CreateRenderPipeline(const wgpu::Device& device,
                                          wgpu::TextureFormat format) {
  wgpu::ShaderModuleWGSLDescriptor wgslDesc{};
  wgslDesc.code = shaderCode;

  wgpu::ShaderModuleDescriptor shaderModuleDescriptor;
  shaderModuleDescriptor.nextInChain = &wgslDesc;
  wgpu::ShaderModule shaderModule =
      device.CreateShaderModule(&shaderModuleDescriptor);

  wgpu::ColorTargetState colorTargetState;
  colorTargetState.format = format;

  wgpu::FragmentState fragmentState;
  fragmentState.module = shaderModule;
  fragmentState.targetCount = 1;
  fragmentState.targets = &colorTargetState;

  wgpu::VertexAttribute attrs[2];
  {
    // vertex
    attrs[0].format = wgpu::VertexFormat::Float32x4;
    attrs[0].offset = 0;
    attrs[0].shaderLocation = 0;

    // texcoord
    attrs[1].format = wgpu::VertexFormat::Float32x2;
    attrs[1].offset = sizeof(float) * 4;
    attrs[1].shaderLocation = 1;
  }

  wgpu::VertexBufferLayout buf_lat;
  buf_lat.attributeCount = _countof(attrs);
  buf_lat.attributes = attrs;
  buf_lat.stepMode = wgpu::VertexStepMode::Vertex;
  buf_lat.arrayStride = sizeof(Vertex);

  wgpu::BindGroupLayoutEntry entries[3];
  entries[0].binding = 0;
  entries[0].visibility = wgpu::ShaderStage::Vertex;
  entries[0].buffer.type = wgpu::BufferBindingType::Uniform;

  entries[1].binding = 1;
  entries[1].visibility = wgpu::ShaderStage::Fragment;
  entries[1].texture.sampleType = wgpu::TextureSampleType::Float;
  entries[1].texture.viewDimension = wgpu::TextureViewDimension::e2D;

  entries[2].binding = 2;
  entries[2].visibility = wgpu::ShaderStage::Fragment;
  entries[2].sampler.type = wgpu::SamplerBindingType::Filtering;

  wgpu::BindGroupLayoutDescriptor binding_desc;
  binding_desc.entryCount = _countof(entries);
  binding_desc.entries = entries;
  wgpu::BindGroupLayout binding = device.CreateBindGroupLayout(&binding_desc);

  wgpu::PipelineLayoutDescriptor pp_lat_desc;
  pp_lat_desc.bindGroupLayoutCount = 1;
  pp_lat_desc.bindGroupLayouts = &binding;
  wgpu::PipelineLayout layout = device.CreatePipelineLayout(&pp_lat_desc);

  wgpu::RenderPipelineDescriptor descriptor;
  descriptor.layout = layout;
  descriptor.vertex.module = shaderModule;
  descriptor.vertex.bufferCount = 1;
  descriptor.vertex.buffers = &buf_lat;
  descriptor.fragment = &fragmentState;

  return device.CreateRenderPipeline(&descriptor);
}

wgpu::Device CreateDevice(const wgpu::Instance& instance,
                          const wgpu::Adapter& adapter) {
  wgpu::DeviceDescriptor device_desc = {};
  device_desc.SetDeviceLostCallback(
      wgpu::CallbackMode::AllowSpontaneous,
      [](const wgpu::Device&, wgpu::DeviceLostReason reason,
         wgpu::StringView message) {
        const char* reasonName = "";
        switch (reason) {
          case wgpu::DeviceLostReason::Unknown:
            reasonName = "Unknown";
            break;
          case wgpu::DeviceLostReason::Destroyed:
            reasonName = "Destroyed";
            break;
          case wgpu::DeviceLostReason::InstanceDropped:
            reasonName = "InstanceDropped";
            break;
          case wgpu::DeviceLostReason::FailedCreation:
            reasonName = "FailedCreation";
            break;
          default:
            break;
        }
        std::cout << "Device lost because of " << reasonName << ": "
                  << std::string_view(message) << '\n';
      });

  device_desc.SetUncapturedErrorCallback(
      [](const wgpu::Device&, wgpu::ErrorType type, wgpu::StringView message) {
        const char* errorTypeName = "";
        switch (type) {
          case wgpu::ErrorType::Validation:
            errorTypeName = "Validation";
            break;
          case wgpu::ErrorType::OutOfMemory:
            errorTypeName = "Out of memory";
            break;
          case wgpu::ErrorType::Unknown:
            errorTypeName = "Unknown";
            break;
          case wgpu::ErrorType::DeviceLost:
            errorTypeName = "Device lost";
            break;
          default:
            break;
        }
        std::cout << errorTypeName << " error: " << std::string_view(message)
                  << '\n';
      });

  wgpu::Device ret_device;
  instance.WaitAny(adapter.RequestDevice(
                       &device_desc, wgpu::CallbackMode::WaitAnyOnly,
                       [&](wgpu::RequestDeviceStatus status,
                           wgpu::Device device, wgpu::StringView message) {
                         if (status != wgpu::RequestDeviceStatus::Success) {
                           std::cout << "Failed to get an device: "
                                     << std::string_view(message) << '\n';
                           return;
                         }

                         ret_device = std::move(device);
                       }),
                   UINT64_MAX);

  return ret_device;
}

wgpu::Adapter CreateAdapter(const wgpu::Instance& instance) {
  wgpu::RequestAdapterOptions options;

  wgpu::Adapter ret_adapter;
  instance.WaitAny(instance.RequestAdapter(
                       &options, wgpu::CallbackMode::WaitAnyOnly,
                       [&](wgpu::RequestAdapterStatus status,
                           wgpu::Adapter adapter, wgpu::StringView message) {
                         if (status != wgpu::RequestAdapterStatus::Success) {
                           std::cout << "Failed to get an adapter: "
                                     << std::string_view(message) << "\n";
                           return;
                         }

                         ret_adapter = std::move(adapter);
                       }),
                   UINT64_MAX);

  return ret_adapter;
}

wgpu::Instance CreateInstance() {
  wgpu::InstanceDescriptor instance_desc = {};
  instance_desc.features.timedWaitAnyEnable = true;
  return wgpu::CreateInstance(&instance_desc);
}

int SDL_main(int argc, char* argv[]) {
  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_AUDIO);

  wgpu::Instance instance = CreateInstance();
  wgpu::Adapter adapter = CreateAdapter(instance);

  wgpu::AdapterInfo info;
  adapter.GetInfo(&info);
  std::cout << "[Adapter] " << std::string_view(info.device) << "\n";

  wgpu::SupportedLimits lmts;
  adapter.GetLimits(&lmts);
  std::cout << "[Adapter] Max bind group: " << lmts.limits.maxBindGroups;

  wgpu::Device device = CreateDevice(instance, adapter);
  wgpu::Queue queue = device.GetQueue();

  std::unique_ptr<ui::Widget> widget(new ui::Widget);
  ui::Widget::InitParams win_params;
  win_params.size = base::Vec2i(800, 600);
  win_params.resizable = true;
  widget->Init(std::move(win_params));

  SDL_PropertiesID win_prop = SDL_GetWindowProperties(widget->AsSDLWindow());

  wgpu::SurfaceSourceWindowsHWND winHWND;
  winHWND.hinstance = ::GetModuleHandle(nullptr);
  winHWND.hwnd = SDL_GetPointerProperty(
      win_prop, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);

  wgpu::SurfaceDescriptor surf_desc;
  surf_desc.nextInChain = &winHWND;
  surf_desc.label = nullptr;

  wgpu::Surface surf = instance.CreateSurface(&surf_desc);
  if (surf == nullptr)
    return 1;

  wgpu::SurfaceCapabilities capabilities;
  surf.GetCapabilities(adapter, &capabilities);

  wgpu::SurfaceConfiguration config;
  config.device = device;
  config.format = capabilities.formats[0];
  config.width = widget->GetSize().x;
  config.height = widget->GetSize().y;
  surf.Configure(&config);

  auto pipeline = CreateRenderPipeline(device, capabilities.formats[0]);

  wgpu::BufferDescriptor vb_desc;
  vb_desc.size = sizeof(Vertex) * 6;
  vb_desc.usage = wgpu::BufferUsage::Vertex | wgpu::BufferUsage::CopyDst;
  auto vbo = device.CreateBuffer(&vb_desc);

  wgpu::BufferDescriptor uni_desc;
  uni_desc.size = sizeof(UniformParams);
  uni_desc.usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst;
  auto ubo = device.CreateBuffer(&uni_desc);

  auto* ss = IMG_Load("test.png");

  wgpu::TextureDescriptor tex_desc;
  tex_desc.size = {(uint32_t)ss->w, (uint32_t)ss->h, 1};
  tex_desc.format = wgpu::TextureFormat::RGBA8Unorm;
  tex_desc.usage = wgpu::TextureUsage::TextureBinding |
                   wgpu::TextureUsage::RenderAttachment |
                   wgpu::TextureUsage::CopyDst;
  wgpu::Texture tex = device.CreateTexture(&tex_desc);

  {
    wgpu::ImageCopyTexture copy_tex;
    copy_tex.texture = tex;

    wgpu::TextureDataLayout dlat;
    dlat.bytesPerRow = ss->pitch;

    wgpu::Extent3D ex3d;
    ex3d.width = ss->w;
    ex3d.height = ss->h;
    ex3d.depthOrArrayLayers = 1;

    queue.WriteTexture(&copy_tex, ss->pixels, ss->h * ss->w * 4, &dlat, &ex3d);
  }

  auto tex_sampler = device.CreateSampler();

  wgpu::BindGroupEntry entries[3];
  entries[0].binding = 0;
  entries[0].buffer = ubo;
  entries[1].binding = 1;
  entries[1].textureView = tex.CreateView();
  entries[2].binding = 2;
  entries[2].sampler = tex_sampler;

  {
    UniformParams upm;
    MakeProjectionMatrix(upm.projMat, widget->GetSize());
    upm.texSize = {1.f / ss->w, 1.f / ss->h, 0, 0};
    queue.WriteBuffer(ubo, 0, (uint8_t*)&upm, sizeof(upm));
  }

  ubo = nullptr;

  wgpu::BindGroupDescriptor bg_desc;
  bg_desc.layout = pipeline.GetBindGroupLayout(0);
  bg_desc.entryCount = _countof(entries);
  bg_desc.entries = entries;

  std::vector<wgpu::BindGroup> groups;
  for (int i = 0; i < 1024; ++i)
    groups.push_back(device.CreateBindGroup(&bg_desc));

  int ii = 0;
  while (true) {
    SDL_Event e;
    SDL_PollEvent(&e);
    if (e.type == SDL_EVENT_QUIT)
      break;
    if (e.type == SDL_EVENT_WINDOW_RESIZED) {
      wgpu::SurfaceConfiguration config;
      config.device = device;
      config.format = capabilities.formats[0];
      config.width = widget->GetSize().x;
      config.height = widget->GetSize().y;
      surf.Configure(&config);
    }

    wgpu::SurfaceTexture surf_tex;
    surf.GetCurrentTexture(&surf_tex);

    wgpu::RenderPassColorAttachment attachment;
    attachment.view = surf_tex.texture.CreateView();
    attachment.loadOp = wgpu::LoadOp::Clear;
    attachment.storeOp = wgpu::StoreOp::Store;
    attachment.clearValue = {0, 0, 1, 1};

    wgpu::RenderPassDescriptor renderpass;
    renderpass.colorAttachmentCount = 1;
    renderpass.colorAttachments = &attachment;

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();

    {
      Vertex verts[6];
      set_rect(verts, {50, 50, 200, 200}, {0, 0, (float)ss->w, (float)ss->h});
      encoder.WriteBuffer(vbo, 0, (uint8_t*)verts, sizeof(verts));
    }

    {
      wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderpass);
      pass.SetViewport(0, 0, widget->GetSize().x, widget->GetSize().y, 0, 0);
      pass.SetPipeline(pipeline);
      pass.SetVertexBuffer(0, vbo);
      pass.SetBindGroup(0, groups[ii]);
      pass.Draw(6);
      pass.End();
    }

    attachment.loadOp = wgpu::LoadOp::Load;

    {
      Vertex verts[6];
      set_rect(verts, {250, 250, 200, 200}, {0, 0, (float)ss->w, (float)ss->h});
      encoder.WriteBuffer(vbo, 0, (uint8_t*)verts, sizeof(verts));
    }

    {
      wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderpass);
      pass.SetViewport(400, 300, widget->GetSize().x - 400,
                       widget->GetSize().y - 300, 0, 0);
      pass.SetPipeline(pipeline);
      pass.SetVertexBuffer(0, vbo);
      pass.SetBindGroup(0, groups[ii]);
      pass.Draw(6);
      pass.End();
    }

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    surf.Present();

    LOG(INFO) << ii;

    ++ii;
    if (ii >= 1024)
      ii = 0;
  }

  groups.clear();

  device = nullptr;

  SDL_Quit();

  return 0;
}
