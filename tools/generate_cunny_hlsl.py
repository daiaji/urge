#!/usr/bin/env python3
"""Generate URGE embedded compute HLSL from Magpie effects."""

from __future__ import annotations

import hashlib
import re
from dataclasses import dataclass
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]
MAGPIE_ROOT = REPO_ROOT.parent / "Magpie"
MAGPIE_CUNNY = MAGPIE_ROOT / "src" / "Effects" / "CuNNy2"
MAGPIE_ANIME4K = MAGPIE_ROOT / "src" / "Effects" / "Anime4K"


TYPE_DEFS = """#define MF float
#define MF1 float1
#define MF2 float2
#define MF3 float3
#define MF4 float4
#define MF1x2 float1x2
#define MF1x3 float1x3
#define MF1x4 float1x4
#define MF2x1 float2x1
#define MF2x2 float2x2
#define MF2x3 float2x3
#define MF2x4 float2x4
#define MF3x1 float3x1
#define MF3x2 float3x2
#define MF3x3 float3x3
#define MF3x4 float3x4
#define MF4x1 float4x1
#define MF4x2 float4x2
#define MF4x3 float4x3
#define MF4x4 float4x4

MF2 MulAdd(MF2 x, MF2x2 y, MF2 a) { return mul(x, y) + a; }
MF3 MulAdd(MF2 x, MF2x3 y, MF3 a) { return mul(x, y) + a; }
MF4 MulAdd(MF2 x, MF2x4 y, MF4 a) { return mul(x, y) + a; }
MF2 MulAdd(MF3 x, MF3x2 y, MF2 a) { return mul(x, y) + a; }
MF3 MulAdd(MF3 x, MF3x3 y, MF3 a) { return mul(x, y) + a; }
MF4 MulAdd(MF3 x, MF3x4 y, MF4 a) { return mul(x, y) + a; }
MF2 MulAdd(MF4 x, MF4x2 y, MF2 a) { return mul(x, y) + a; }
MF3 MulAdd(MF4 x, MF4x3 y, MF3 a) { return mul(x, y) + a; }
MF4 MulAdd(MF4 x, MF4x4 y, MF4 a) { return mul(x, y) + a; }
"""

COMMON_DEFS = """#define INPUT u_Texture
#define INPUT_sampler u_Texture_sampler
#define T0 u_Texture0
#define T0_sampler u_Texture0_sampler
#define T1 u_Texture1
#define T1_sampler u_Texture1_sampler
#define T2 u_Texture2
#define T2_sampler u_Texture2_sampler
#define T3 u_Texture3
#define T3_sampler u_Texture3_sampler
#define T4 u_Texture4
#define T4_sampler u_Texture4_sampler
#define T5 u_Texture5
#define T5_sampler u_Texture5_sampler
#define T6 u_Texture6
#define T6_sampler u_Texture6_sampler
#define T7 u_Texture7
#define T7_sampler u_Texture7_sampler
#define T8 u_Texture8
#define T8_sampler u_Texture8_sampler
#define T9 u_Texture9
#define T9_sampler u_Texture9_sampler
#define T10 u_Texture10
#define T10_sampler u_Texture10_sampler
#define T11 u_Texture11
#define T11_sampler u_Texture11_sampler
#define SL u_Texture_sampler
#define O(t, x, y) t.SampleLevel(t##_sampler, pos + float2(x, y) * pt, 0)
#define V4 MF4
#define M4 MF4x4

float2 GetInputPt() { return u_InputPt; }
float2 GetOutputPt() { return u_OutputPt; }
uint2 GetInputSize() { return uint2(u_InputSize); }
uint2 GetOutputSize() { return uint2(u_OutputSize); }
uint __Bfe(uint src, uint off, uint bits) { uint mask = (1u << bits) - 1; return (src >> off) & mask; }
uint __BfiM(uint src, uint ins, uint bits) { uint mask = (1u << bits) - 1; return (ins & mask) | (src & (~mask)); }
uint2 Rmp8x8(uint a) { return uint2(__Bfe(a, 1u, 3u), __BfiM(__Bfe(a, 3u, 3u), a, 1u)); }
"""


@dataclass
class PassBlock:
    number: int
    desc: str
    block_size: int
    inputs: list[str]
    outputs: list[str]
    prelude: str
    body: str


