struct Vertex {
  float2 pos : POSITION;
  float4 col : COLOR;
};

struct Output {
  float4 pos : SV_POSITION;
  float4 col : COLOR;
};

Output main(Vertex vert) {
  Output output;

  output.pos = float4(vert.pos, 0.0f, 1.0f);
  output.col = vert.col;

  return output;
}
