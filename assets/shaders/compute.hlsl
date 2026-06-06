RWStructuredBuffer<float> DataBuffer : register(u0);

[numthreads(64, 1, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID) {
    uint index = dispatchThreadID.x;
    
    DataBuffer[index] += 0.25f; 
}