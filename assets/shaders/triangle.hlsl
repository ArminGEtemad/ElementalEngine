struct VSOut {
    float4 Pos : SV_Position;
    float3 Color: COLOR;
};

// vertex
VSOut VSMain(uint VertexID : SV_VERTEXID) {
    VSOut output = (VSOut)0;

    if (VertexID == 0) { 
        output.Pos = float4(0.0, 0.5, 0.0, 1.0); 
        output.Color = float3(1.0, 0.0, 0.0); 
    } else if (VertexID == 1) { 
        output.Pos = float4(0.5, -0.5, 0.0, 1.0); 
        output.Color = float3(0.0, 1.0, 0.0); 
    } else if (VertexID == 2) { 
        output.Pos = float4(-0.5, -0.5, 0.0, 1.0); 
        output.Color = float3(0.0, 0.0, 1.0); 
    }
    
    return output;
}

// fragment
float4 PSMain(VSOut input) : SV_Target {
    return float4(input.Color, 1.0);
}
