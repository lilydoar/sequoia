struct Input {
  float4 pos : SV_Position;
  float4 col : COLOR0;
};

float4 main(Input input) : SV_Target {
  return input.col;
}
