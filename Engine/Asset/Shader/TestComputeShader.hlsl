// Test Compute Shader for verification
// Simple data processing compute shader

// Input/Output buffer - structured buffer for test data
RWStructuredBuffer<float4> TestBuffer : register(u0);

// Constant buffer for parameters
cbuffer ComputeConstants : register(b0)
{
    uint NumElements;
    float Multiplier;
    float2 Padding;
};

// Thread group size - 64 threads per group is common for compute shaders
[numthreads(64, 1, 1)]
void CSMain(uint3 id : SV_DispatchThreadID)
{
    // Bounds check
    if (id.x >= NumElements)
        return;

    // Simple test: multiply each element by the multiplier and add thread ID
    float4 originalValue = TestBuffer[id.x];

    // Perform some computation for testing
    TestBuffer[id.x] = float4(
        originalValue.x * Multiplier + id.x,
        originalValue.y * Multiplier + id.x * 0.1f,
        originalValue.z * Multiplier + id.x * 0.01f,
        originalValue.w * Multiplier + 1.0f
    );
}