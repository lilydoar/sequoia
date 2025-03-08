// Constants
#define SQRT_3 1.73205f
static const float2 tri_vertices[3] = { float2(0.0f, 2.0f),
                                        float2(-SQRT_3, -1.0f),
                                        float2(SQRT_3, -1.0f) };

// Types
struct Circle {
  float2 position;
  float radius;
  uint color;
};

struct Uniforms {
  float4x4 view_projection;
};

struct Output {
  float4 pos : SV_Position;
  float4 col : COLOR0;
  float2 tri : TEXCOORD0;
};

// Functions
#define COLOR_R_SHIFT 24
#define COLOR_G_SHIFT 16
#define COLOR_B_SHIFT 8
#define COLOR_A_SHIFT 0
#define COLOR_MASK 0xFF

float4 unpackRGBA(uint color) {
  return float4(((color >> COLOR_R_SHIFT) & COLOR_MASK) / 255.0f,
                ((color >> COLOR_G_SHIFT) & COLOR_MASK) / 255.0f,
                ((color >> COLOR_B_SHIFT) & COLOR_MASK) / 255.0f,
                ((color >> COLOR_A_SHIFT) & COLOR_MASK) / 255.0f);
}

// Bindings
ConstantBuffer<Uniforms> uniforms : register(b0, space1);
StructuredBuffer<Circle> circles : register(t0, space0);

// Program
Output main(uint vertex_id: SV_VertexID, uint instance_id: SV_InstanceID) {
  Circle circle = circles[instance_id];

  float2 vertex_pos = tri_vertices[vertex_id];
  vertex_pos *= circle.radius;
  vertex_pos += circle.position.xy;

  float4 pos = float4(vertex_pos, 0.0f, 1.0f);
  pos = mul(uniforms.view_projection, pos);

  Output output;
  output.pos = pos;
  output.col = unpackRGBA(circle.color);
	output.tri = tri_vertices[vertex_id];
  return output;
}

