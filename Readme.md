# ![Logo](app/resources/rgu_favicon_64.png) Universal Ruby Game Engine (in-progress)

## Overview

 - URGE is a simple game engine compatible with RGSS 1/2/3, utilizing SDL3 as the foundation and WebGPU/Dawn for the rendering component.  
 - URGE not only maintains compatibility with the original RGSS but also offers cross-platform support and performance enhancements. Additionally, it provides advanced features such as custom shaders, 3D views and network extensions.  
 - This project is open-source under the BSD-2-Clause license.  

## Features

- Multi-thread: The program operates on a multi-threaded architecture, with several worker threads within the program. Each worker has an interface for task dispatching. The engine decomposes event handling, logic rendering, audio playback, video decoding, network processing, and other tasks into multiple threads.  
- Modern Graphics API: Game graphics rendering uses the WebGPU to balance compatibility and graphics features.  
- Cross-platforms: The engine support platforms for Microsoft Windows, Android, Linux, Emscripten/WASM, etc. (Currently, it is no support plan for Apple's OS.)  
- Highly efficient: The script processing part of the engine uses the modern Ruby interpreter.  

## 3rd party

### Include in Source
- SDL_image - https://github.com/libsdl-org/SDL_image  
- SDL_ttf - https://github.com/libsdl-org/SDL_ttf  
- fiber - https://github.com/paladin-t/fiber  
- dav1d - https://github.com/videolan/dav1d  
- inih - https://github.com/benhoyt/inih  

### Reference on Project
- dawn - https://github.com/google/dawn  
- SDL3 - https://github.com/libsdl-org/SDL  
- freetype - https://github.com/freetype/freetype  

## Contact With

- Mail: admenri0504@gmail.com / admenri@qq.com  
- Website: https://admenri.com/  

© 2015-2025 Admenri
