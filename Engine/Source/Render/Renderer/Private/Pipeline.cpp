#include "pch.h"
#include "Render/Renderer/Public/Pipeline.h"

/// @brief 그래픽 파이프라인을 관리하는 클래스
UPipeline::UPipeline(ID3D11DeviceContext* InDeviceContext)
	: DeviceContext(InDeviceContext)
{
}

UPipeline::~UPipeline()
{
	// Device Context는 Device Resource에서 제거
}


/// @brief 파이프라인 상태를 업데이트
void UPipeline::UpdatePipeline(FPipelineInfo Info)
{
	if (LastPipelineInfo.Topology != Info.Topology) {
		DeviceContext->IASetPrimitiveTopology(Info.Topology);
		LastPipelineInfo.Topology = Info.Topology;
	}
	if (LastPipelineInfo.InputLayout != Info.InputLayout) {
		DeviceContext->IASetInputLayout(Info.InputLayout);
		LastPipelineInfo.InputLayout = Info.InputLayout;
	}
	if (LastPipelineInfo.VertexShader != Info.VertexShader) {
		DeviceContext->VSSetShader(Info.VertexShader, nullptr, 0);
		LastPipelineInfo.VertexShader = Info.VertexShader;
	}
	if (LastPipelineInfo.RasterizerState != Info.RasterizerState) {
		DeviceContext->RSSetState(Info.RasterizerState);
		LastPipelineInfo.RasterizerState = Info.RasterizerState;
	}
	if (Info.DepthStencilState) {
		DeviceContext->OMSetDepthStencilState(Info.DepthStencilState, 0);
	}
	if (LastPipelineInfo.PixelShader != Info.PixelShader) {
		DeviceContext->PSSetShader(Info.PixelShader, nullptr, 0);
		LastPipelineInfo.PixelShader = Info.PixelShader;
	}
	if (LastPipelineInfo.BlendState != Info.BlendState) {
		DeviceContext->OMSetBlendState(Info.BlendState, nullptr, 0xffffffff);
		LastPipelineInfo.BlendState = Info.BlendState;
	}
}

void UPipeline::SetIndexBuffer(ID3D11Buffer* indexBuffer, uint32 stride)
{
	uint32 fffset = 0;
	DeviceContext->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
}

/// @brief 정점 버퍼를 바인딩
void UPipeline::SetVertexBuffer(ID3D11Buffer* VertexBuffer, uint32 Stride)
{
	uint32 Offset = 0;
	DeviceContext->IASetVertexBuffers(0, 1, &VertexBuffer, &Stride, &Offset);
}

/// @brief 상수 버퍼를 설정
void UPipeline::SetConstantBuffer(uint32 Slot, bool bIsVS, ID3D11Buffer* ConstantBuffer)
{
	if (ConstantBuffer)
	{
		if (bIsVS)
			DeviceContext->VSSetConstantBuffers(Slot, 1, &ConstantBuffer);
		else
			DeviceContext->PSSetConstantBuffers(Slot, 1, &ConstantBuffer);
	}
}

/// @brief 텍스처를 설정
void UPipeline::SetTexture(uint32 Slot, bool bIsVS, ID3D11ShaderResourceView* Srv)
{
	if (Srv)
	{
		if (bIsVS)
			DeviceContext->VSSetShaderResources(Slot, 1, &Srv);
		else
			DeviceContext->PSSetShaderResources(Slot, 1, &Srv);
	}
}

/// @brief 샘플러 상태를 설정
void UPipeline::SetSamplerState(uint32 Slot, bool bIsVS, ID3D11SamplerState* SamplerState)
{
	if (SamplerState)
	{
		if (bIsVS)
			DeviceContext->VSSetSamplers(Slot, 1, &SamplerState);
		else
			DeviceContext->PSSetSamplers(Slot, 1, &SamplerState);
	}
}

/// @brief 정점 개수를 기반으로 드로우 호출
void UPipeline::Draw(uint32 VertexCount, uint32 StartLocation)
{
	DeviceContext->Draw(VertexCount, StartLocation);
}

void UPipeline::DrawIndexed(uint32 indexCount, uint32 startIndexLocation, uint32 baseVertexLocation)
{
	DeviceContext->DrawIndexed(indexCount, startIndexLocation, baseVertexLocation);
}
