# Squash Recovery Audit

## Scope

This note records the recovery audit for changes after
`277a461b859294f04e55536664b50e17f45ab31a`, using
`backup-before-squash` as the reference for content that existed before the
squash/cleanup pass.

The goal of this audit was not to review every feature commit on its own merits.
The goal was to find content that was lost or damaged during cleanup, restore
valid content, and avoid restoring deprecated Anime4K Mode A code.

## Findings

The squash/cleanup pass removed or altered several valid changes that still
belonged in the tree:

- RGSS3 compatibility script behavior in
  `binding/mri/third_party/rgss/rgss3_compat.rb`.
- Audio changes in `content/media/audio_impl.cc`, including BGM volume access,
  ME position access, MIDI SoundFont setup, and ME monitor guards.
- Graphics persistence and dirty-save behavior in the screen/profile path.
- UDL/CuNNy 2x upscaler runtime paths, lazy readiness, auto-fit behavior, and
  direct 2x present path.
- Shader bytecode cache setup and persistence through the render pipeline.
- CuNNy shader declarations and loader/pipeline wiring.
- Console initialization and visibility handling in application startup.

These were restored from `backup-before-squash` and then checked against the
current intent of the project.

## Intentional Differences From Backup

The current tree intentionally does not match `backup-before-squash` in these
areas:

- Anime4K Mode A remains removed. Runtime code, unused shader strings, comments,
  and documentation references were cleaned up instead of restored.
- Confirmed dead artifacts were removed:
  - `Anime4K_Upscale_Denoise_L.png`
  - `MODE A.png`
  - `Magpie.png`
  - `ruby_kernel_20260412_234747.bin`
- `Binding_Upscale::ScalingParams` was kept 16-byte aligned for constant buffer
  layout safety.
- Android logger sink setup was kept fixed so the shared sink vector is populated
  correctly.
- Linux renderer startup was hardened so an OpenGL context is created only when
  the OpenGL backend is actually attempted.
- Crash handler includes and register/backtrace code were guarded for Linux-only
  APIs.
- `dlfcn.h` usage in MRI startup was restricted to Linux.
- GUI rendering gained null guards for missing present textures/views and empty
  FPS history.

## Non-Expected Differences Recovered

The following differences were identified as cleanup drift rather than intended
engine changes, and were restored to the `backup-before-squash` content:

- `AGENTS.md`
- `INTEGRATION.md`
- `binding/mri/mri_util.h`
- `content/canvas/font_context.cc`
- `cunny_udl_pso_lazy_loading_plan.md`

In particular, `binding/mri/mri_util.h` no longer exposes `dlfcn.h` from a common
header.

## Verification Performed

- `git diff --check`
- `ruby -c binding/mri/third_party/rgss/rgss3_compat.rb`
- `cmake --build build -j$(nproc)`
- Residual search for Mode A symbols/references in project-owned code.
- Comparison against `backup-before-squash` with whitespace-at-EOL ignored.

## Remaining Review Targets

The restored tree builds, but these areas still deserve normal feature review or
runtime validation before considering the branch ready for release:

- Real-game validation for UDL/CuNNy modes 6/7/8, including resize, move window,
  auto-fit, and CAS interaction.
- Shader bytecode cache cold-start and warm-start logs on the target backend.
- Linux Vulkan and OpenGL fallback behavior on actual user environments.
- Crash handler behavior on non-Linux platforms.
- ME/BGM interaction and MIDI seek/loop semantics in real RGSS titles.
- Whether the `build_and_deploy.sh` executable bit should be kept as a separate
  housekeeping change.

## Commit Strategy

The safest strategy is to commit the recovery as independent commits first, then
consider rebase or history cleanup later. This branch is already ahead of
`origin/main` while also behind it, so rebasing before preserving the recovery
would increase the chance of repeating the original loss.
