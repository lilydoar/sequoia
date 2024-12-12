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


#line 10 "shaders/slang/triangle.slang"
struct VertexStageOutput_0
{
    float3 color_0 [[user(COARSEVERTEX)]];
    float4 sv_position_0 [[position]];
};


#line 10
struct vertexInput_0
{
    float3 position_0 [[attribute(0)]];
    float3 color_1 [[attribute(1)]];
};


#line 10
struct SLANG_ParameterGroup_Uniforms_natural_0
{
    _MatrixStorage_float4x4_ColMajornatural_0 modelViewProjection_0;
};



struct CoarseVertex_0
{
    float3 color_2;
};


#line 30
struct VertexStageOutput_1
{
    CoarseVertex_0 coarseVertex_0;
    float4 sv_position_1;
};


[[vertex]] VertexStageOutput_0 vertexMain(vertexInput_0 _S2 [[stage_in]], SLANG_ParameterGroup_Uniforms_natural_0 constant* Uniforms_0 [[buffer(0)]])
{

    thread VertexStageOutput_1 output_0;

#line 45
    (&(&output_0)->coarseVertex_0)->color_2 = _S2.color_1;
    (&output_0)->sv_position_1 = (((float4(_S2.position_0, 1.0)) * (unpackStorage_0(Uniforms_0->modelViewProjection_0))));

#line 46
    thread VertexStageOutput_0 _S3;

#line 46
    (&_S3)->color_0 = output_0.coarseVertex_0.color_2;

#line 46
    (&_S3)->sv_position_0 = output_0.sv_position_1;

    return _S3;
}


#line 23
struct Fragment_0
{
    float4 color_3 [[user(_SLANG_ATTR)]];
};


#line 23
struct pixelInput_0
{
    float3 color_4 [[user(COARSEVERTEX)]];
};


#line 23
struct pixelInput_1
{
    CoarseVertex_0 coarseVertex_1;
};


#line 54
[[fragment]] Fragment_0 fragmentMain(pixelInput_0 _S4 [[stage_in]], SLANG_ParameterGroup_Uniforms_natural_0 constant* Uniforms_1 [[buffer(0)]])
{

#line 54
    thread pixelInput_1 _S5;

#line 54
    (&(&_S5)->coarseVertex_1)->color_2 = _S4.color_4;

#line 59
    thread Fragment_0 output_1;
    (&output_1)->color_3 = float4(_S5.coarseVertex_1.color_2, 1.0);
    return output_1;
}

