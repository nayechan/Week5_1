struct PS_INPUT
{
	float4 position : SV_POSITION; // Transformed position to pass to the pixel shader
};


float4 mainPS(PS_INPUT input) : SV_TARGET
{
	return float4(0.5f, 0.5f, 0.5f, 1.0f);
}
