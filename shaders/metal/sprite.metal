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


#line 16 "shaders/slang/sprite.slang"
struct VOut_0
{
    float4 position_0 [[position]];
    float2 uv_0 [[user(_SLANG_ATTR)]];
};


#line 1409 "core.meta.slang"
struct vertexInput_0
{
    float3 position_1 [[attribute(0)]];
    float2 uv_1 [[attribute(1)]];
};


#line 1409
struct Camera_natural_0
{
    _MatrixStorage_float4x4_ColMajornatural_0 mvp_0;
};


#line 34 "shaders/slang/sprite.slang"
struct KernelContext_0
{
    Camera_natural_0 constant* camera_0;
    texture2d<float, access::sample> constant* atlas_0;
    sampler samplerState_0;
};


#line 23
[[vertex]] VOut_0 vertexMain(vertexInput_0 _S2 [[stage_in]], Camera_natural_0 constant* camera_1 [[buffer(1)]], texture2d<float, access::sample> constant* atlas_1 [[buffer(0)]], sampler samplerState_1 [[sampler(0)]])
{

#line 23
    KernelContext_0 kernelContext_0;

#line 23
    (&kernelContext_0)->camera_0 = camera_1;

#line 23
    (&kernelContext_0)->atlas_0 = atlas_1;

#line 23
    (&kernelContext_0)->samplerState_0 = samplerState_1;

    thread VOut_0 output_0;
    (&output_0)->position_0 = (((float4(_S2.position_1, 1.0)) * (unpackStorage_0((&kernelContext_0)->camera_0->mvp_0))));
    (&output_0)->uv_0 = _S2.uv_1;
    return output_0;
}


#line 4325 "core.meta.slang"
struct pixelOutput_0
{
    float4 output_1 [[color(0)]];
};


#line 4325
struct pixelInput_0
{
    float2 uv_2 [[user(_SLANG_ATTR)]];
};


#line 32 "shaders/slang/sprite.slang"
[[fragment]] pixelOutput_0 fragmentMain(pixelInput_0 _S3 [[stage_in]], float4 position_2 [[position]], Camera_natural_0 constant* camera_2 [[buffer(1)]], texture2d<float, access::sample> constant* atlas_2 [[buffer(0)]], sampler samplerState_2 [[sampler(0)]])
{

#line 32
    KernelContext_0 kernelContext_1;

#line 32
    (&kernelContext_1)->camera_0 = camera_2;

#line 32
    (&kernelContext_1)->atlas_0 = atlas_2;

#line 32
    (&kernelContext_1)->samplerState_0 = samplerState_2;

#line 32
    pixelOutput_0 _S4 = { ((*(&kernelContext_1)->atlas_0).sample((samplerState_2), (_S3.uv_2))) };


    return _S4;
}

