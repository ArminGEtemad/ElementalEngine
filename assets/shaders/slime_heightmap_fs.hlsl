struct VSOut {
    float4 pos : SV_Position;
    float2 uv : TEXCOORD0;
};

float4 FSMain(VSOut input) : SV_Target {
    // Convert local UVs from [0.0, 1.0] to [-1.0, 1.0]
    float2 coord = input.uv * 2.0 - 1.0;
    float distSq = dot(coord, coord);
    
    // Discard pixels outside the circular boundary of the circle 
    // or sphere when moving to 3D
    if (distSq > 1.0) {
         discard;
    }

    float height = sqrt(1.0 - distSq);

    // output the height directly to the Red channel.
    // additive blending -> these heights will stack smoothly where particles overlap
    return float4(height, 0.0f, 0.0f, 1.0f);
}