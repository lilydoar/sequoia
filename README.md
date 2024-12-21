# Sequoia

Sequoia is a 2D game engine built for learning purposes.

## References

- <https://wiki.libsdl.org/SDL3/FrontPage>
- <https://github.com/TheSpydog/SDL_gpu_examples/tree/main>
- <http://shader-slang.com/slang/>
- <https://github.com/recp/cglm>
- <http://nothings.org/stb_ds/>
- <https://www.aseprite.org/docs/>

## SDL Notes:

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

Steps to move data between the CPU and GPU

1. Create GPU Transfer Buffer
1. Get a pointer for where I should put the CPU data
1. Memcopy data to that pointer(a function could do data transformation before
   memcopying).
1. Get a pointer for where I should put the GPU data
1. Unmap the pointer. Then encode upload commands(Run UploadToGpuBuffer,
   UploadToGpuTexture, etc)
1. Run the upload(Submit the command buffer)

- The SDL UploadBmdBuf has copy passes. I think I can run multiple copy passes
  before submitting the cmd_buf
- I think I want an interface where I can accumulate upload commands before the
  creation of a copy pass. Then have a function which creates a copy pass,
  applies all the commands, and then ends the copy pass
