# ![Logo](app/resources/urge_favicon_64.png) Universal Ruby Game Engine (开发中)

## 语言选择 Language selection
- [English](README_EN.md)
- [简体中文](README.md)

## 概览

 - URGE 是一款兼容RGSS 1/2/3系API的游戏引擎, 使用了 SDL3 作为跨平台底层库与 WebGPU/Dawn 作为渲染RHI，提供了 D3D12/Vulkan 等现代API的支持（不再支持OpenGL系与D3D11之前的API）。  
 - 本项目使用 BSD-2-Clause 许可证开源。    
 - 项目风格来自 The Chromium Project。  
 - 项目启发于 Chromium。  
 
## 快照

<img src="app/test/1.png" height="400">

## 特性

- 多线程: 引擎的游戏逻辑与渲染逻辑是分开于独立两个线程的，可以最大限度利用多核处理器的优势。  
- 现代图形API: 得益于 WebGPU 规范，使得引擎支持了 D3D11/D3D12/Vulkan 这类现代图形API，相比OpenGL家族有了更好的性能。  
- 跨平台: 引擎支持 Windows, Linux, Android, WASM/Emscripten 平台。（直到我找到购买苹果产品的理由前 macOS 与 iOS 不提供官方支持）  
- 高性能: 游戏脚本层使用了解耦方式处理，可接入 MRI，MRuby，TruffleRuby 等解释器来最大限度提升脚本语言处理速度。  

## 第三方库

### 包含于项目源码中
- SDL_image - https://github.com/libsdl-org/SDL_image  
- SDL_ttf - https://github.com/libsdl-org/SDL_ttf  
- dav1d - https://github.com/videolan/dav1d  
- imgui - https://github.com/ocornut/imgui  
- concurrentqueue - https://github.com/cameron314/concurrentqueue  
- fiber - https://github.com/paladin-t/fiber  
- inih - https://github.com/benhoyt/inih  

### 外部引用
- dawn - https://dawn.googlesource.com/dawn  
- SDL3 - https://github.com/libsdl-org/SDL  
- freetype - https://github.com/freetype/freetype  
- zlib - https://github.com/madler/zlib  
- physfs - https://github.com/icculus/physfs  

## 联系方式

- 邮箱: admenri0504@gmail.com / admenri@qq.com  
- 网站: https://admenri.com/  

© 2015-2025 Admenri
