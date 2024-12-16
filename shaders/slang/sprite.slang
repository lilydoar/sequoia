struct Vertex
{
    float3 position;
    float2 uv;
}

struct Camera
{
    float4x4 mvp;
}

ParameterBlock<Texture2D> atlas;
ConstantBuffer<Camera> camera;
SamplerState samplerState;

struct VOut
{
    float4 position : SV_Position;
    float2 uv;
}

[shader("vertex")]
VOut vertexMain(Vertex input)
{
    VOut output;
    output.position = mul(camera.mvp, float4(input.position, 1.0));
    output.uv = input.uv;
    return output;
}

[shader("fragment")]
float4 fragmentMain(VOut input) : SV_Target
{
    let color = atlas.Sample(samplerState, input.uv);
    return color;
}
