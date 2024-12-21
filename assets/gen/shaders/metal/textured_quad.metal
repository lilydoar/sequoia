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


#line 1408 "core.meta.slang"
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


#line 1409
struct SLANG_ParameterGroup_Uniforms_natural_0
{
    _MatrixStorage_float4x4_ColMajornatural_0 modelViewProjection_0;
};


#line 58 "assets/shaders/textured_quad.slang"
struct KernelContext_0
{
    SLANG_ParameterGroup_Uniforms_natural_0 constant* Uniforms_0;
    texture2d<float, access::sample> Texture_0;
    sampler Sampler_0;
};


#line 19
struct CoarseVertex_0
{
    float2 texCoord_2;
};


struct VertexStageOutput_1
{
    CoarseVertex_0 coarseVertex_0;
    float4 sv_position_1;
};


#line 39
[[vertex]] VertexStageOutput_0 vertexMain(vertexInput_0 _S2 [[stage_in]], SLANG_ParameterGroup_Uniforms_natural_0 constant* Uniforms_1 [[buffer(0)]], texture2d<float, access::sample> Texture_1 [[texture(0)]], sampler Sampler_1 [[sampler(0)]])
{

#line 39
    KernelContext_0 kernelContext_0;

#line 39
    (&kernelContext_0)->Uniforms_0 = Uniforms_1;

#line 39
    (&kernelContext_0)->Texture_0 = Texture_1;

#line 39
    (&kernelContext_0)->Sampler_0 = Sampler_1;


    thread VertexStageOutput_1 output_0;



    (&(&output_0)->coarseVertex_0)->texCoord_2 = _S2.texCoord_1;
    (&output_0)->sv_position_1 = (((float4(_S2.position_0, 1.0)) * (unpackStorage_0((&kernelContext_0)->Uniforms_0->modelViewProjection_0))));

#line 47
    thread VertexStageOutput_0 _S3;

#line 47
    (&_S3)->texCoord_0 = output_0.coarseVertex_0.texCoord_2;

#line 47
    (&_S3)->sv_position_0 = output_0.sv_position_1;

    return _S3;
}


#line 32
struct Fragment_0
{
    float4 color_0 [[color(0)]];
};


#line 32
struct pixelInput_0
{
    float2 texCoord_3 [[user(COARSEVERTEX)]];
};


#line 32
struct pixelInput_1
{
    CoarseVertex_0 coarseVertex_1;
};


#line 54
[[fragment]] Fragment_0 fragmentMain(pixelInput_0 _S4 [[stage_in]], SLANG_ParameterGroup_Uniforms_natural_0 constant* Uniforms_2 [[buffer(0)]], texture2d<float, access::sample> Texture_2 [[texture(0)]], sampler Sampler_2 [[sampler(0)]])
{

#line 54
    KernelContext_0 kernelContext_1;

#line 54
    (&kernelContext_1)->Uniforms_0 = Uniforms_2;

#line 54
    (&kernelContext_1)->Texture_0 = Texture_2;

#line 54
    (&kernelContext_1)->Sampler_0 = Sampler_2;

#line 54
    thread pixelInput_1 _S5;

#line 54
    (&(&_S5)->coarseVertex_1)->texCoord_2 = _S4.texCoord_3;


    thread Fragment_0 output_1;
    (&output_1)->color_0 = (((&kernelContext_1)->Texture_0).sample((Sampler_2), (_S5.coarseVertex_1.texCoord_2)));
    return output_1;
}

