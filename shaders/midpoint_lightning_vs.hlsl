struct LightningRenderParams {
  float4x4 viewProj;
  float4 cameraPos;
  float opacity;
  float thickness;
  float2 pad;
};

struct Segments {
  float3 p0;
  float scale;
  float3 p1;
  float pad;
};

[[vk::push_constant]] LightningRenderParams renderParams;

StructuredBuffer<Segments> segments : register(t0);

struct VSOut {
  float4 position : SV_POSITION;
  float2 uv : TEXCOORD0;
  float scale : TEXCOORD1;
};

// Defines the 6 vertices for the 2 triangles of the quad
static float2 QUAD_OFFSETS[6] = {float2(0.0f, -1.0f), float2(1.0f, -1.0f),
                                 float2(0.0f, 1.0f),  float2(0.0f, 1.0f),
                                 float2(1.0f, -1.0f), float2(1.0f, 1.0f)};

VSOut VSMain(uint vertexID : SV_VertexID) {
  VSOut output;

  float thickness = renderParams.thickness;

  uint segmentID = vertexID / 6;
  uint localVertexID = vertexID % 6;

  Segments s = segments[segmentID];
  float3 p0 = s.p0;
  float3 p1 = s.p1;
  output.scale = s.scale;

  //  3D direction
  float3 dir = p1 - p0;
  float len = length(dir);

  float3 perp = float3(0.0f, 0.0f, 1.0f); // Default fallback

  if (len > 1e-5f) {
    float3 branchDir = dir / len;

    // Find the vector pointing from the lightning to the camera lens
    float3 viewDir = normalize(renderParams.cameraPos.xyz - p0);

    // Cross Product yields a vector that is perpendicular to BOTH the
    // branch and the camera -> the quad will automatically rotate its
    // flat face towards the screen!
    float3 crossProd = cross(branchDir, viewDir);
    float crossLen = length(crossProd);

    // If the camera isn't looking exactly perfectly down the barrel of the
    // branch
    if (crossLen > 1e-5f) {
      perp = crossProd / crossLen;
    }
  }

  // Determine which end of the segment this vertex belongs to
  float2 offsetMode = QUAD_OFFSETS[localVertexID];
  float3 basePos = (offsetMode.x == 0.0f) ? p0 : p1;
  float3 worldPos =
      basePos + (perp * (offsetMode.y * thickness * output.scale));

  // Project to screen
  output.position = mul(renderParams.viewProj, float4(worldPos, 1.0f));
  output.uv = offsetMode;

  return output;
}