def parse_passes(source: str) -> list[PassBlock]:
    matches = list(re.finditer(r"^//!PASS\s+(\d+)\s*$", source, re.MULTILINE))
    passes: list[PassBlock] = []
    for index, match in enumerate(matches):
        start = match.start()
        end = matches[index + 1].start() if index + 1 < len(matches) else len(source)
        chunk = source[start:end]
        number = int(match.group(1))
        desc = re.search(r"^//!DESC\s+(.+)$", chunk, re.MULTILINE)
        block_size = re.search(r"^//!BLOCK_SIZE\s+(\d+)$", chunk, re.MULTILINE)
        inputs = re.search(r"^//!IN\s+(.+)$", chunk, re.MULTILINE)
        outputs = re.search(r"^//!OUT\s+(.+)$", chunk, re.MULTILINE)
        body = re.search(
            rf"void\s+Pass{number}\s*\(uint2\s+blockStart,\s*uint3\s+\w+\)\s*\{{(.*)\}}\s*$",
            chunk,
            re.DOTALL,
        )
        if not desc or not block_size or not inputs or not outputs or not body:
            raise ValueError(f"Failed to parse Pass{number}")
        prelude = chunk[outputs.end():body.start()].strip()
        passes.append(PassBlock(
            number=number,
            desc=desc.group(1).strip(),
            block_size=int(block_size.group(1)),
            inputs=[x.strip() for x in inputs.group(1).split(",")],
            outputs=[x.strip() for x in outputs.group(1).split(",")],
            prelude=prelude,
            body=body.group(1).strip("\n"),
        ))
    return passes


def texture_name(name: str) -> str:
    if name == "INPUT":
        return "u_Texture"
    if name.startswith("tex"):
        return "u_Texture" + name[3:]
    return "u_Texture" + name[1:]


def texture_decls(inputs: list[str]) -> str:
    lines: list[str] = []
    for input_name in inputs:
        name = texture_name(input_name)
        lines.append(f"Texture2D {name};")
        lines.append(f"SamplerState {name}_sampler;")
    return "\n".join(lines)


def output_decls(outputs: list[str]) -> str:
    if outputs == ["OUTPUT"]:
        return "RWTexture2D<float4> u_Output;"
    lines = [f"RWTexture2D<float4> u_Output{i};" for i in range(len(outputs))]
    return "\n".join(lines)


def anime4k_texture_decls(inputs: list[str]) -> str:
    lines: list[str] = []
    for input_name in inputs:
        name = texture_name(input_name)
        lines.append(f"Texture2D {name};")
        lines.append(f"SamplerState {name}_sampler;")
    return "\n".join(lines)


def anime4k_defs(inputs: list[str]) -> str:
    lines = [
        "#define INPUT u_Texture",
        "#define tex1 u_Texture1",
        "#define tex2 u_Texture2",
        "#define tex3 u_Texture3",
        "#define tex4 u_Texture4",
    ]
    if "INPUT" in inputs:
        lines.append("#define sam1 u_Texture_sampler")
    point_input = next((x for x in inputs if x != "INPUT"), None)
    if point_input:
        lines.append(f"#define sam {texture_name(point_input)}_sampler")
    elif "INPUT" in inputs:
        lines.append("#define sam u_Texture_sampler")
    lines.extend([
        "float2 GetInputPt() { return u_InputPt; }",
        "float2 GetOutputPt() { return u_OutputPt; }",
        "uint2 GetInputSize() { return uint2(u_InputSize); }",
        "uint2 GetOutputSize() { return uint2(u_OutputSize); }",
        "uint __Bfe(uint src, uint off, uint bits) { uint mask = (1u << bits) - 1; return (src >> off) & mask; }",
        "uint __BfiM(uint src, uint ins, uint bits) { uint mask = (1u << bits) - 1; return (ins & mask) | (src & (~mask)); }",
        "uint2 Rmp8x8(uint a) { return uint2(__Bfe(a, 1u, 3u), __BfiM(__Bfe(a, 3u, 3u), a, 1u)); }",
    ])
    return "\n".join(lines)


def rewrite_body(pass_block: PassBlock) -> str:
    body = pass_block.body

    if pass_block.outputs == ["OUTPUT"]:
        body = body.replace("float2(GetOutputPt())", "u_OutputPt")
        body = body.replace("GetOutputPt()", "u_OutputPt")
        body = body.replace("OUTPUT[", "u_Output[")
        guarded_lines: list[str] = []
        guarded_index = 0
        for line in body.splitlines():
            match = re.match(r"(\s*)u_Output\[(gxy \+ int2\((\d), (\d)\))\] = (.+);", line)
            if not match:
                guarded_lines.append(line)
                continue
            indent, expr, _x, _y, value = match.groups()
            name = f"out_xy{guarded_index}"
            guarded_lines.append(f"{indent}uint2 {name} = {expr};")
            guarded_lines.append(
                f"{indent}if ({name}.x < uint(u_OutputSize.x) && {name}.y < uint(u_OutputSize.y))")
            guarded_lines.append(f"{indent}\tu_Output[{name}] = {value};")
            guarded_index += 1
        body = "\n".join(guarded_lines)
    else:
        for i, output in enumerate(pass_block.outputs):
            body = body.replace(f"{output}[gxy] =", f"u_Output{i}[gxy] =")

    return body.rstrip()


def rewrite_anime4k_body(pass_block: PassBlock) -> str:
    body = pass_block.body
    if pass_block.outputs == ["OUTPUT"]:
        body = body.replace("OUTPUT[", "u_Output[")
    else:
        for i, output in enumerate(pass_block.outputs):
            body = body.replace(f"{output}[", f"u_Output{i}[")
    return body.rstrip()


