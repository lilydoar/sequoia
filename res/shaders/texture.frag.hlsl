struct Fragment {
  float4 position : SV_POSITION;
  float2 uv : TEXCOORD;
};

Texture2D texture : register(t0);
SamplerState textureSampler : register(s0);

float4 main(Fragment frag) : SV_TARGET {
  return texture.Sample(textureSampler, frag.uv);
}
