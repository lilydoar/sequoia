cbuffer Uniform
{
    float4x4 mvp;
}

Texture2D<float4> atlas;
SamplerState samplerState;

struct Vertex
{
    float2 position;
    float2 uv;
}

struct VertexOut
{
    float4 position : SV_Position;
    float2 uv;
}

[shader("vertex")]
VertexOut vertexMain(Vertex in)
{
    VertexOut out;
    out.position = mul(mvp, float4(in.position, 0.0, 1.0));
    out.uv = in.uv;
    return out;
}

[shader("fragment")]
float4 fragmentMain(VertexOut in) : SV_Target
{
    let color = atlas.Sample(samplerState, in.uv);
    return color;
}
