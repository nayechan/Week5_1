#pragma once
#include <d3d11.h>
#include "Global/Vector.h"

struct FEditorPrimitive
{
	ID3D11VertexShader* VertexShader =  nullptr;
	ID3D11PixelShader* PixelShader = nullptr;
	ID3D11InputLayout* InputLayout = nullptr;
	ID3D11Buffer* Vertexbuffer = nullptr;
	ID3D11Buffer* IndexBuffer = nullptr;
	uint32 NumVertices;
	uint32 NumIndices;
	D3D11_PRIMITIVE_TOPOLOGY Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	FVector4 Color;
	FVector Location;
	FVector Rotation;
	FVector Scale;
	FRenderState RenderState;
	bool bShouldAlwaysVisible = false;
};
