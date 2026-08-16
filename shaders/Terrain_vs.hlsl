struct CameraFrameData {
  float4x4 viewMatrix;
  float4x4 projectionMatrix;
  float4x4 viewProjection;
  float4 cameraPosition;
  float time;
  float deltaTime;
  float2 padding;
};

// identical to rhi common
struct Vertex {
  float4 position; // xyz
  float4 normal;   // xyz
  float4 uv;       // xy
};

StructuredBuffer<Vertex> Vertices : register(t0);
cbuffer FrameData : register(b1) { CameraFrameData camera; };

struct VSOutput {
  float4 position : SV_Position;
  float3 normal : NORMAL;
  float2 uv : TEXCOORD0;
};

VSOutput VSMain(uint vertexID : SV_VertexID) {
  VSOutput output;

  Vertex vert = Vertices[vertexID];

  output.position = mul(camera.viewProjection, float4(vert.position.xyz, 1.0f));

  output.normal = vert.normal.xyz;
  output.uv = vert.uv.xy;

  return output;
}
