struct Vertex {
  float2 pos : POSITION;
  float2 uv : TEXCOORD;
};

struct Output {
  float4 pos : SV_POSITION;
  float2 uv : TEXCOORD;
};

Output main(Vertex vert) {
  Output output;

  output.pos = float4(vert.pos, 0.0f, 1.0f);
  output.uv = vert.uv;

  return output;
}
