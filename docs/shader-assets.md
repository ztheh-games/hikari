# SDL3 GPU shader assets

Hikari uses SDL3's GPU API, not SDL_Renderer or the older standalone SDL_gpu
library. Its shaders are authored in HLSL and packaged as SPIR-V (Vulkan),
DXIL (Direct3D 12), and MSL (Metal), with JSON reflection metadata. Normal game
builds use the checked-in artifacts and do not build or require shadercross,
DXC, SPIRV-Cross, or another project checkout.

## Regeneration

Select an existing native SDL_shadercross executable when configuring:

```powershell
cmake -S . -B build -DHIKARI_SHADERCROSS_EXECUTABLE="C:\path\to\cache\bin\shadercross.exe"
cmake --build build --target regenerate-shaders
```

The local recodr integration maintains a completed host-tool cache under
`%LOCALAPPDATA%\recodr\shadercross` on Windows. Use a compatible completed entry
containing `bin\shadercross.exe`, adjacent `dxcompiler.dll` and `dxil.dll`, and
its `complete` marker. The reviewed recipe uses SDL_shadercross commit
`1ff05bec573988a98ef9e0260b4da44f512b8367`. Paths are machine-specific and should
remain in local CMake configuration.

Hikari invokes the selected compiler in place. It neither changes recodr's
cache fingerprint nor downloads, configures, builds, cleans, or invalidates
that compiler cache. Missing tooling is an explicit regeneration error, not
an automatic bootstrap. An installed compatible native CLI is also supported.
Cross-compilation must select a compiler for the build host, not the game
target.

The regeneration target compiles each HLSL shader to all three formats and
reflects SPIR-V into JSON. Commit the updated HLSL, binaries, MSL, and metadata
together. The runtime validates sampler and uniform counts before creating
pipelines, and reports shader/device errors rather than rendering a blank
image or falling back to another renderer.

## Custom content

Shader artifacts live under `assets/shaders/gpu` and are loaded through
PhysicsFS, including its existing `content.zip` and `custom.zip` precedence.
A custom shader package must include the compiled format required by each
target platform and matching metadata. Existing SFML GLSL `.frag` overrides
must be ported and regenerated; editing HLSL alone does not change runtime
behavior.

Resource bindings follow SDL_gpu's conventions: vertex uniforms use
`b0, space1`; fragment textures/samplers use consecutive `t`/`s` registers in
`space2`; fragment uniforms use `b0, space3`. Vertex semantics use consecutive
`TEXCOORD` locations. Keep the 16-byte projection/palette/fade uniform layouts
in sync with the CPU renderer.

The palette shader preserves the original red-channel index lookup and 8x32
table, including transparent-pixel discard. The screen fade uses the original
discrete brightness levels; it is distinct from alpha-overlay state fades.
