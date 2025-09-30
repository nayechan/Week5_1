// Depth Buffer Copy Compute Shader
// Copies depth buffer to a float texture for HZB generation

// Input depth texture (from depth stencil buffer)
Texture2D<float> SourceDepthTexture : register(t0);

// Output depth texture (float format)
RWTexture2D<float> DestDepthTexture : register(u0);

// Thread group size
[numthreads(8, 8, 1)]
void CSMain(uint3 id : SV_DispatchThreadID)
{
    // Read depth value from source
    float depthValue = SourceDepthTexture[id.xy];

    // Write to destination
    DestDepthTexture[id.xy] = depthValue;
}