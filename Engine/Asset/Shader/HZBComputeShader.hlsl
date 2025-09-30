// HZB (Hierarchical Z-Buffer) Compute Shader
// This shader generates a hierarchical depth buffer from the scene's depth buffer
// Each mip level represents the maximum depth (farthest) for a 2x2 pixel region

// Input texture - either original depth buffer or previous mip level
Texture2D<float> SourceTexture : register(t0);

// Output texture for current mip level
RWTexture2D<float> OutputTexture : register(u0);

// Constant buffer for HZB parameters
cbuffer HZBConstants : register(b0)
{
    uint2 SourceDimensions;     // Dimensions of the source texture
    uint2 TargetDimensions;     // Dimensions of the target mip level
    uint MipLevel;              // Current mip level being generated
    float MinDepthClamp;        // Minimum depth value to clamp mip0 (prevents near object culling issues)
};

// Sample the source texture with bounds checking
float SampleDepth(uint2 coord, uint2 maxCoord)
{
    coord = min(coord, maxCoord);
    return SourceTexture[coord];
}

// Thread group size - 8x8 threads per group
[numthreads(8, 8, 1)]
void CSMain(uint3 id : SV_DispatchThreadID)
{
    // Bounds check
    if (id.x >= TargetDimensions.x || id.y >= TargetDimensions.y)
        return;

    uint2 targetCoord = id.xy;
    float maxDepth = 0.0f;

    if (MipLevel == 0)
    {
        // Level 0: Copy from source depth buffer with clamping for near objects
        maxDepth = SourceTexture[targetCoord];

        // Clamp to prevent near object culling issues (when camera is inside objects)
        maxDepth = max(maxDepth, MinDepthClamp);

        OutputTexture[targetCoord] = maxDepth;
    }
    else
    {
        // Level 1+: Sample 2x2 region from previous mip level
        uint2 sourceCoord = targetCoord * 2;
        uint2 maxSourceCoord = SourceDimensions - 1;

        // Sample 2x2 region and find maximum depth (farthest)
        float depth00 = SampleDepth(sourceCoord + uint2(0, 0), maxSourceCoord);
        float depth01 = SampleDepth(sourceCoord + uint2(1, 0), maxSourceCoord);
        float depth10 = SampleDepth(sourceCoord + uint2(0, 1), maxSourceCoord);
        float depth11 = SampleDepth(sourceCoord + uint2(1, 1), maxSourceCoord);

        maxDepth = max(max(depth00, depth01), max(depth10, depth11));
        OutputTexture[targetCoord] = maxDepth;
    }
}
