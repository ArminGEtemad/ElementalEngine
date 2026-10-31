struct CameraFrameData {
  float4x4 viewMatrix;
  float4x4 projectionMatrix;
  float4x4 viewProjection;
  float4x4 invViewMatrix;
  float4x4 invProjectionMatrix;
  float4x4 invViewProjection;
  float4 cameraPosition;
  float time;
  float deltaTime;
  float2 padding;
};

struct VSOutput {
  float4 posClip : SV_POSITION;
  float3 posView : TEXCOORD0;
  float2 uv : TEXCOORD1;
  float health : TEXCOORD2;
};

struct PushConstants {
  float DomainWidth;
  float DomainDepth;
  float WorldSizeX;
  float WorldSizeZ;

  float ParticleRadius;
  float3 pad;
};

cbuffer FrameData : register(b2) { CameraFrameData camera; };
[[vk::push_constant]] PushConstants pushConstants;

struct PSOutput {
  float4 depthOut : SV_Target0;
  float depthZ : SV_Depth;
};

// calculate the ray sphere intersection and outputs the depth
PSOutput FSMain(VSOutput input) {
  PSOutput output;
  float distSq = dot(input.uv, input.uv);

  if (distSq > 1.0)
    discard;

  // Reconstruct sphere normal/depth
  float z = sqrt(1.0 - distSq);

  // The actual point on the sphere surface in view-space
  float3 spherePosView = input.posView;
  spherePosView.z += z * pushConstants.ParticleRadius;

  output.depthOut = float4(spherePosView.z, input.health, 0.0, 1.0);

  float4 clipPos = mul(camera.projectionMatrix, float4(spherePosView, 1.0));
  output.depthZ = clipPos.z / clipPos.w;

  return output;
}
