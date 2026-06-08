struct VSIn {
    float2 position: POSITION;
    float3 color: COLOR;
};

struct VSOut {
    float4 position: SV_POSITION;
    float3 color: COLOR;
};

// vertex
VSOut VSMain(VSIn input) {
    VSOut output;
    output.position = float4(input.position, 0.0f, 1.0f);
    output.color = input.color;
    return output;
}

// fragment
float4 PSMain(VSOut input) : SV_TARGET {
    return float4(input.color, 1.0);
}