def block_shift(block_size: int) -> int:
    if block_size <= 0 or block_size & (block_size - 1):
        raise ValueError(f"Block size must be a power of two: {block_size}")
    return block_size.bit_length() - 1


def render_pass(pass_block: PassBlock, variant: str) -> str:
    body = rewrite_body(pass_block)
    shift = block_shift(pass_block.block_size)
    return f"""// CuNNy_{variant} Pass{pass_block.number}: {pass_block.desc}
extern const std::string kHLSL_CuNNy_{variant}_Pass{pass_block.number}_Pixel = R"(
{texture_decls(pass_block.inputs)}

{output_decls(pass_block.outputs)}

cbuffer ScalingParamsBuffer {{ float2 u_InputSize; float2 u_OutputSize; float2 u_InputPt; float2 u_OutputPt; }};

{TYPE_DEFS}
{COMMON_DEFS}
{pass_block.prelude}
//!BLOCK_SIZE {pass_block.block_size}
//!NUM_THREADS 64

void Pass{pass_block.number}(uint2 blockStart, uint3 tid) {{
{body}
}}

[numthreads(64, 1, 1)]
void CSMain(uint3 tid : SV_GroupThreadID, uint3 gid : SV_GroupID) {{
  Pass{pass_block.number}(gid.xy << {shift}u, tid);
}}
)";
// {len(body)} body chars
"""


def render_anime4k_udl_pass(pass_block: PassBlock) -> str:
    body = rewrite_anime4k_body(pass_block)
    shift = block_shift(pass_block.block_size)
    symbol_index = pass_block.number - 1
    return f"""// Anime4K_Upscale_Denoise_L Magpie Pass{pass_block.number}: {pass_block.desc}
extern const std::string kHLSL_Anime4K_UDL_Pass{symbol_index}_Compute = R"(
{anime4k_texture_decls(pass_block.inputs)}

{output_decls(pass_block.outputs)}

cbuffer ScalingParamsBuffer {{ float2 u_InputSize; float2 u_OutputSize; float2 u_InputPt; float2 u_OutputPt; }};

{TYPE_DEFS}
{anime4k_defs(pass_block.inputs)}
//!BLOCK_SIZE {pass_block.block_size}
//!NUM_THREADS 64

void Pass{pass_block.number}(uint2 blockStart, uint3 threadId) {{
{body}
}}

[numthreads(64, 1, 1)]
void CSMain(uint3 threadId : SV_GroupThreadID, uint3 gid : SV_GroupID) {{
  Pass{pass_block.number}(gid.xy << {shift}u, threadId);
}}
)";
// {len(body)} body chars
"""


def generate_variant(variant: str, source_name: str, output_name: str) -> None:
    source_path = MAGPIE_CUNNY / source_name
    source = source_path.read_text()
    source_hash = hashlib.sha256(source.encode()).hexdigest()
    passes = parse_passes(source)
    if [p.number for p in passes] != [1, 2, 3, 4, 5, 6]:
        raise ValueError(f"Unexpected pass list for {source_name}")
    pass_sources = "\n".join(render_pass(p, variant) for p in passes)
    output = f"""#include "renderer/pipeline/builtin_hlsl.h"

namespace renderer {{

// Generated by tools/generate_cunny_hlsl.py from Magpie {source_name}.
// Source SHA256: {source_hash}
{pass_sources}
}}  // namespace renderer
"""
    (REPO_ROOT / "renderer" / "pipeline" / output_name).write_text(output)


def generate_anime4k_udl() -> None:
    source_name = "Anime4K_Upscale_Denoise_L.hlsl"
    source_path = MAGPIE_ANIME4K / source_name
    source = source_path.read_text()
    source_hash = hashlib.sha256(source.encode()).hexdigest()
    passes = parse_passes(source)
    if [p.number for p in passes] != [1, 2, 3, 4]:
        raise ValueError(f"Unexpected pass list for {source_name}")
    pass_sources = "\n".join(render_anime4k_udl_pass(p) for p in passes)
    output = f"""#include "renderer/pipeline/builtin_hlsl.h"

namespace renderer {{

// Generated by tools/generate_cunny_hlsl.py from Magpie {source_name}.
// Source SHA256: {source_hash}
{pass_sources}
}}  // namespace renderer
"""
    (REPO_ROOT / "renderer" / "pipeline" / "builtin_hlsl_anime4k_udl.cc").write_text(output)


def main() -> None:
    generate_variant("4x16", "CuNNy-4x16-NVL.hlsl", "builtin_hlsl_cunny_4x16.cc")
    generate_variant("4x24", "CuNNy-4x24-NVL.hlsl", "builtin_hlsl_cunny_4x24.cc")
    generate_anime4k_udl()


if __name__ == "__main__":
    main()
