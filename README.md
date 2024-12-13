# Sequoia

Sequoia is a 2D game engine built for learning.

## References

- <https://wiki.libsdl.org/SDL3/FrontPage>
- <http://shader-slang.com/slang/>
- <https://github.com/recp/cglm>
- <https://github.com/TheSpydog/SDL_gpu_examples/tree/main>

## Stream

# Thu Dec 12 2024

Todo

- Make structs
  - Texture,
  - Sprite(used in scene/attached to entities),
  - ShaderAction: A programmable way to apply a shader to a rect,
  - RenderPass,
  - RenderStep(begins and ends SDLrenderpass(S)): Things like RenderTileMap,
    RenderEntities, RenderEffects, RenderUI, RenderMenu
  - Quad,
- Triangle example
- Texture example
- Compute buffer demo

SDL Graphics notes:

A pipeline is a structure specifying the parameters of a graphics pipeline state.
A pipeline has

- V Shader
- F Shader
- Set of vertices
- Primitive type: Triangles, TrianglesByIdx, TriangleStrip, etc
- Color Target Type

A GPUBuffer is a collection of data located on the GPU
A GPUBuffer is a thing that can be "bound" to

- Uniform. Named VertexStorageBuffer in SDL land. Things like Camera transform
- Vertex.
- Texture Binding. Just the combination of a texture and a sampler

DrawGPUPrimitives draws the primitive type of the current pipeline(tri, triByIdx,
TriStrip, etc)

DrawPrimitivesIndirect is SDL lang for triByIdx(comes with vertex data and
offsets in pairs of 3)

Sequioa notes:

Sequioa will have Types of Buckets. Buckets are just various types of Buffers.
Sequioa defines a fixed set of transition types between Bucket types

REAL_LIFE_TODO: Setup my gaming PC downstairs for the entire duration of the
game jam. Goal is to always have a runnable build of the game going.
