Texture2D HeightmapTex : register(t0);
SamplerState LinearSampler : register(s1);

struct VSOut {
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

float4 FSMain(VSOut input) : SV_Target {
    // get the resolution of the heightmap
    float width, height;
    HeightmapTex.GetDimensions(width, height);
    float2 texelSize = float2(1.0f / width, 1.0f / height);

    // sampler the center of the height
    float hCenter = HeightmapTex.Sample(LinearSampler, input.uv).r;

    if (hCenter < 0.15f) {
        discard;
    }

    // sample neighbors to calculatee surface derivatives
    float hLeft = HeightmapTex.Sample(LinearSampler, input.uv + float2(-texelSize.x, 0.0f)).r;
    float hRight = HeightmapTex.Sample(LinearSampler, input.uv + float2(texelSize.x, 0.0f)).r;
    float hDown = HeightmapTex.Sample(LinearSampler, input.uv + float2(0.0f, -texelSize.y)).r;
    float hUp = HeightmapTex.Sample(LinearSampler, input.uv + float2(0.0f, texelSize.y)).r;

    // surface normal
    float normalScale = 0.4f; 
    float3 normal = normalize(float3(hLeft - hRight, hDown - hUp, normalScale));

    // volumetric coloring
    // center is darker, thin outer edges are bright translucent green
    float3 baseColor = float3(0.08f, 0.45f, 0.04f); 
    float3 deepColor = float3(0.01f, 0.22f, 0.01f); 
    float3 slimeColor = lerp(baseColor, deepColor, saturate(hCenter));

    float3 lightDir = normalize(float3(0.3f, 0.5f, 1.2f)); // Light source position
    float3 viewDir = float3(0.0f, 0.0f, 1.0f);            // Camera direction
    float3 halfDir = normalize(lightDir + viewDir);

    float diffuse = max(dot(normal, lightDir), 0.0f);
    float specular = pow(max(dot(normal, halfDir), 0.0f), 64.0f); // High exponent for wet shine

    // Combine diffuse, ambient (0.3f to keep it looking translucent), and shiny highlights
    float3 finalColor = slimeColor * (diffuse + 0.3f) + float3(0.8f, 1.0f, 0.3f) * specular;
    
    // Smoothly fade the outer edges of the silhouette
    float alpha = saturate((hCenter - 0.15f) * 4.0f);

    return float4(finalColor, alpha);
}