struct Vertex
{
    float2 position : POSITION;
    float4 color    : COLOR;
};

struct VertexOutput
{
    float4 position : SV_Position;
    float4 color    : COLOR;
};

[shader("vertex")]
VertexOutput vertexMain(Vertex input)
{
    VertexOutput output;

    output.position = float4(input.position, 0.0, 1.0f);
    output.color = input.color;
    
    return output;
}

[shader("fragment")]
float4 fragmentMain(VertexOutput input) : SV_Target
{
    return input.color;
}
