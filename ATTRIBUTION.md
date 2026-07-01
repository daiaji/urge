# Attribution & Third-Party Notices

URGE is licensed under BSD 2-Clause (see LICENCE.txt). Portions of this software are derived from the following third-party projects:

## Magpie (GPL-3.0)

- **Source**: https://github.com/Blinue/Magpie
- **License**: GPL-3.0
- **Used in**: `renderer/pipeline/builtin_hlsl.cc`
- **Components**:
  - Lanczos3 shader (port from Magpie `src/Effects/Lanczos.hlsl`, originally from libretro common-shaders)
  - Bicubic shader (port from Magpie `src/Effects/Bicubic.hlsl`, originally from Cemu graphic packs)

```
Copyright (C) 2021-2024 Blinue

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
```

## AMD FidelityFX CAS (MIT)

- **Source**: https://github.com/GPUOpen-Effects/FidelityFX-CAS
- **License**: MIT
- **Used in**: CAS shader (referenced via Magpie wrapper)

## Anime4K (MIT)

- **Source**: https://github.com/bloc97/Anime4K
- **License**: MIT
- **Used in**: Anime4K DoG shader

Copyright (c) 2019-2021 bloc97
All rights reserved.

## mkxp-z (GPL-2.0)

- **Source**: https://github.com/mkxp-z/mkxp-z
- **License**: GPL-2.0
- **Used as**: Architecture reference only (no direct code copying)
