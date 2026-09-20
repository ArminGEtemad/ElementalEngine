// white hot core -> yellow -> orange -> red -> dark cold

struct VSOut {
  float4 position : SV_Position;
  float2 uv : TEXCOORD0;
  float temperature : TEXCOORD1;
  float lifeRatio : TEXCOORD2;
};

float4 FSMain(VSOut input) : SV_Target {
  float2 coord = input.uv * 2.0f - 1.0f;

  float taper = coord.y * -0.5f + 0.5f; // Top(y=-1) -> 1.0, Bottom(y=1) -> 0.0
  coord.x *= lerp(0.6f, 2.5f, taper);

  // the flame flutter as it dies out
  // We use lifeRatio to animate it over the particle's lifespan
  float scroll = (1.0f - input.lifeRatio) * 15.0f;
  float wiggle = sin(coord.y * 5.0f - scroll) * 0.15f;

  // We only want the top of the flame to wiggle, not the base, so we multiply
  // by taper.
  coord.x += wiggle * taper;

  float dist2 = dot(coord, coord);

  // Smooth teardrop crop
  if (dist2 > 1.0f) {
    discard;
  }

  // Steeper falloff for a sharper inner core
  float shapeAlpha = exp(-dist2 * 3.5f);

  // Smoothly fade the particle entirely before it dies to prevent popping
  float lifeFade = smoothstep(0.0f, 0.2f, input.lifeRatio);

  float intensityScale = 0.1f;
  float alpha = shapeAlpha * lifeFade * input.temperature * intensityScale;

  // Shifted colors to be a bit richer
  float3 whiteHot = float3(1.00f, 0.95f, 0.70f);     // Bright core
  float3 brightYellow = float3(1.00f, 0.60f, 0.05f); // Vibrant yellow
  float3 deepOrange = float3(0.80f, 0.15f, 0.01f);   // Orange edge
  float3 deepRed = float3(0.40f, 0.01f, 0.00f);      // Cool red edge
  float3 darkCold = float3(0.0f, 0.0f, 0.0f);        // Pure black

  float3 fireColor = darkCold;
  float temp = saturate(input.temperature);

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