struct VSOutput {
  float4 posClip : SV_POSITION;
  float2 uv : TEXCOORD0;
};

VSOutput VSMain(uint vertexID : SV_VertexID) {
  VSOutput output;

  output.uv = float2((vertexID << 1) & 2, vertexID & 2);
  output.posClip =
      float4(output.uv.x * 2.0f - 1.0f, 1.0f - output.uv.y * 2.0f, 0.0f, 1.0f);
  return output;
}