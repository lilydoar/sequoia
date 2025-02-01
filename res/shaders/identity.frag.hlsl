struct Fragment {
  float4 position : SV_POSITION;
  float4 color : COLOR;
};

float4 main(Fragment frag) : SV_TARGET { return frag.color; }
