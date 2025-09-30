cbuffer constants : register(b0)
{
	row_major float4x4 world;
}

cbuffer PerFrame : register(b1)
{
	row_major float4x4 View;		// View Matrix Calculation of MVP Matrix
	row_major float4x4 Projection;	// Projection Matrix Calculation of MVP Matrix
};

cbuffer MaterialConstants : register(b2)
{
	float4 Ka;		// Ambient color
	float4 Kd;		// Diffuse color
	float4 Ks;		// Specular color
	float Ns;		// Specular exponent
	float Ni;		// Index of refraction
	float D;		// Dissolve factor
	uint MaterialFlags;	// Which textures are available (bitfield)
	float Time;
};

Texture2D DiffuseTexture : register(t0);	// map_Kd
Texture2D AmbientTexture : register(t1);	// map_Ka
Texture2D SpecularTexture : register(t2);	// map_Ks
Texture2D NormalTexture : register(t3);		// map_Ns
Texture2D AlphaTexture : register(t4);		// map_d
Texture2D BumpTexture : register(t5);		// map_bump

SamplerState SamplerWrap : register(s0);

// Material flags
#define HAS_DIFFUSE_MAP	 (1 << 0)
#define HAS_AMBIENT_MAP	 (1 << 1)
#define HAS_SPECULAR_MAP (1 << 2)
#define HAS_NORMAL_MAP	 (1 << 3)
#define HAS_ALPHA_MAP	 (1 << 4)
#define HAS_BUMP_MAP	 (1 << 5)

struct VS_INPUT
{
	float4 position : POSITION; // Input position from vertex buffer
	float3 normal : NORMAL;
	float2 tex : TEXCOORD0;
};

struct PS_INPUT
{
	float4 position : SV_POSITION;	// Transformed position to pass to the pixel shader
	float3 normal : TEXCOORD0;
	float2 tex : TEXCOORD1;
};

PS_INPUT mainVS(VS_INPUT input)
{
	PS_INPUT output;

	float4 tmp = input.position;
	tmp = mul(tmp, world);
	tmp = mul(tmp, View);
	tmp = mul(tmp, Projection);
	output.position = tmp;
	//output.normal = normalize(mul(float4(input.normal, 0.0f), world).xyz);
	output.tex = input.tex;

	return output;
}

float4 mainPS(PS_INPUT input) : SV_TARGET
{
	//float4 finalColor = float4(0.f, 0.f, 0.f, 1.f);

	//// Base diffuse color
	//float4 diffuseColor = Kd;
	//if (MaterialFlags & HAS_DIFFUSE_MAP)
	//{
	//	diffuseColor *= DiffuseTexture.Sample(SamplerWrap, input.tex);
	//}

	//// Ambient contribution
	//float4 ambientColor = Ka;
	//if (MaterialFlags & HAS_AMBIENT_MAP)
	//{
	//	ambientColor *= AmbientTexture.Sample(SamplerWrap, input.tex);
	//}

	//finalColor.rgb = diffuseColor.rgb + ambientColor.rgb;

	//// Alpha handling
	//finalColor.a = D;
	//if (MaterialFlags & HAS_ALPHA_MAP)
	//{
	//	float alpha = AlphaTexture.Sample(SamplerWrap, input.tex).r;
	//	finalColor.a *= alpha;
	//}
	
	//return finalColor;

	float2 ScrollSpeed = float2(0.0f, 0.1f);
	float2 UV = frac(input.tex + ScrollSpeed * Time);
	float4 texColor = DiffuseTexture.Sample(SamplerWrap, UV);
	return texColor;
}
