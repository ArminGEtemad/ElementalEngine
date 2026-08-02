// white hot core -> yellow -> orange -> red -> dark cold

struct VSOut {
  float4 position : SV_Position;
  float2 uv : TEXTCOORD0;
  float temperature : TEXTCOORD1;
  float lifeRatio : EXTRA0;
};

float4 FSMain(VSOut input) : SV_Target {
  float2 coord = input.uv * 2.0f - 1.0f;
  float dist2 = dot(coord, coord);

  // circular shape particles
  if (dist2 > 1.0f) {
    discard;
  }

  float alpha = exp(-dist2 * 2.0f) * input.lifeRatio;

  float3 whiteHot = float3(1.0f, 0.95f, 0.8f);     // Hot combustion core
  float3 brightYellow = float3(1.0f, 0.75f, 0.1f); // Flame body
  float3 deepOrange = float3(0.95f, 0.35f, 0.02f); // Outer flame
  float3 deepRed = float3(0.6f, 0.08f, 0.01f);     // Cool flame edge
  float3 darkCold = float3(0.08f, 0.06f, 0.06f);   // Cold soot/smoke

  float3 fireColor = darkCold;
  float temp = input.temperature;

  // Map temperature to blackbody colors
  if (temp > 0.75f) {
    fireColor = lerp(brightYellow, whiteHot, (temp - 0.75f) / 0.25f);
  } else if (temp > 0.35f) {
    fireColor = lerp(deepOrange, brightYellow, (temp - 0.35f) / 0.40f);
  } else if (temp > 0.08f) {
    fireColor = lerp(deepRed, deepOrange, (temp - 0.08f) / 0.27f);
  } else {
    fireColor = lerp(darkCold, deepRed, temp / 0.08f);
  }

  return float4(fireColor * alpha, alpha);
}