// Constants
// AABB vertices form a quad (rectangle)
static const float2 aabb_vertices[4] = { float2(-0.5f, -0.5f),
                                         float2(0.5f, -0.5f),
                                         float2(-0.5f, 0.5f),
                                         float2(0.5f, 0.5f) };

// Types
struct AABB {
  // Relative to World Space
  float2 position;
  float2 size;
  uint color;
};

struct Uniforms {
  // world space ---> eye space
  float4x4 world_to_camera;
  // eye space ---> clip space
  float4x4 camera_to_clip;
  // clip space ---> normalized device space
  float4x4 clip_to_dnc;
	// world_space ---> normalized device space
  float4x4 view_projection;
};

struct Output {
  float4 pos : SV_Position;
  float4 col : COLOR0;
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
StructuredBuffer<AABB> aabb_list : register(t0, space0);

// Program
Output main(uint vertex_id: SV_VertexID, uint instance_id: SV_InstanceID) {
  AABB aabb = aabb_list[instance_id];

  float2 vertex_pos = aabb_vertices[vertex_id];
  vertex_pos *= aabb.size;
  vertex_pos += aabb.position;

  float4 pos = float4(vertex_pos, 0.0f, 1.0f);
  pos = mul(uniforms.view_projection, pos);

  Output output;
  output.pos = pos;
  output.col = unpackRGBA(aabb.color);
  return output;
}
