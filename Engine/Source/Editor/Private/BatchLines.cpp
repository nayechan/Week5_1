#include "pch.h"
#include "Editor/Public/BatchLines.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Editor/Public/EditorPrimitive.h"
#include "Manager/Asset/Public/AssetManager.h"

UBatchLines::UBatchLines()
	: Grid()
	, BoundingBoxLines()
{
	Vertices.reserve(Grid.GetNumVertices() + BoundingBoxLines.GetNumVertices());
	Vertices.resize(Grid.GetNumVertices() + BoundingBoxLines.GetNumVertices());

	Grid.MergeVerticesAt(Vertices, 0);
	BoundingBoxLines.MergeVerticesAt(Vertices, Grid.GetNumVertices());

	SetIndices();

	URenderer& Renderer = URenderer::GetInstance();

	//BatchLineConstData.CellSize = 1.0f;
	//BatchLineConstData.BoundingBoxModel = FMatrix::Identity();

	/*AddWorldGridVerticesAndConstData();
	AddBoundingBoxVertices();*/

	Primitive.NumVertices = static_cast<uint32>(Vertices.size());
	Primitive.NumIndices = static_cast<uint32>(Indices.size());
	Primitive.IndexBuffer = Renderer.CreateIndexBuffer(Indices.data(), Primitive.NumIndices * sizeof(uint32));
	//Primitive.Color = FVector4(1, 1, 1, 0.2f);
	Primitive.Topology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
	Primitive.Vertexbuffer = Renderer.CreateVertexBuffer(
		Vertices.data(), Primitive.NumVertices * sizeof(FVector), true);
	/*Primitive.Location = FVector(0, 0, 0);
	Primitive.Rotation = FVector(0, 0, 0);
	Primitive.Scale = FVector(1, 1, 1);*/
	Primitive.VertexShader = UAssetManager::GetInstance().GetVertexShader(EShaderType::BatchLine);
	Primitive.InputLayout = UAssetManager::GetInstance().GetIputLayout(EShaderType::BatchLine);
	Primitive.PixelShader = UAssetManager::GetInstance().GetPixelShader(EShaderType::BatchLine);
}

UBatchLines::~UBatchLines()
{
	URenderer::ReleaseVertexBuffer(Primitive.Vertexbuffer);
	Primitive.InputLayout->Release();
	Primitive.VertexShader->Release();
	Primitive.IndexBuffer->Release();
	Primitive.PixelShader->Release();
}

void UBatchLines::UpdateUGridVertices(const float newCellSize)
{
	if (newCellSize == Grid.GetCellSize())
	{
		return;
	}
	Grid.UpdateVerticesBy(newCellSize);
	Grid.MergeVerticesAt(Vertices, 0);
	bChangedVertices = true;
}

void UBatchLines::UpdateBoundingBoxVertices(const FAABB& newBoundingBoxInfo)
{
	FAABB curBoudingBoxInfo = BoundingBoxLines.GetRenderedBoxInfo();
	if (newBoundingBoxInfo.Min == curBoudingBoxInfo.Min && newBoundingBoxInfo.Max == curBoudingBoxInfo.Max)
	{
		return;
	}

	BoundingBoxLines.UpdateVertices(newBoundingBoxInfo);
	BoundingBoxLines.MergeVerticesAt(Vertices, Grid.GetNumVertices());
	bChangedVertices = true;
}

void UBatchLines::UpdateBatchLineVertices(const float newCellSize, const FAABB& newBoundingBoxInfo)
{
	UpdateUGridVertices(newCellSize);
	UpdateBoundingBoxVertices(newBoundingBoxInfo);
	bChangedVertices = true;
}

void UBatchLines::UpdateVertexBuffer()
{
	if (bChangedVertices)
	{
		URenderer::GetInstance().UpdateVertexBuffer(Primitive.Vertexbuffer, Vertices);
	}
	bChangedVertices = false;
}

void UBatchLines::Render()
{
	URenderer& Renderer = URenderer::GetInstance();

	// to do: 아래 함수를 batch에 맞게 수정해야 함.
	Renderer.RenderPrimitiveIndexed(Primitive, Primitive.RenderState, false, sizeof(FVector), sizeof(uint32));
}

void UBatchLines::SetIndices()
{
	const uint32 numGridVertices = Grid.GetNumVertices();

	// 기존 그리드 라인 인덱스
	for (uint32 index = 0; index < numGridVertices; ++index)
	{
		Indices.push_back(index);
	}

	// Bounding Box 라인 인덱스 (LineList)
	uint32 boundingBoxLineIdx[] = {
		// 앞면
		0, 1,
		1, 2,
		2, 3,
		3, 0,

		// 뒷면
		4, 5,
		5, 6,
		6, 7,
		7, 4,

		// 옆면 연결
		0, 4,
		1, 5,
		2, 6,
		3, 7
	};

	// numGridVertices 이후에 추가된 8개의 꼭짓점에 맞춰 오프셋 적용
	for (uint32 i = 0; i < std::size(boundingBoxLineIdx); ++i)
	{
		Indices.push_back(numGridVertices + boundingBoxLineIdx[i]);
	}
}
