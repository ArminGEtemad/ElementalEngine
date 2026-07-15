struct VSOut {
    float4 pos : SV_Position;
    float2 uv : TEXCOORD0;
};


float4 FSMain(VSOut input) : SV_Target {

    float2 coord = input.uv * 2.0 - 1.0;
    float dist2 = dot(coord, coord);
    
    // make the slim particles circle
    if (dist2 > 1.0) {
         discard;
    }

    // coloring the goo particles
    float3 baseColor;
    baseColor = float3(0.1, 0.4, 0.05); 
    return float4(baseColor, 1.0);
}