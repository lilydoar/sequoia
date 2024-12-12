#include <metal_stdlib>
#include <metal_math>
#include <metal_texture>
using namespace metal;

#line 1410 "core.meta.slang"
struct VertexStageOutput_0
{
    float2 texCoord_0 [[user(COARSEVERTEX)]];
    float4 sv_position_0 [[position]];
};


#line 1409
struct vertexInput_0
{
    float3 position_0 [[attribute(0)]];
    float2 texCoord_1 [[attribute(1)]];
};


#line 50 "shaders/slang/textured_quad.slang"
struct KernelContext_0
{
    texture2d<float, access::sample> Texture_0;
    sampler Sampler_0;
};


#line 13
struct CoarseVertex_0
{
    float2 texCoord_2;
};


struct VertexStageOutput_1
{
    CoarseVertex_0 coarseVertex_0;
    float4 sv_position_1;
};


#line 33
[[vertex]] VertexStageOutput_0 vertexMain(vertexInput_0 _S1 [[stage_in]], texture2d<float, access::sample> Texture_1 [[texture(0)]], sampler Sampler_1 [[sampler(0)]])
{

#line 33
    KernelContext_0 kernelContext_0;

#line 33
    (&kernelContext_0)->Texture_0 = Texture_1;

#line 33
    (&kernelContext_0)->Sampler_0 = Sampler_1;


    thread VertexStageOutput_1 output_0;

    (&(&output_0)->coarseVertex_0)->texCoord_2 = _S1.texCoord_1;
    (&output_0)->sv_position_1 = float4(_S1.position_0, 1.0);

#line 39
    thread VertexStageOutput_0 _S2;

#line 39
    (&_S2)->texCoord_0 = output_0.coarseVertex_0.texCoord_2;

#line 39
    (&_S2)->sv_position_0 = output_0.sv_position_1;

    return _S2;
}


#line 26
struct Fragment_0
{
    float4 color_0 [[color(0)]];
};


#line 26
struct pixelInput_0
{
    float2 texCoord_3 [[user(COARSEVERTEX)]];
};


#line 26
struct pixelInput_1
{
    CoarseVertex_0 coarseVertex_1;
};


#line 46
[[fragment]] Fragment_0 fragmentMain(pixelInput_0 _S3 [[stage_in]], texture2d<float, access::sample> Texture_2 [[texture(0)]], sampler Sampler_2 [[sampler(0)]])
{

#line 46
    KernelContext_0 kernelContext_1;

#line 46
    (&kernelContext_1)->Texture_0 = Texture_2;

#line 46
    (&kernelContext_1)->Sampler_0 = Sampler_2;

#line 46
    thread pixelInput_1 _S4;

#line 46
    (&(&_S4)->coarseVertex_1)->texCoord_2 = _S3.texCoord_3;


    thread Fragment_0 output_1;
    (&output_1)->color_0 = (((&kernelContext_1)->Texture_0).sample((Sampler_2), (_S4.coarseVertex_1.texCoord_2)));
    return output_1;
}

