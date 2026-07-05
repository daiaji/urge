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

    # Build rename map - ALL textures map to u_Texture (single-texture pipeline)
    renames = {}
    for i, (sv, ssv, slot) in enumerate(tex_vars):
        renames[sv] = 'u_Texture'
        # Map __27_sampler → u_Texture_sampler (all use same sampler)
        renames[ssv] = 'u_Texture_sampler'

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

    # Force single texture only (all texture samples map to u_Texture)
    out.append('Texture2D u_Texture;')
    out.append('SamplerState u_Texture_sampler;')
    out.append('')

    out.append('cbuffer ScalingParamsBuffer {')
    out.append('  float2 u_InputSize;')
    out.append('  float2 u_OutputSize;')
    out.append('  float2 u_InputPt;')
    out.append('  float2 u_OutputPt;')
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

    result = '\n'.join(out)
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
