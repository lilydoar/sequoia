// Texture binding in a generic way
Texture2D<float4> Texture;
SamplerState Sampler;

// Input vertex data
struct AssembledVertex
{
    float3 position : POSITION;
    float2 texCoord : TEXCOORD0;
};

// Data passed between vertex and fragment stages
struct CoarseVertex
{
    float2 texCoord;
};

// Vertex shader output structure
struct VertexStageOutput
{
    CoarseVertex    coarseVertex    : CoarseVertex;
    float4          sv_position     : SV_Position;
};

// Fragment shader output
struct Fragment
{
    float4 color : SV_Target0;
};

// Vertex Shader
[shader("vertex")]
VertexStageOutput vertexMain(
    AssembledVertex assembledVertex)
{
    VertexStageOutput output;
    
    output.coarseVertex.texCoord = assembledVertex.texCoord;
    output.sv_position = float4(assembledVertex.position, 1.0);
    
    return output;
}

// Fragment Shader
[shader("fragment")]
Fragment fragmentMain(
    CoarseVertex coarseVertex : CoarseVertex)
{
    Fragment output;
    output.color = Texture.Sample(Sampler, coarseVertex.texCoord);
    return output;
}
