#include <metal_stdlib>
#include <metal_math>
#include <metal_texture>
using namespace metal;

#line 1334 "diff.meta.slang"
struct _MatrixStorage_float4x4_ColMajornatural_0
{
    array<float4, int(4)> data_0;
};


#line 1334
matrix<float,int(4),int(4)>  unpackStorage_0(_MatrixStorage_float4x4_ColMajornatural_0 _S1)
{

#line 1334
    return matrix<float,int(4),int(4)> (_S1.data_0[int(0)][int(0)], _S1.data_0[int(1)][int(0)], _S1.data_0[int(2)][int(0)], _S1.data_0[int(3)][int(0)], _S1.data_0[int(0)][int(1)], _S1.data_0[int(1)][int(1)], _S1.data_0[int(2)][int(1)], _S1.data_0[int(3)][int(1)], _S1.data_0[int(0)][int(2)], _S1.data_0[int(1)][int(2)], _S1.data_0[int(2)][int(2)], _S1.data_0[int(3)][int(2)], _S1.data_0[int(0)][int(3)], _S1.data_0[int(1)][int(3)], _S1.data_0[int(2)][int(3)], _S1.data_0[int(3)][int(3)]);
}


#line 15 "assets/shaders/sprite.slang"
struct VertexOut_0
{
    float4 position_0 [[position]];
    float2 uv_0 [[user(_SLANG_ATTR)]];
};


#line 15
struct vertexInput_0
{
    float2 position_1 [[attribute(0)]];
    float2 uv_1 [[attribute(1)]];
};


#line 15
struct SLANG_ParameterGroup_Uniform_natural_0
{
    _MatrixStorage_float4x4_ColMajornatural_0 mvp_0;
};


#line 33
struct KernelContext_0
{
    SLANG_ParameterGroup_Uniform_natural_0 constant* Uniform_0;
    texture2d<float, access::sample> atlas_0;
    sampler samplerState_0;
};


#line 22
[[vertex]] VertexOut_0 vertexMain(vertexInput_0 _S2 [[stage_in]], SLANG_ParameterGroup_Uniform_natural_0 constant* Uniform_1 [[buffer(0)]], texture2d<float, access::sample> atlas_1 [[texture(0)]], sampler samplerState_1 [[sampler(0)]])
{

#line 22
    KernelContext_0 kernelContext_0;

#line 22
    (&kernelContext_0)->Uniform_0 = Uniform_1;

#line 22
    (&kernelContext_0)->atlas_0 = atlas_1;

#line 22
    (&kernelContext_0)->samplerState_0 = samplerState_1;

    thread VertexOut_0 out_0;
    (&out_0)->position_0 = (((float4(_S2.position_1, 0.0, 1.0)) * (unpackStorage_0((&kernelContext_0)->Uniform_0->mvp_0))));
    (&out_0)->uv_0 = _S2.uv_1;
    return out_0;
}


#line 4325 "core.meta.slang"
struct pixelOutput_0
{
    float4 output_0 [[color(0)]];
};


#line 4325
struct pixelInput_0
{
    float2 uv_2 [[user(_SLANG_ATTR)]];
};


#line 31 "assets/shaders/sprite.slang"
[[fragment]] pixelOutput_0 fragmentMain(pixelInput_0 _S3 [[stage_in]], float4 position_2 [[position]], SLANG_ParameterGroup_Uniform_natural_0 constant* Uniform_2 [[buffer(0)]], texture2d<float, access::sample> atlas_2 [[texture(0)]], sampler samplerState_2 [[sampler(0)]])
{

#line 31
    KernelContext_0 kernelContext_1;

#line 31
    (&kernelContext_1)->Uniform_0 = Uniform_2;

#line 31
    (&kernelContext_1)->atlas_0 = atlas_2;

#line 31
    (&kernelContext_1)->samplerState_0 = samplerState_2;

#line 31
    pixelOutput_0 _S4 = { (((&kernelContext_1)->atlas_0).sample((samplerState_2), (_S3.uv_2))) };


    return _S4;
}

