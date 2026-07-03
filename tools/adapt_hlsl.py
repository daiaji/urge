#!/usr/bin/env python3
"""
Adapt spirv-cross HLSL → URGE-compatible HLSL.

Usage:
  python3 adapt_hlsl.py input_spirv.hlsl output_urge.hlsl [pass_index]
"""
import re, sys, os

def adapt(src: str, pass_index: int = 0) -> str:
    lines = src.split('\n')

    # ── 1. Find spirv-cross variable names ──
    tex_vars = []  # (spirv_tex_var, spirv_sampler_var, register_slot)
    for ln in lines:
        m = re.match(r'Texture2D<float4>\s+(\w+)\s*:\s*register\(t(\d+)', ln)
        if m:
            sv = m.group(1)  # e.g. _27
            # sampler is __27_sampler (double underscore prefix)
            ssv = f'_{sv}_sampler'
            tex_vars.append((sv, ssv, int(m.group(2))))

    # Sort by register slot
    tex_vars.sort(key=lambda x: x[2])

    # ── 2. Determine which textures are actually used in the shader ──
    all_code = '\n'.join(lines)
    used_vars = []
    for sv, ssv, slot in tex_vars:
        # Check if this texture variable appears in code (excluding its own declaration line)
        if sv in all_code and f'Texture2D<float4> {sv}' not in all_code.split('\n')[0]:
            used_vars.append((sv, ssv, slot))
        elif f'Sample({sv}_sampler' not in all_code and f'Sample(__{sv}_sampler' not in all_code and f'Sample(__{sv}_sampler' not in all_code.replace('__', '_'):
            # Also check if the sampler is used
            pass  # keep if sampler is referenced

    # Keep only textures actually used in Sample() calls
    actually_used = []
    for sv, ssv, slot in tex_vars:
        # ssv already has the correct spirv sampler name (e.g., __27_sampler)
        if f'.Sample({ssv}' in all_code:
            actually_used.append((sv, ssv, slot))
    if actually_used:
        tex_vars = actually_used

    # Build rename map, using original bind names where available
    renames = {}
    # Try to determine original bind name from metadata (if available via passes list)
    # Simple heuristic: if a texture's original spirv is registered but the code
    # references it like a STATSMAX texture (bound as second texture in Clamp passes)
    for i, (sv, ssv, slot) in enumerate(tex_vars):
        un = 'u_Texture' if i == 0 else f'u_Texture{i}'
        # Check if this variable is used as a STATSMAX texture (via its sampler ref in all_code)
        # Heuristic: if the sampler is used with .x channel access and appears with min()
        us = f'{un}_sampler'
        renames[sv] = un
        renames[ssv] = us

    # ── 3. Find static float2/float4 vars (uv and output color) ──
    uv_var = None
    out_var = None
    for ln in lines:
        m = re.match(r'static\s+float2\s+(\w+)\s*;', ln)
        if m:
            uv_var = m.group(1)
        # Output could be float4 (RGBA) or float (single channel)
        m = re.match(r'static\s+(?:float4|float)\s+(\w+)\s*;', ln)
        if m and not out_var:
            out_var = m.group(1)

    # ── 4. Find cbuffer param mapping ──
    # spirv: float2 _59_m0 : packoffset(c0) → maps to u_InputPt
    param_field = None
    for ln in lines:
        if 'packoffset(c0)' in ln and 'float2' in ln:
            m = re.search(r'(\w+)\s*:\s*packoffset', ln)
            if m:
                param_field = m.group(1)

    # ── 5. Extract helper functions (before frag_main) ──
    helper_lines = []
    in_helper = False
    brace = 0
    helper_start = 0
    for i, ln in enumerate(lines):
        stripped = ln.strip()
        if not in_helper and re.match(r'float[\d]*\s+\w+\s*\(', stripped):
            in_helper = True
            helper_start = i
            brace = ln.count('{') - ln.count('}')
        elif in_helper:
            brace += ln.count('{') - ln.count('}')
            if brace <= 0:
                for j in range(helper_start, i + 1):
                    helper_lines.append(lines[j])
                in_helper = False

    # But ignore spirv-cross boilerplate (SPIRV_Cross_Input/Output/main)
    # Filter: only keep actual math helpers (contain 'dot', 'max', 'min' etc)
    helper_lines = [ln for ln in helper_lines
                    if len(ln.strip()) > 20 or 'return' in ln or '{' in ln or '}' in ln]

    # ── 6. Extract frag_main body (brace-aware) ──
    frag_body = []
    in_frag = False
    frag_depth = 0
    for ln in lines:
        if 'void frag_main()' in ln:
            in_frag = True
            continue
        if in_frag:
            prev = frag_depth
            frag_depth += ln.count('{') - ln.count('}')
            if prev > 0 and frag_depth == 0:
                break  # closing frag_main reached, don't include this }
            frag_body.append(ln)

    # ── 7. Build output ──
    out = []
    out.append(f'// Mode A Pass {pass_index}')
    out.append('')
    out.append('struct PSInput {')
    out.append('  float4 Pos : SV_Position;')
    out.append('  float2 UV : TEXCOORD0;')
    out.append('};')
    out.append('')

    for i, (sv, ssv, slot) in enumerate(tex_vars):
        un = renames[sv]
        out.append(f'Texture2D {un};')
        out.append(f'SamplerState {un}_sampler;')
    if not tex_vars:
        out.append('Texture2D u_Texture;')
        out.append('SamplerState u_Texture_sampler;')
    out.append('')

    out.append('cbuffer ScalingParamsBuffer {')
    out.append('  float2 u_InputSize;')
    out.append('  float2 u_OutputSize;')
    out.append('  float2 u_InputPt;')
    out.append('  float2 u_OutputPt;')
    if len(tex_vars) > 1:
        out.append('  float4 _Dummy;')
    out.append('};')
    out.append('')

    out.append('struct PSOutput {')
    out.append('  float4 Color : SV_TARGET;')
    out.append('};')
    out.append('')

    # Global static vars for spirv-cross helper functions
    if uv_var:
        out.append(f'static float2 {uv_var};')
    if out_var and out_var != uv_var:
        # Determine type from original spirv declaration
        out_type = 'float4'
        for ln in lines:
            if re.match(rf'static\s+float\s+{re.escape(out_var)}\s*;', ln):
                out_type = 'float'
                break
        out.append(f'static {out_type} {out_var};')
    out.append('')

    # Helper functions: only replace textures and param_field (not uv_var/out_var)
    # Helpers use spirv-cross global static vars for uv; param_field is in cbuffer
    for ln in helper_lines:
        nl = ln
        for old, new in sorted(renames.items(), key=lambda x: -len(x[0])):
            nl = re.sub(r'(?<!\w)' + re.escape(old) + r'(?!\w)', new, nl)
        # param_field replacement (cbuffer name changed → must replace everywhere)
        if param_field:
            if re.search(r'frac\s*\(\s*uv\s*\*\s*' + re.escape(param_field) + r'\s*\)', nl):
                nl = re.sub(r'(?<!\w)' + re.escape(param_field) + r'(?!\w)', 'u_InputSize', nl)
            else:
                nl = re.sub(r'(?<!\w)' + re.escape(param_field) + r'(?!\w)', 'u_InputPt', nl)
        out.append(nl)
    if helper_lines:
        out.append('')

    # PSMain
    out.append('void PSMain(in PSInput PSIn, out PSOutput PSOut) {')
    if uv_var:
        out.append(f'  {uv_var} = PSIn.UV;')
    out.append('  float2 uv = PSIn.UV;')

    frag_opens = sum(ln.count('{') for ln in frag_body)
    frag_closes = sum(ln.count('}') for ln in frag_body)
    extra_close = frag_opens - frag_closes + 1  # +1 for PSMain itself

    for ln in frag_body:
        nl = ln
        if not nl.strip():
            continue
        for old, new in sorted(renames.items(), key=lambda x: -len(x[0])):
            nl = re.sub(r'(?<!\w)' + re.escape(old) + r'(?!\w)', new, nl)
        if uv_var:
            nl = re.sub(r'(?<!\w)' + re.escape(uv_var) + r'(?!\w)', 'uv', nl)
        if out_var:
            nl = re.sub(r'(?<!\w)' + re.escape(out_var) + r'(?!\w)', 'PSOut.Color', nl)
        if param_field:
            if re.search(r'frac\s*\(\s*uv\s*\*\s*' + re.escape(param_field) + r'\s*\)', nl):
                nl = re.sub(r'(?<!\w)' + re.escape(param_field) + r'(?!\w)', 'u_InputSize', nl)
            else:
                nl = re.sub(r'(?<!\w)' + re.escape(param_field) + r'(?!\w)', 'u_InputPt', nl)
        out.append(f'  {nl.rstrip()}')

    for _ in range(extra_close):
        out.append('}')

    # ── 8. Transpose float4x4 from col-major (GLSL) to row-major (URGE) ──
    # spirv-cross preserves GLSL column-major layout. URGE uses PACK_MATRIX_ROW_MAJOR.
    # float4x4(a,b,c,d, e,f,g,h, i,j,k,l, m,n,o,p) in GLSL col-major means:
    #   col0=(a,b,c,d), col1=(e,f,g,h), col2=(i,j,k,l), col3=(m,n,o,p)
    # In HLSL row-major we need:
    #   row0=(a,e,i,m), row1=(b,f,j,n), row2=(c,g,k,o), row3=(d,h,l,p)
    result = '\n'.join(out)
    def _transpose_float4x4(m):
        inner = m.group(1)
        nums = re.findall(r'-?\d+\.\d*(?:[eE][+-]?\d+)?f?', inner)
        if len(nums) < 16:
            # spirv-cross may replace float4(0,0,0,0) with 0.0f.xxxx
            # Pad with the last value
            last = nums[-1] if nums else '0.0'
            nums = nums + [last] * (16 - len(nums))
        nums = nums[:16]
        # Strip 'f' suffix
        nums = [n.rstrip('f') for n in nums]
        # Transpose col-major → row-major
        transposed = [
            nums[0], nums[4], nums[8], nums[12],
            nums[1], nums[5], nums[9], nums[13],
            nums[2], nums[6], nums[10], nums[14],
            nums[3], nums[7], nums[11], nums[15],
        ]
        return 'float4x4(' + ', '.join(transposed) + ')'
    result = re.sub(r'float4x4\s*\(((?:[^()]+|\([^()]*\))*)\)', _transpose_float4x4, result)
    return result

def main():
    if len(sys.argv) < 3:
        print(f'Usage: {sys.argv[0]} input_spirv.hlsl output_urge.hlsl [pass_index]')
        sys.exit(1)
    src_path = sys.argv[1]
    dst_path = sys.argv[2]
    pi = int(sys.argv[3]) if len(sys.argv) > 3 else 0
    with open(src_path) as f:
        result = adapt(f.read(), pi)
    with open(dst_path, 'w') as f:
        f.write(result)
    print(f'Adapted: {dst_path} ({len(result)} bytes)')

if __name__ == '__main__':
    main()
