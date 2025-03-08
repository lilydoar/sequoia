struct Input {
  float4 pos : SV_Position;
  float4 col : COLOR0;
  float2 tri : TEXCOORD0;
};

float4 main(Input input) : SV_Target {
  float dist_sq = dot(input.tri, input.tri);
  float delta = fwidth(dist_sq) * 0.5;
  float alpha = smoothstep(1.0 + delta, 1.0 - delta, dist_sq);

  float4 col = input.col;
  col.a *= alpha;
  return col;
}

